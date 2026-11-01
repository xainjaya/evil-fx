#pragma once
#include "ExtraModules.h"

// Transient Shaper, Auto-Wah, Volume Swell and Blur.
// Blur is the heaviest of the four and still only a handful of short delay lines.

namespace xfx
{
inline float toDb (float g) { return 20.0f * std::log10 (juce::jmax (g, 1.0e-6f)); }

//==============================================================================
// Two envelope followers (fast and slow). Where the fast one leads, that's the
// attack; where the slow one hangs on, that's the sustain. Boost or cut each.
class TransientShaper : public Module
{
public:
    explicit TransientShaper (APVTS& s) : attack (raw (s, "trans_attack")), sustain (raw (s, "trans_sustain")), mix (raw (s, "trans_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        auto c = [this] (double ms) { return (float) std::exp (-1.0 / (sr * ms * 0.001)); };
        fA = c (0.5); fR = c (25.0); sA = c (25.0); sR = c (250.0); gC = c (2.0);
        reset();
    }
    void reset() override { fast = slow = 0.0f; g = 1.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float at = attack->load(), su = sustain->load();
        for (int i = 0; i < n; ++i)
        {
            const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            fast = lvl > fast ? fA * fast + (1 - fA) * lvl : fR * fast + (1 - fR) * lvl;
            slow = lvl > slow ? sA * slow + (1 - sA) * lvl : sR * slow + (1 - sR) * lvl;
            const float diff = toDb (fast) - toDb (slow);
            const float gainDb = at * juce::jlimit (0.0f, 18.0f, diff) * 0.9f + su * juce::jlimit (0.0f, 18.0f, -diff) * 0.9f;
            g = gC * g + (1 - gC) * juce::Decibels::decibelsToGain (juce::jlimit (-18.0f, 18.0f, gainDb));
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], L[i] * g, m);
            R[i] = blend (R[i], R[i] * g, m);
        }
    }

private:
    std::atomic<float> *attack, *sustain, *mix;
    float fast = 0, slow = 0, g = 1, fA = 0, fR = 0, sA = 0, sR = 0, gC = 0;
};

//==============================================================================
// Envelope-following band-pass: play harder, the filter opens further.
class AutoWah : public Module
{
public:
    explicit AutoWah (APVTS& s) : sens (raw (s, "wah_sens")), range (raw (s, "wah_range")), reso (raw (s, "wah_reso")), mix (raw (s, "wah_mix")) {}

    void prepare (double r, int maxBlock) override
    {
        sr = r; prepareMix (sr);
        filter.prepare ({ sr, (juce::uint32) maxBlock, 2 });
        filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        envA = (float) std::exp (-1.0 / (sr * 0.004)); envR = (float) std::exp (-1.0 / (sr * 0.12));
        reset();
    }
    void reset() override { filter.reset(); env = 0.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float drive = 2.0f + sens->load() * 30.0f, rg = range->load() * 5.0f;
        filter.setResonance (1.5f + reso->load() * 8.0f);
        const float maxF = (float) (sr * 0.45);
        for (int i = 0; i < n; ++i)
        {
            const float lvl = juce::jmin (1.0f, juce::jmax (std::abs (L[i]), std::abs (R[i])) * drive);
            env = lvl > env ? envA * env + (1 - envA) * lvl : envR * env + (1 - envR) * lvl;
            if ((i & 15) == 0) filter.setCutoffFrequency (juce::jmin (maxF, 280.0f * std::exp2 (env * rg)));
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], filter.processSample (0, L[i]) * 1.1f, m);
            R[i] = blend (R[i], filter.processSample (1, R[i]) * 1.1f, m);
        }
    }

private:
    std::atomic<float> *sens, *range, *reso, *mix;
    juce::dsp::StateVariableTPTFilter<float> filter;
    float env = 0.0f, envA = 0.0f, envR = 0.0f;
};

