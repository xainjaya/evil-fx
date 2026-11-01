#pragma once
#include "AmbientModules.h"

// Talk Box, Frequency Shifter, Flanger and Reverse Delay.
// Same rules as everything else: fixed-size buffers allocated in prepare(),
// nothing allocated or locked on the audio thread, feedback loops bounded by tanh.

namespace xfx
{
//==============================================================================
// TALK BOX — three formant band-passes that morph through A-E-I-O-U, with an
// LFO that sweeps the vowel so it "talks". Voice shifts every formant up or down.
class TalkBox : public Module
{
public:
    TalkBox (APVTS& s, const Transport& t)
        : vowel (raw (s, "talk_vowel")), rate (raw (s, "talk_rate")), sweep (raw (s, "talk_sweep")),
          mix (raw (s, "talk_mix")), voice (raw (s, "talk_voice")), sync (raw (s, "talk_sync")) { lfo.transport = &t; }

    void prepare (double r, int) override { sr = r; prepareMix (sr); vowelS.reset (sr, 0.05); reset(); }
    void reset() override
    {
        for (auto& c : ch) for (auto& f : c) f.reset();
        lfo.phase = 0.0; countdown = 0;
        vowelS.setCurrentAndTargetValue (vowel->load());
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        vowelS.setTargetValue (vowel->load());
        static constexpr float voiceScale[] { 0.8f, 1.0f, 1.28f };
        const float scale = voiceScale[juce::jlimit (0, 2, (int) voice->load())];
        lfo.begin ((int) sync->load(), rate->load(), sr);
        const float sw = sweep->load();

        for (int i = 0; i < n; ++i)
        {
            const float v = vowelS.getNextValue();
            const float phase = lfo.next();
            if (--countdown <= 0)   // formants only need updating every 32 samples
            {
                countdown = 32;
                const float lfo = 0.5f - 0.5f * std::cos (twoPi * phase);   // 0..1, starts at the set vowel
                float vv = v + sw * (lfo - 0.5f);                            // sweep around the set vowel...
                if (vv < 0.0f) vv = -vv; else if (vv > 1.0f) vv = 2.0f - vv;  // ...bouncing off the ends
                updateFormants (juce::jlimit (0.0f, 1.0f, vv), scale);
            }
            const float m = mixS.getNextValue();
            float* io[2] = { L, R };
            for (int c = 0; c < 2; ++c)
            {
                const float x = std::tanh (io[c][i] * 1.5f) * 0.8f;   // a little grit, like a real tube in your mouth
                auto& f = ch[(size_t) c];
                const float y = (f[0].process (x) * 1.0f + f[1].process (x) * 0.7f + f[2].process (x) * 0.45f) * 2.6f;
                io[c][i] = blend (io[c][i], y, m);
            }
        }
    }

private:
    void updateFormants (float v, float scale)
    {
        // classic formant table (Hz) for A, E, I, O, U
        static constexpr float F[5][3] { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                         { 570, 840, 2410 },  { 300, 870, 2240 } };
        const float pos = v * 4.0f;
        const int a = juce::jlimit (0, 3, (int) pos);
        const float t = pos - (float) a;
        for (int k = 0; k < 3; ++k)
        {
            const float hz = (F[a][k] + (F[a + 1][k] - F[a][k]) * t) * scale;
            const double q = k == 0 ? 6.0 : k == 1 ? 9.0 : 12.0;
            for (auto& c : ch) c[(size_t) k].set (2, sr, hz, q);
        }
    }

