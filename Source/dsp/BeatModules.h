#pragma once
#include "Modules.h"

// Tempo-synced and "studio essential" modules:
//   GLITCH.EXE  — beat repeat / stutter / reverse / tape stop on the grid
//   GATE.EXE    — rhythmic 16-step gate patterns
//   NIGHTCORE.EXE — pitch shifter
//   RINGTONE.EXE  — ring modulator with ringtone arpeggio
//   Glory Comp  — simple compressor with auto makeup

namespace xfx
{
// Host transport, filled in by the processor once per chunk. When the host isn't
// playing, the processor keeps its own free-running beat clock so the BPM effects
// still work (e.g. in the standalone app or with the transport stopped).
struct Transport
{
    double ppq = 0.0;     // position in quarter notes at the start of the chunk
    double bpm = 120.0;
};

//==============================================================================
class Glitch : public Module
{
public:
    Glitch (APVTS& s, const Transport& t)
        : transport (t), div (raw (s, "glitch_div")), style (raw (s, "glitch_style")),
          chance (raw (s, "glitch_chance")), mix (raw (s, "glitch_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 4.0) + 4;
        buf.setSize (2, len);
        fade = juce::jmax (16, (int) (sr * 0.003));
        microFade = juce::jmax (8, (int) (sr * 0.001));
        reset();
    }
    void reset() override { buf.clear(); w = 0; lastSlice = -1; action = None; k = 0; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const double divBeats = Params::beatDivBeats ((int) div->load());
        const double beatsPerSample = transport.bpm / 60.0 / sr;
        const int styleChoice = (int) style->load();
        const float pChance = chance->load();
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            const double ppq = transport.ppq + i * beatsPerSample;
            const auto slice = (juce::int64) std::floor (ppq / divBeats);
            if (slice != lastSlice)
            {
                lastSlice = slice;
                k = 0;
                sliceLen = juce::jlimit (64, len / 2 - 1, (int) (divBeats * 60.0 / transport.bpm * sr));
                sliceStart = w;
                readPos = (double) w;
                action = None;
                if (rng.nextFloat() < pChance)
                {
                    action = styleChoice == 0 ? (Action) (1 + rng.nextInt (4)) : (Action) styleChoice;
                    chunk = juce::jmax (32, sliceLen / (2 << rng.nextInt (3)));   // 1/2, 1/4 or 1/8 of the slice
                }
            }

            b[0][w] = io[0][i]; b[1][w] = io[1][i];
            const float m = mixS.getNextValue();

            if (action != None)
            {
                float e = juce::jmin (1.0f, (float) k / (float) fade, (float) juce::jmax (0, sliceLen - k) / (float) fade);
                float wl = 0.0f, wr = 0.0f;
                if (action == TapeStop)
                {
                    const float t = juce::jlimit (0.0f, 1.0f, (float) k / (float) sliceLen);
                    readPos += std::pow (1.0f - t, 1.5f);
                    double rp = std::fmod (readPos, (double) len);
                    const int i0 = (int) rp, i1 = (i0 + 1) % len;
                    const float fr = (float) (rp - i0);
                    wl = b[0][i0] + (b[0][i1] - b[0][i0]) * fr;
                    wr = b[1][i0] + (b[1][i1] - b[1][i0]) * fr;
                }
                else
                {
                    int idx = 0;
                    if (action == Repeat)       idx = sliceStart - sliceLen + k;
                    else if (action == Reverse) idx = sliceStart - 1 - k;
                    else
                    {
                        const int c = k % chunk;
                        idx = sliceStart + c;
                        e *= juce::jmin (1.0f, (float) c / (float) microFade, (float) (chunk - c) / (float) microFade);
                    }
                    idx = ((idx % len) + len) % len;
                    wl = b[0][idx]; wr = b[1][idx];
                }
                io[0][i] = blend (io[0][i], io[0][i] + (wl - io[0][i]) * e, m);
                io[1][i] = blend (io[1][i], io[1][i] + (wr - io[1][i]) * e, m);
            }
            ++k;
            if (++w >= len) w = 0;
        }
    }

private:
    enum Action { None = 0, Repeat = 1, Stutter = 2, Reverse = 3, TapeStop = 4 };
    const Transport& transport;
    std::atomic<float> *div, *style, *chance, *mix;
    juce::AudioBuffer<float> buf;
    juce::Random rng { 808 };
    juce::int64 lastSlice = -1;
    Action action = None;
    int len = 1, w = 0, k = 0, sliceLen = 1024, sliceStart = 0, chunk = 256, fade = 128, microFade = 48;
    double readPos = 0.0;
};

//==============================================================================
class BeatGate : public Module
{
public:
    BeatGate (APVTS& s, const Transport& t)
        : transport (t), pattern (raw (s, "gate_pattern")), div (raw (s, "gate_div")),
          depth (raw (s, "gate_depth")), smooth (raw (s, "gate_smooth")) {}

