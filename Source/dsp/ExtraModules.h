#pragma once
#include "BeatModules.h"

// Glass EQ, VINYL.EXE, TTS.EXE, PUMP.EXE and Stereo Spread. All cheap: a few
// biquads or a short delay line each, nothing allocated on the audio thread.

namespace xfx
{
//==============================================================================
class GlassEQ : public Module
{
public:
    explicit GlassEQ (APVTS& s) : low (raw (s, "eq_low")), mid (raw (s, "eq_mid")), midF (raw (s, "eq_midfreq")), high (raw (s, "eq_high")) {}

    void prepare (double r, int) override { sr = r; cache[0] = 999.0f; reset(); }
    void reset() override { for (auto& c : ch) { c.lo.reset(); c.pk.reset(); c.hi.reset(); } }

    void process (float* L, float* R, int n) override
    {
        const float v[4] { low->load(), mid->load(), midF->load(), high->load() };
        if (v[0] != cache[0] || v[1] != cache[1] || v[2] != cache[2] || v[3] != cache[3])
        {
            for (auto& c : ch)
            {
                c.lo.setShelf (false, sr, 120.0, v[0]);
                c.pk.setPeak (sr, v[2], 0.9, v[1]);
                c.hi.setShelf (true, sr, 8000.0, v[3]);
            }
            std::copy (v, v + 4, cache);
        }
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            auto& st = ch[(size_t) c];
            for (int i = 0; i < n; ++i) io[c][i] = st.hi.process (st.pk.process (st.lo.process (io[c][i])));
        }
    }

private:
    struct Chan { Biquad lo, pk, hi; };
    std::atomic<float> *low, *mid, *midF, *high;
    std::array<Chan, 2> ch;
    float cache[4] { 999.0f, 0, 0, 0 };
};

//==============================================================================
class Vinyl : public Module
{
public:
    explicit Vinyl (APVTS& s) : crackle (raw (s, "vinyl_crackle")), wear (raw (s, "vinyl_wear")), wow (raw (s, "vinyl_wow")), mix (raw (s, "vinyl_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.02) + 4;
        buf.setSize (2, len);
        lastWear = -1.0f;
        reset();
    }
    void reset() override
    {
        buf.clear(); w = 0; phase = 0.0f; pop = 0.0f;
        for (auto& c : ch) { c.lp1.reset(); c.lp2.reset(); c.rumble.reset(); c.click = 0.0f; }
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float cr = crackle->load(), we = wear->load();
        if (std::abs (we - lastWear) > 1.0e-4f)
        {
            const double cut = 18000.0 * std::pow (3500.0 / 18000.0, (double) we);
            for (auto& c : ch) { c.lp1.setCutoff (sr, cut); c.lp2.setCutoff (sr, cut); c.rumble.setCutoff (sr, 25.0); }
            lastWear = we;
        }
        const float clickProb = cr * 0.0025f * (float) (48000.0 / sr);
        const float popProb = cr * 0.00004f * (float) (48000.0 / sr);
        const float ms = (float) sr * 0.001f;
        const float wowDepth = wow->load() * 1.6f * ms, base = 4.0f * ms;
        const float inc = 0.555f / (float) sr;   // 33 1/3 rpm
        const float narrow = 1.0f - we * 0.6f, hiss = we * 0.012f;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            b[0][w] = io[0][i]; b[1][w] = io[1][i];
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            float rp = (float) w - (base + wowDepth * std::sin (twoPi * phase));
            if (rp < 0.0f) rp += (float) len;
            const int i0 = (int) rp, i1 = (i0 + 1) % len;
            const float fr = rp - (float) i0;

            if (rng.nextFloat() < popProb) pop = (rng.nextFloat() * 2.0f - 1.0f) * 0.5f * cr;
            pop *= 0.92f;
            const float m = mixS.getNextValue();
            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                float v = b[c][i0] + (b[c][i1] - b[c][i0]) * fr;
                v = st.lp2.lp (st.lp1.lp (v));
                v = st.rumble.hp (v);
                if (rng.nextFloat() < clickProb) st.click = (rng.nextFloat() * 2.0f - 1.0f) * 0.18f * cr;
                st.click *= 0.45f;
                y[c] = v + st.click + pop + hiss * (rng.nextFloat() * 2.0f - 1.0f);
            }
            const float midV = 0.5f * (y[0] + y[1]), side = 0.5f * (y[0] - y[1]) * narrow;
            io[0][i] = blend (io[0][i], midV + side, m);
            io[1][i] = blend (io[1][i], midV - side, m);
            if (++w >= len) w = 0;
        }
    }

private:
    struct Chan { OnePole lp1, lp2, rumble; float click = 0.0f; };
    std::atomic<float> *crackle, *wear, *wow, *mix;
    juce::AudioBuffer<float> buf;
    std::array<Chan, 2> ch;
    juce::Random rng { 33 };
    int len = 1, w = 0;
    float phase = 0.0f, pop = 0.0f, lastWear = -1.0f;
};