//==============================================================================
// Auto-swell: every time a new note or chord starts, the volume fades in from
// silence over the Attack time, like rolling a guitar's volume knob.
class VolumeSwell : public Module
{
public:
    explicit VolumeSwell (APVTS& s) : attack (raw (s, "swell_attack")), sens (raw (s, "swell_sens")), mix (raw (s, "swell_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        auto c = [this] (double ms) { return (float) std::exp (-1.0 / (sr * ms * 0.001)); };
        fA = c (1.0); fR = c (40.0); sA = c (60.0); sR = c (300.0); gC = c (3.0);
        holdLen = (int) (sr * 0.12);
        reset();
    }
    void reset() override { fast = slow = 0.0f; ramp = 1.0f; g = 1.0f; open = false; hold = 0; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float inc = 1.0f / (attack->load() * 0.001f * (float) sr);
        const float thrDb = -55.0f + (1.0f - sens->load()) * 30.0f;   // more sensitive = lower threshold
        const float onsetDb = 9.0f - sens->load() * 5.0f;
        for (int i = 0; i < n; ++i)
        {
            const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            fast = lvl > fast ? fA * fast + (1 - fA) * lvl : fR * fast + (1 - fR) * lvl;
            slow = lvl > slow ? sA * slow + (1 - sA) * lvl : sR * slow + (1 - sR) * lvl;
            const float fDb = toDb (fast), sDb = toDb (slow);
            if (hold > 0) --hold;
            const bool newNote = (! open && fDb > thrDb) || (open && hold == 0 && fDb - sDb > onsetDb && fDb > thrDb);
            if (newNote) { ramp = 0.0f; hold = holdLen; open = true; }
            if (open && fDb < thrDb - 8.0f) open = false;
            ramp = juce::jmin (1.0f, ramp + inc);
            g = gC * g + (1 - gC) * (ramp * ramp);
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], L[i] * g, m);
            R[i] = blend (R[i], R[i] * g, m);
        }
    }

private:
    std::atomic<float> *attack, *sens, *mix;
    float fast = 0, slow = 0, ramp = 1, g = 1, fA = 0, fR = 0, sA = 0, sR = 0, gC = 0;
    bool open = false;
    int hold = 0, holdLen = 1;
};

//==============================================================================
// Blur: a feedback loop through a chain of diffusing all-passes smears everything
// into a wash. Shimmer pitches the loop up an octave each time around (angelic pads),
// and Freeze holds whatever is in the loop forever.
class Blur : public Module
{
public:
    explicit Blur (APVTS& s)
        : size (raw (s, "blur_size")), smear (raw (s, "blur_smear")), shimmer (raw (s, "blur_shimmer")),
          mix (raw (s, "blur_mix")), freeze (raw (s, "blur_freeze")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        for (int c = 0; c < 2; ++c)
        {
            auto& st = ch[(size_t) c];
            for (auto& ap : st.ap) ap.init ((int) (sr * 0.12) + 4);
            st.loop.assign ((size_t) ((int) (sr * 0.5) + 4), 0.0f);
            st.pitch.assign ((size_t) ((int) (sr * 0.1) + 4), 0.0f);
        }
        fbS.reset (sr, 0.15); inS.reset (sr, 0.05);
        reset();
    }
    void reset() override
    {
        for (auto& st : ch)
        {
            for (auto& ap : st.ap) ap.clear();
            std::fill (st.loop.begin(), st.loop.end(), 0.0f);
            std::fill (st.pitch.begin(), st.pitch.end(), 0.0f);
            st.lp = 0.0f; st.lw = 0; st.pw = 0;
        }
        modPh = 0.0f; shPh = 0.0f;
    }