    void prepare (double r, int) override { sr = r; reset(); }
    void reset() override { g = 1.0f; lastStep = -1; randomOn = true; }

    void process (float* L, float* R, int n) override
    {
        static constexpr juce::uint16 masks[] { 0xAAAA, 0x5555, 0x9249, 0xB6DB, 0xEEAE, 0xFACA, 0xF0F0, 0x0000 };
        const int pat = juce::jlimit (0, 7, (int) pattern->load());
        const double divBeats = Params::beatDivBeats ((int) div->load());
        const double beatsPerSample = transport.bpm / 60.0 / sr;
        const float d = depth->load();
        const float coef = (float) (1.0 - std::exp (-1.0 / (sr * (0.0005 + smooth->load() * 0.025))));

        for (int i = 0; i < n; ++i)
        {
            const auto stepAbs = (juce::int64) std::floor ((transport.ppq + i * beatsPerSample) / divBeats);
            const int step = (int) (((stepAbs % 16) + 16) % 16);
            if (stepAbs != lastStep) { lastStep = stepAbs; randomOn = rng.nextFloat() < 0.6f; }
            const bool on = pat == 7 ? randomOn : ((masks[pat] >> (15 - step)) & 1) != 0;
            g += coef * ((on ? 1.0f : 1.0f - d) - g);
            L[i] *= g; R[i] *= g;
        }
    }

private:
    const Transport& transport;
    std::atomic<float> *pattern, *div, *depth, *smooth;
    juce::Random rng { 1616 };
    juce::int64 lastStep = -1;
    float g = 1.0f;
    bool randomOn = true;
};

//==============================================================================
// Two-tap delay-line pitch shifter (the classic, cheap one). Grain controls the
// window: short = tighter but buzzier, long = smoother but more chorusy.
class Nightcore : public Module
{
public:
    explicit Nightcore (APVTS& s) : semi (raw (s, "pitch_semi")), window (raw (s, "pitch_window")), mix (raw (s, "pitch_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.2) + 4;
        buf.setSize (2, len);
        reset();
    }
    void reset() override { buf.clear(); w = 0; phase = 0.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float ratio = std::pow (2.0f, semi->load() / 12.0f);
        const float win = (float) sr * (0.02f + window->load() * 0.08f);
        const float minD = (float) sr * 0.002f;
        const float inc = (1.0f - ratio) / win;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };
        const float pi = juce::MathConstants<float>::pi;

        for (int i = 0; i < n; ++i)
        {
            b[0][w] = io[0][i]; b[1][w] = io[1][i];
            phase += inc;
            phase -= std::floor (phase);
            const float p2 = phase + 0.5f - std::floor (phase + 0.5f);
            const float s1 = std::sin (pi * phase), s2 = std::sin (pi * p2);
            const float g1 = s1 * s1, g2 = s2 * s2;
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                const float y = read (b[c], minD + phase * win) * g1 + read (b[c], minD + p2 * win) * g2;
                io[c][i] = blend (io[c][i], y, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    float read (const float* b, float delay) const
    {
        float rp = (float) w - delay;
        if (rp < 0.0f) rp += (float) len;
        const int i0 = (int) rp, i1 = (i0 + 1) % len;
        const float fr = rp - (float) i0;
        return b[i0] + (b[i1] - b[i0]) * fr;
    }

    std::atomic<float> *semi, *window, *mix;
    juce::AudioBuffer<float> buf;
    int len = 1, w = 0;
    float phase = 0.0f;
};

//==============================================================================
// Ring modulator. Warble turns the carrier into a little polyphonic-ringtone arpeggio.
class Ringtone : public Module
{
public:
    explicit Ringtone (APVTS& s) : freq (raw (s, "ring_freq")), shape (raw (s, "ring_shape")), warble (raw (s, "ring_warble")), mix (raw (s, "ring_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); freqS.reset (sr, 0.02); reset(); }
    void reset() override { phase = 0.0f; arpCount = 0; arpStep = 0; freqS.setCurrentAndTargetValue (freq->load()); }

    void process (float* L, float* R, int n) override
    {
        static constexpr float arp[] { 1.0f, 1.26f, 1.5f, 2.0f };
        mixS.setTargetValue (mix->load());
        const float k = 1.0f + shape->load() * 10.0f, norm = 1.0f / std::tanh (k);
        const float wb = warble->load();
        const int arpLen = (int) (sr * 0.125);
        for (int i = 0; i < n; ++i)
        {
            if (++arpCount >= arpLen) { arpCount = 0; arpStep = (arpStep + 1) & 3; }
            freqS.setTargetValue (freq->load() * (1.0f + (arp[arpStep] - 1.0f) * wb));
            phase += freqS.getNextValue() / (float) sr;
            phase -= std::floor (phase);
            const float carrier = std::tanh (k * std::sin (twoPi * phase)) * norm;
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], L[i] * carrier, m);
            R[i] = blend (R[i], R[i] * carrier, m);
        }
    }

private:
    std::atomic<float> *freq, *shape, *warble, *mix;
    juce::SmoothedValue<float> freqS;
    float phase = 0.0f;
    int arpCount = 0, arpStep = 0;
};

//==============================================================================
// Glory Comp: Squash sets the threshold, Speed sets attack + release together,
// makeup gain is automatic. Stereo-linked, soft knee.
class GloryComp : public Module
{
public:
    explicit GloryComp (APVTS& s) : squash (raw (s, "comp_squash")), ratio (raw (s, "comp_ratio")), speed (raw (s, "comp_speed")), mix (raw (s, "comp_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); reset(); }
    void reset() override { env = 0.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float sp = speed->load();
        const float att = (float) std::exp (-1.0 / (sr * (0.0005 + sp * 0.03)));
        const float rel = (float) std::exp (-1.0 / (sr * (0.04 + sp * 0.5)));
        const float thr = -squash->load() * 40.0f, rt = ratio->load();
        const float slope = 1.0f / rt - 1.0f, knee = 6.0f;
        const float makeup = -thr * (1.0f - 1.0f / rt) * 0.5f;

        for (int i = 0; i < n; ++i)
        {
            const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            env = lvl > env ? att * env + (1.0f - att) * lvl : rel * env + (1.0f - rel) * lvl;
            const float over = juce::Decibels::gainToDecibels (env, -120.0f) - thr;
            float gr = 0.0f;
            if (over >= knee * 0.5f)      gr = slope * over;
            else if (over > -knee * 0.5f) gr = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
            const float g = juce::Decibels::decibelsToGain (gr + makeup);
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], L[i] * g, m);
            R[i] = blend (R[i], R[i] * g, m);
        }
    }

private:
    std::atomic<float> *squash, *ratio, *speed, *mix;
    float env = 0.0f;
};

} // namespace xfx