//==============================================================================
// Feedback comb tuned to a "voice" pitch (metallic robot formant) plus a square-wave
// ring mod at the same pitch for that old text-to-speech buzz.
class TTSRobot : public Module
{
public:
    explicit TTSRobot (APVTS& s) : pitch (raw (s, "tts_pitch")), metal (raw (s, "tts_metal")), buzz (raw (s, "tts_buzz")), mix (raw (s, "tts_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.03) + 4;
        buf.setSize (2, len);
        pitchS.reset (sr, 0.03);
        reset();
    }
    void reset() override { buf.clear(); w = 0; phase = 0.0f; pitchS.setCurrentAndTargetValue (pitch->load()); }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        pitchS.setTargetValue (pitch->load());
        const float fb = metal->load(), bz = buzz->load() * 0.7f, norm = 1.0f - fb * 0.85f;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            const float f = pitchS.getNextValue();
            const float d = juce::jlimit (2.0f, (float) len - 2.0f, (float) sr / f);
            float rp = (float) w - d; if (rp < 0.0f) rp += (float) len;
            const int i0 = (int) rp, i1 = (i0 + 1) % len;
            const float fr = rp - (float) i0;
            phase += f / (float) sr; if (phase >= 1.0f) phase -= 1.0f;
            const float carrier = phase < 0.5f ? 1.0f : -1.0f;
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                const float delayed = b[c][i0] + (b[c][i1] - b[c][i0]) * fr;
                const float y = io[c][i] + fb * delayed;
                b[c][w] = std::tanh (y);                       // keep the loop bounded
                float out = y * norm;
                out = out + (out * carrier - out) * bz;
                io[c][i] = blend (io[c][i], out, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    std::atomic<float> *pitch, *metal, *buzz, *mix;
    juce::AudioBuffer<float> buf;
    juce::SmoothedValue<float> pitchS;
    int len = 1, w = 0;
    float phase = 0.0f;
};

//==============================================================================
// Tempo-synced volume ducking, like a kick sidechaining the track.
class Pump : public Module
{
public:
    Pump (APVTS& s, const Transport& t) : transport (t), div (raw (s, "pump_div")), depth (raw (s, "pump_depth")), shape (raw (s, "pump_shape")) {}

    void prepare (double r, int) override { sr = r; coef = (float) (1.0 - std::exp (-1.0 / (sr * 0.0015))); reset(); }
    void reset() override { g = 1.0f; }

    void process (float* L, float* R, int n) override
    {
        static constexpr double beats[] { 2.0, 1.0, 0.5, 0.25 };
        const double divBeats = beats[juce::jlimit (0, 3, (int) div->load())];
        const double beatsPerSample = transport.bpm / 60.0 / sr;
        const float d = depth->load(), e = 0.2f + shape->load() * 2.3f;
        for (int i = 0; i < n; ++i)
        {
            const double pos = (transport.ppq + i * beatsPerSample) / divBeats;
            const float ph = (float) (pos - std::floor (pos));
            const float target = 1.0f - d * (1.0f - std::pow (ph, e));
            g += coef * (target - g);
            L[i] *= g; R[i] *= g;
        }
    }

private:
    const Transport& transport;
    std::atomic<float> *div, *depth, *shape;
    float g = 1.0f, coef = 0.01f;
};

//==============================================================================
// Mono-safe widener: adds a decorrelated, slowly moving copy of the mid as side
// signal, so it gets wider in stereo and cancels out cleanly in mono.
class StereoSpread : public Module
{
public:
    explicit StereoSpread (APVTS& s) : width (raw (s, "spread_width")), motion (raw (s, "spread_motion")), mix (raw (s, "spread_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.04) + 4;
        buf.assign ((size_t) len, 0.0f);
        reset();
    }
    void reset() override { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; phase = 0.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float wd = width->load() * 0.9f, ms = (float) sr * 0.001f;
        const float mod = motion->load() * 2.0f * ms;
        const float inc = 0.23f / (float) sr;
        for (int i = 0; i < n; ++i)
        {
            const float midV = 0.5f * (L[i] + R[i]);
            buf[(size_t) w] = midV;
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            const float s = std::sin (twoPi * phase);
            const float decor = 0.5f * (read (9.0f * ms + mod * s) - read (17.0f * ms - mod * s));
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], L[i] + decor * wd, m);
            R[i] = blend (R[i], R[i] - decor * wd, m);
            if (++w >= len) w = 0;
        }
    }

private:
    float read (float d) const
    {
        float rp = (float) w - d; if (rp < 0.0f) rp += (float) len;
        const int i0 = (int) rp, i1 = (i0 + 1) % len;
        const float fr = rp - (float) i0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
    }

    std::atomic<float> *width, *motion, *mix;
    std::vector<float> buf;
    int len = 1, w = 0;
    float phase = 0.0f;
};

} // namespace xfx