    void process (float* L, float* R, int n) override
    {
        static constexpr float apMs[6] { 7.3f, 11.9f, 17.1f, 23.7f, 31.3f, 41.9f };
        mixS.setTargetValue (mix->load());
        const bool frozen = freeze->load() > 0.5f;
        fbS.setTargetValue (frozen ? 0.997f : 0.35f + smear->load() * 0.58f);
        inS.setTargetValue (frozen ? 0.0f : 1.0f);
        const float sz = 0.35f + size->load() * 1.65f;
        const float shim = shimmer->load();
        const float lpCoef = frozen ? 1.0f : 0.35f;                              // damping in the loop (off while frozen)
        const float loopMs = 60.0f + size->load() * 240.0f;
        const float pitchWin = (float) sr * 0.06f;

        for (int c = 0; c < 2; ++c)
        {
            const float spread = c == 0 ? 1.0f : 1.071f;
            for (int k = 0; k < 6; ++k) ch[(size_t) c].ap[(size_t) k].len = juce::jlimit (8, ch[(size_t) c].ap[(size_t) k].size - 2, (int) (apMs[k] * spread * sz * 0.001f * (float) sr));
        }

        for (int i = 0; i < n; ++i)
        {
            const float fb = fbS.getNextValue(), inG = inS.getNextValue(), m = mixS.getNextValue();
            modPh += 0.21f / (float) sr; if (modPh >= 1.0f) modPh -= 1.0f;
            shPh += -1.0f / pitchWin;                                             // octave up: ratio 2 -> (1 - 2) / window
            shPh -= std::floor (shPh);
            float* io[2] = { L + i, R + i };
            float wet[2];
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                const int loopLen = (int) st.loop.size();
                const float d = (loopMs * (c == 0 ? 1.0f : 1.13f) + 1.2f * std::sin (twoPi * (modPh + 0.25f * (float) c))) * 0.001f * (float) sr;
                float back = readFrac (st.loop, st.lw, d);

                if (shim > 0.0f)   // two-tap octave-up shifter inside the loop
                {
                    st.pitch[(size_t) st.pw] = back;
                    const float p2 = shPh + 0.5f - std::floor (shPh + 0.5f);
                    const float s1 = std::sin (juce::MathConstants<float>::pi * shPh), s2 = std::sin (juce::MathConstants<float>::pi * p2);
                    const float up = readFrac (st.pitch, st.pw, 2.0f + shPh * pitchWin) * s1 * s1
                                   + readFrac (st.pitch, st.pw, 2.0f + p2 * pitchWin) * s2 * s2;
                    if (++st.pw >= (int) st.pitch.size()) st.pw = 0;
                    back += (up - back) * shim * 0.7f;
                }

                st.lp += lpCoef * (back - st.lp);
                float v = *io[c] * inG + std::tanh (st.lp * fb);
                for (auto& ap : st.ap) v = ap.process (v);
                st.loop[(size_t) st.lw] = v;
                if (++st.lw >= loopLen) st.lw = 0;
                wet[c] = v;
            }
            *io[0] = blend (*io[0], wet[0] * 0.6f + wet[1] * 0.1f, m);
            *io[1] = blend (*io[1], wet[1] * 0.6f + wet[0] * 0.1f, m);
        }
    }

private:
    struct AllPass
    {
        std::vector<float> buf; int size = 0, len = 8, w = 0;
        void init (int n) { size = n; buf.assign ((size_t) n, 0.0f); }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
        inline float process (float x)
        {
            int r = w - len; if (r < 0) r += size;
            const float d = buf[(size_t) r];
            const float v = x - 0.62f * d;
            buf[(size_t) w] = v;
            if (++w >= size) w = 0;
            return d + 0.62f * v;
        }
    };
    struct Chan { std::array<AllPass, 6> ap; std::vector<float> loop, pitch; float lp = 0.0f; int lw = 0, pw = 0; };

    static float readFrac (const std::vector<float>& b, int w, float delay)
    {
        const int len = (int) b.size();
        float rp = (float) w - delay;
        while (rp < 0.0f) rp += (float) len;
        const int i0 = (int) rp % len, i1 = (i0 + 1) % len;
        const float fr = rp - std::floor (rp);
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }

    std::atomic<float> *size, *smear, *shimmer, *mix, *freeze;
    std::array<Chan, 2> ch;
    juce::SmoothedValue<float> fbS, inS;
    float modPh = 0.0f, shPh = 0.0f;
};

} // namespace xfx