    std::atomic<float> *vowel, *rate, *sweep, *mix, *voice, *sync;
    std::array<std::array<Biquad, 3>, 2> ch;
    juce::SmoothedValue<float> vowelS;
    LfoClock lfo;
    int countdown = 0;
};

//==============================================================================
// FREQUENCY SHIFTER — single-sideband shift via a 90-degree allpass pair
// (Olli Niemitalo's Hilbert coefficients). Unlike pitch shifting, every partial
// moves by the same number of Hz, so small shifts thicken and big ones go alien.
// Feedback gives the endless "barber pole" rise/fall. Spread shifts L and R in
// opposite directions.
class FreqShifter : public Module
{
public:
    explicit FreqShifter (APVTS& s)
        : shift (raw (s, "fshift_hz")), fb (raw (s, "fshift_fb")), spread (raw (s, "fshift_spread")), mix (raw (s, "fshift_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); shiftS.reset (sr, 0.05); reset(); }
    void reset() override
    {
        for (auto& c : ch) c = {};
        phase[0] = phase[1] = 0.0;
        shiftS.setCurrentAndTargetValue (shift->load());
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        shiftS.setTargetValue (shift->load());
        const float f = fb->load() * 0.9f, sp = spread->load();
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            const double hz = shiftS.getNextValue();
            // right channel: spread morphs from the same shift to the mirror shift
            const double step[2] { hz / sr, hz * (1.0 - 2.0 * sp) / sr };
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                phase[c] += step[c];
                phase[c] -= std::floor (phase[c]);
                const double p = phase[c];
                const float co = (float) std::cos (juce::MathConstants<double>::twoPi * p);
                const float si = (float) std::sin (juce::MathConstants<double>::twoPi * p);
                const float x = io[c][i] + std::tanh (st.last * f);
                float a = x, b = x;
                for (int k = 0; k < 4; ++k) a = st.pa[k].process (a, kA[k]);
                for (int k = 0; k < 4; ++k) b = st.pb[k].process (b, kB[k]);
                const float ad = st.aDelay; st.aDelay = a;   // path A runs one sample late by design
                const float y = ad * co + b * si;
                st.last = y;
                io[c][i] = blend (io[c][i], y, m);
            }
        }
    }

private:
    struct AP   // y[n] = a^2 (x[n] + y[n-2]) - x[n-2]
    {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        inline float process (float x, float a2)
        {
            const float y = a2 * (x + y2) - x2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
    };
    struct Chan { AP pa[4], pb[4]; float aDelay = 0, last = 0; };
    static constexpr float kA[4] { 0.6923878f * 0.6923878f, 0.9360654322959f * 0.9360654322959f,
                                   0.9882295226860f * 0.9882295226860f, 0.9987488452737f * 0.9987488452737f };
    static constexpr float kB[4] { 0.4021921162426f * 0.4021921162426f, 0.8561710882420f * 0.8561710882420f,
                                   0.9722909545651f * 0.9722909545651f, 0.9952884791278f * 0.9952884791278f };

    std::atomic<float> *shift, *fb, *spread, *mix;
    std::array<Chan, 2> ch;
    juce::SmoothedValue<float> shiftS;
    double phase[2] { 0.0, 0.0 };
};

//==============================================================================
// FLANGER — short modulated delay (0.2-7 ms) with bipolar feedback for that jet
// whoosh. The right channel's LFO runs a quarter cycle ahead for a stereo sweep.
class Flanger : public Module
{
public:
    Flanger (APVTS& s, const Transport& t) : rate (raw (s, "flange_rate")), depth (raw (s, "flange_depth")), fb (raw (s, "flange_fb")), mix (raw (s, "flange_mix")), sync (raw (s, "flange_sync")) { lfo.transport = &t; }

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.012) + 4;
        buf.setSize (2, len);
        depthS.reset (sr, 0.05);
        reset();
    }
    void reset() override { buf.clear(); w = 0; lfo.phase = 0.0; depthS.setCurrentAndTargetValue (depth->load()); }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        depthS.setTargetValue (depth->load());
        lfo.begin ((int) sync->load(), rate->load(), sr);
        const float f = fb->load();
        const float ms = (float) sr * 0.001f, minD = 0.2f * ms, maxD = 7.0f * ms;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            const float phase = lfo.next();
            const float d = depthS.getNextValue(), m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                float ph = phase + (c == 1 ? 0.25f : 0.0f); if (ph >= 1.0f) ph -= 1.0f;
                const float tri = 1.0f - std::abs (2.0f * ph - 1.0f);                // triangle sounds most "jet"
                const float lfo = tri * tri * (3.0f - 2.0f * tri);                  // smoothed corners
                const float delay = minD + (maxD - minD) * d * lfo + 0.5f;
                float rp = (float) w - delay; if (rp < 0.0f) rp += (float) len;
                const int iR = (int) rp, i0 = iR % len, i1 = (i0 + 1) % len;   // rp can round up to exactly len
                const float fr = (float) (rp - iR);
                const float del = b[c][i0] + (b[c][i1] - b[c][i0]) * fr;
                b[c][w] = std::tanh (io[c][i] + f * del);
                const float wet = 0.5f * (io[c][i] + (f < 0.0f ? -del : del));      // through-zero-ish null on negative fb
                io[c][i] = blend (io[c][i], wet * 1.4f, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    std::atomic<float> *rate, *depth, *fb, *mix, *sync;
    juce::AudioBuffer<float> buf;
    juce::SmoothedValue<float> depthS;
    LfoClock lfo;
    int len = 1, w = 0;
};

//==============================================================================
// REVERSE DELAY — records chunks and plays each one back backwards. Two read
// heads half a chunk apart with sin^2 windows (they always sum to 1) keep it
// click-free. Locks to the project tempo when Sync isn't Free.
class ReverseDelay : public Module
{
public:
    ReverseDelay (APVTS& s, const Transport& t)
        : transport (t), sync (raw (s, "rdelay_sync")), time (raw (s, "rdelay_time")), fb (raw (s, "rdelay_fb")), mix (raw (s, "rdelay_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 4.2) + 8;   // two chunks of up to 2 s, plus margin
        buf.setSize (2, len);
        chunkS.reset (sr, 0.2);
        reset();
    }
    void reset() override
    {
        buf.clear(); w = 0; pos = 0.0f; tone[0].reset(); tone[1].reset();
        for (auto& t : tone) t.setCutoff (sr, 9000.0);
        chunkS.setCurrentAndTargetValue (chunkSamples());
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        chunkS.setTargetValue (chunkSamples());
        const float f = fb->load();
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            const float N = chunkS.getNextValue();
            pos += 1.0f / N; if (pos >= 1.0f) pos -= 1.0f;
            float out[2] { 0.0f, 0.0f };
            for (int h = 0; h < 2; ++h)
            {
                float p = pos + 0.5f * (float) h; if (p >= 1.0f) p -= 1.0f;
                const float win = std::sin (juce::MathConstants<float>::pi * p);
                const float delay = 2.0f * p * N + 1.0f;   // grows 2x as fast as time -> plays backwards
                float rp = (float) w - delay; while (rp < 0.0f) rp += (float) len;
                const int iR = (int) rp, i0 = iR % len, i1 = (i0 + 1) % len;   // rp can round up to exactly len
                const float fr = (float) (rp - iR);
                for (int c = 0; c < 2; ++c) out[c] += win * win * (b[c][i0] + (b[c][i1] - b[c][i0]) * fr);
            }
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                const float wet = tone[c].lp (out[c]);
                b[c][w] = std::tanh (io[c][i] + f * wet);
                io[c][i] = blend (io[c][i], wet, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    float chunkSamples() const
    {
        const int s = (int) sync->load();
        double ms = time->load();
        if (s > 0) ms = 60000.0 / juce::jmax (20.0, transport.bpm) * Params::delaySyncBeats (s);
        return (float) juce::jlimit (20.0, (double) (len - 8) / 2.0 - 2.0, ms * 0.001 * sr);
    }

    const Transport& transport;
    std::atomic<float> *sync, *time, *fb, *mix;
    juce::AudioBuffer<float> buf;
    juce::SmoothedValue<float> chunkS;
    OnePole tone[2];
    int len = 1, w = 0;
    float pos = 0.0f;
};

} // namespace xfx
