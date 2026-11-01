#pragma once
#include "FreshModules.h"

// Noise Gate, Fuzz, Auto-Tune and Vocoder.
// Fixed-size state only: everything is sized in prepare(), the audio thread never allocates.

namespace xfx
{
//==============================================================================
// NOISE GATE — peak detector with hysteresis and a short hold, so it doesn't
// chatter around the threshold. Can be keyed from the sidechain input.
class NoiseGate : public Module
{
public:
    NoiseGate (APVTS& s, const SideChain& sc)
        : side (sc), thresh (raw (s, "ngate_thresh")), range (raw (s, "ngate_range")),
          release (raw (s, "ngate_release")), key (raw (s, "ngate_key")) {}

    void prepare (double r, int) override
    {
        sr = r;
        detAtt = coefFor (0.0005); detRel = coefFor (0.03); openCoef = coefFor (0.001);
        holdSamples = (int) (sr * 0.03);
        reset();
    }
    void reset() override { env = 0.0f; g = 1.0f; open = true; hold = 0; }

    void process (float* L, float* R, int n) override
    {
        const float openAt = juce::Decibels::decibelsToGain (thresh->load());
        const float closeAt = openAt * 0.63f;                                  // 4 dB hysteresis
        const float floorGain = juce::Decibels::decibelsToGain (-range->load(), -100.0f);
        const float closeCoef = coefFor (release->load() * 0.001);
        const bool useSide = key->load() > 0.5f && side.active();
        for (int i = 0; i < n; ++i)
        {
            const float lvl = useSide ? juce::jmax (std::abs (side.L[i]), std::abs (side.R[i]))
                                      : juce::jmax (std::abs (L[i]), std::abs (R[i]));
            env += (lvl > env ? detAtt : detRel) * (lvl - env);
            if (env > openAt) { open = true; hold = holdSamples; }
            else if (env < closeAt && --hold <= 0) open = false;
            const float target = open ? 1.0f : floorGain;
            g += (target > g ? openCoef : closeCoef) * (target - g);
            L[i] *= g; R[i] *= g;
        }
    }

    float isOpen() const { return open; }

private:
    float coefFor (double seconds) const { return (float) (1.0 - std::exp (-1.0 / (sr * juce::jmax (1.0e-5, seconds)))); }

    const SideChain& side;
    std::atomic<float> *thresh, *range, *release, *key;
    float env = 0, g = 1, detAtt = 0.1f, detRel = 0.01f, openCoef = 0.1f;
    int hold = 0, holdSamples = 1000;
    bool open = true;
};

//==============================================================================
// FUZZ — four flavours of real distortion with a tone control and a sputter gate.
//   Fuzz        asymmetric, biased transistor-style clipping
//   Hard Clip   brickwall clipping — the harshest
//   Octave Up   full-wave rectifier, adds an octave above (classic octave fuzz)
//   Wavefolder  folds the wave back on itself — metallic, synthy
class Fuzz : public Module
{
public:
    explicit Fuzz (APVTS& s)
        : type (raw (s, "fuzz_type")), drive (raw (s, "fuzz_drive")), tone (raw (s, "fuzz_tone")),
          gate (raw (s, "fuzz_gate")), mix (raw (s, "fuzz_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        driveS.reset (sr, 0.03);
        envRel = (float) (1.0 - std::exp (-1.0 / (sr * 0.05)));
        lastTone = -1.0f;
        reset();
    }
    void reset() override
    {
        for (auto& c : ch) { c.pre.reset(); c.post1.reset(); c.post2.reset(); c.dc.reset(); c.g = 1.0f; }
        env = 0.0f;
        driveS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (drive->load()));
        for (auto& c : ch) { c.pre.setCutoff (sr, 30.0); c.dc.setCutoff (sr, 12.0); c.rectAvg.setCutoff (sr, 25.0); c.rectAvg.reset(); }
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        driveS.setTargetValue (juce::Decibels::decibelsToGain (drive->load()));
        const int t = juce::jlimit (0, 3, (int) type->load());
        const float tn = tone->load();
        if (std::abs (tn - lastTone) > 0.5f)
        {
            for (auto& c : ch) { c.post1.set (0, sr, tn, 0.707); c.post2.set (0, sr, tn * 1.3, 0.6); }
            lastTone = tn;
        }
        const float gt = gate->load(), gateLvl = gt * gt * 0.08f;
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            const float k = driveS.getNextValue(), m = mixS.getNextValue();
            env += (juce::jmax (std::abs (L[i]), std::abs (R[i])) - env) * (std::abs (L[i]) > env ? 0.5f : envRel);
            const float gateTarget = gt <= 0.0f ? 1.0f : juce::jlimit (0.0f, 1.0f, (env - gateLvl * 0.5f) / (gateLvl * 0.5f + 1.0e-6f));
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                const float clean = st.pre.hp (io[c][i]);
                const float x = clean * k;
                float y;
                switch (t)
                {
                    case 1:  y = juce::jlimit (-1.0f, 1.0f, x); break;
                    case 2:   // rectify the clean signal first (that's the octave), then drive it
                    {
                        const float rect = std::abs (clean);
                        const float ac = rect - st.rectAvg.lp (rect);
                        y = std::tanh (ac * k * 2.0f) * 1.2f;
                        break;
                    }
                    case 3:  y = std::sin (juce::MathConstants<float>::halfPi * std::tanh (x * 0.25f) * 4.0f); break;
                    default: y = std::tanh (x + 0.35f) - 0.336f; y = y > 0.0f ? y : y * 0.75f; break;
                }
                st.g += (gateTarget - st.g) * 0.004f;
                y = st.dc.hp (st.post2.process (st.post1.process (y))) * st.g * 0.55f;
                io[c][i] = blend (io[c][i], y, m);
            }
        }
    }

private:
    struct Chan { OnePole pre, dc, rectAvg; Biquad post1, post2; float g = 1.0f; };
    std::atomic<float> *type, *drive, *tone, *gate, *mix;
    std::array<Chan, 2> ch;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveS;
    float env = 0.0f, envRel = 0.01f, lastTone = -1.0f;
};

//==============================================================================
// AUTO-TUNE — YIN pitch detection on a ~12 kHz decimated copy of the input,
// snapping to the nearest note of the chosen key/scale, then a two-tap
// delay-line shifter retunes the voice. Speed 0 = instant snap (the robot sound).
class AutoTune : public Module
{
public:
    static constexpr int kDecBuf = 1024, kWin = 384, kHop = 128;

    explicit AutoTune (APVTS& s)
        : key (raw (s, "tune_key")), scale (raw (s, "tune_scale")), speed (raw (s, "tune_speed")),
          amount (raw (s, "tune_amount")), mix (raw (s, "tune_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        dec = juce::jmax (1, (int) std::round (sr / 12000.0));
        dsr = sr / dec;
        aa1.set (0, sr, dsr * 0.42, 0.54); aa2.set (0, sr, dsr * 0.42, 1.31);
        tauMin = juce::jmax (2, (int) (dsr / 1000.0));
        tauMax = juce::jmin (kDecBuf - kWin - 2, (int) (dsr / 70.0));
        len = (int) (sr * 0.1) + 4;
        buf.setSize (2, len);
        win = (float) (sr * 0.03);
        reset();
    }
    void reset() override
    {
        aa1.reset(); aa2.reset();
        decBuf.fill (0.0f); dw = 0; decCount = 0; hopCount = 0;
        buf.clear(); w = 0; phase = 0.0f;
        ratio = targetRatio = 1.0f; detectedHz = 0.0f;
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float sp = speed->load();
        const float smooth = sp < 0.005f ? 0.02f : (float) (1.0 - std::exp (-1.0 / (sr * sp * 0.4)));
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };
        const float minD = (float) sr * 0.002f, pi = juce::MathConstants<float>::pi;

        for (int i = 0; i < n; ++i)
        {
            // --- analysis on a decimated mono copy
            const float mono = aa2.process (aa1.process (0.5f * (L[i] + R[i])));
            if (++decCount >= dec)
            {
                decCount = 0;
                decBuf[(size_t) dw] = mono;
                dw = (dw + 1) % kDecBuf;
                if (++hopCount >= kHop) { hopCount = 0; analyse(); }
            }

            // --- resynthesis: two crossfading taps moving at (1 - ratio)
            ratio += (targetRatio - ratio) * smooth;
            b[0][w] = L[i]; b[1][w] = R[i];
            phase += (1.0f - ratio) / win;
            phase -= std::floor (phase);
            const float p2 = phase + 0.5f - std::floor (phase + 0.5f);
            const float s1 = std::sin (pi * phase), s2 = std::sin (pi * p2);
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                const float y = read (b[c], minD + phase * win) * s1 * s1 + read (b[c], minD + p2 * win) * s2 * s2;
                io[c][i] = blend (io[c][i], y, m);
            }
            if (++w >= len) w = 0;
        }
    }

    float getDetectedHz() const { return detectedHz; }

private:
    void analyse()
    {
        // newest kWin + tauMax samples, oldest first
        const int need = kWin + tauMax;
        float* x = scratchX.data();
        float* d = scratchD.data();
        float energy = 0.0f;
        for (int j = 0; j < need; ++j)
        {
            x[j] = decBuf[(size_t) ((dw - need + j + kDecBuf * 2) % kDecBuf)];
            energy += x[j] * x[j];
        }
        if (energy / (float) need < 1.0e-5f) { targetRatio = 1.0f; detectedHz = 0.0f; return; }   // silence

        // YIN: difference function -> cumulative mean normalised difference
        float running = 0.0f;
        int best = -1;
        d[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < kWin; ++j) { const float diff = x[j] - x[j + tau]; sum += diff * diff; }
            running += sum;
            d[tau] = running > 0.0f ? sum * (float) tau / running : 1.0f;
        }
        for (int tau = tauMin; tau < tauMax; ++tau)
            if (d[tau] < 0.15f)
            {
                while (tau + 1 < tauMax && d[tau + 1] < d[tau]) ++tau;
                best = tau;
                break;
            }
        if (best < 0) { targetRatio = 1.0f; detectedHz = 0.0f; return; }   // unvoiced: leave it alone

        // parabolic interpolation for sub-sample accuracy
        float t = (float) best;
        if (best > 1 && best < tauMax - 1)
        {
            const float a = d[best - 1], bb = d[best], c = d[best + 1], den = a - 2.0f * bb + c;
            if (std::abs (den) > 1.0e-9f) t += 0.5f * (a - c) / den;
        }
        detectedHz = (float) (dsr / t);

        const float midi = 69.0f + 12.0f * std::log2 (detectedHz / 440.0f);
        const float target = nearestInScale (midi);
        const float semis = (target - midi) * amount->load();
        targetRatio = std::exp2 (semis / 12.0f);
    }

    float nearestInScale (float midi) const
    {
        static constexpr int masks[6] { 0xFFF,                                        // chromatic
                                        (1<<0)|(1<<2)|(1<<4)|(1<<5)|(1<<7)|(1<<9)|(1<<11),   // major
                                        (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<10),   // natural minor
                                        (1<<0)|(1<<2)|(1<<4)|(1<<7)|(1<<9),                  // major pentatonic
                                        (1<<0)|(1<<3)|(1<<5)|(1<<7)|(1<<10),                 // minor pentatonic
                                        (1<<0)|(1<<3)|(1<<5)|(1<<6)|(1<<7)|(1<<10) };        // blues
        const int mask = masks[juce::jlimit (0, 5, (int) scale->load())];
        const int root = juce::jlimit (0, 11, (int) key->load());
        const int centre = (int) std::round (midi);
        float best = midi, bestDist = 1.0e9f;
        for (int nn = centre - 6; nn <= centre + 6; ++nn)
        {
            const int degree = ((nn - root) % 12 + 12) % 12;
            if (! (mask & (1 << degree))) continue;
            const float dist = std::abs ((float) nn - midi);
            if (dist < bestDist) { bestDist = dist; best = (float) nn; }
        }
        return best;
    }

    float read (const float* bb, float delay) const
    {
        float rp = (float) w - delay;
        if (rp < 0.0f) rp += (float) len;
        const int iR = (int) rp, i0 = iR % len, i1 = (i0 + 1) % len;   // rp can round up to exactly len
        const float fr = (float) (rp - iR);
        return bb[i0] + (bb[i1] - bb[i0]) * fr;
    }

    std::atomic<float> *key, *scale, *speed, *amount, *mix;
    Biquad aa1, aa2;
    std::array<float, kDecBuf> decBuf {}, scratchX {}, scratchD {};
    int dec = 4, dw = 0, decCount = 0, hopCount = 0, tauMin = 12, tauMax = 171;
    double dsr = 12000.0;
    juce::AudioBuffer<float> buf;
    int len = 1, w = 0;
    float win = 1440.0f, phase = 0.0f, ratio = 1.0f, targetRatio = 1.0f, detectedHz = 0.0f;
};

//==============================================================================
// VOCODER — 16-band channel vocoder. The input (modulator) is split into bands,
// each band's loudness drives the same band of a carrier: a saw, a major chord,
// noise, or whatever is on the sidechain input. Formant slides the carrier bands
// up/down for chipmunk / giant voices.
class Vocoder : public Module
{
public:
    static constexpr int kBands = 16;

    Vocoder (APVTS& s, const SideChain& sc)
        : side (sc), carrier (raw (s, "voc_carrier")), note (raw (s, "voc_note")), formant (raw (s, "voc_formant")),
          release (raw (s, "voc_release")), mix (raw (s, "voc_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        envAtt = (float) (1.0 - std::exp (-1.0 / (sr * 0.002)));
        lastFormant = 99.0f;
        reset();
    }
    void reset() override
    {
        for (auto& b : bands) { b.m1.reset(); b.m2.reset(); b.c1.reset(); b.c2.reset(); b.env = 0.0f; }
        for (auto& p : osc) p = 0.0;
        hiss.reset();
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float fm = formant->load();
        if (std::abs (fm - lastFormant) > 1.0e-3f) { setupBands (fm); lastFormant = fm; }
        const float rel = (float) (1.0 - std::exp (-1.0 / (sr * release->load() * 0.001)));
        int src = juce::jlimit (0, 3, (int) carrier->load());
        if (src == 3 && ! side.active()) src = 1;   // no sidechain connected: use the chord

        static constexpr float chordSemis[4] { 0.0f, 4.0f, 7.0f, 12.0f };
        const float root = 440.0f * std::exp2 ((note->load() - 69.0f) / 12.0f);
        double inc[4];
        for (int v = 0; v < 4; ++v)
            inc[v] = (src == 0 ? root * (v == 0 ? 1.0f : v == 1 ? 1.004f : v == 2 ? 0.996f : 2.0f)
                               : root * std::exp2 (chordSemis[v] / 12.0f) * (v == 1 ? 1.002f : 1.0f)) / sr;

        for (int i = 0; i < n; ++i)
        {
            const float mod = 0.5f * (L[i] + R[i]);
            float car;
            if (src == 2) car = rng.nextFloat() * 2.0f - 1.0f;
            else if (src == 3) car = 0.5f * (side.L[i] + side.R[i]) * 3.0f;
            else
            {
                car = 0.0f;
                for (int v = 0; v < 4; ++v)
                {
                    const double dt = inc[v];
                    double& p = osc[(size_t) v];
                    car += polySaw ((float) p, (float) dt) * (src == 0 && v == 3 ? 0.4f : 0.5f);
                    p += dt; if (p >= 1.0) p -= 1.0;
                }
            }
            car += 0.06f * hiss.hp (rng.nextFloat() * 2.0f - 1.0f);   // a little air so S and T come through

            float outL = 0.0f, outR = 0.0f;
            for (int k = 0; k < kBands; ++k)
            {
                auto& b = bands[(size_t) k];
                const float mb = b.m2.process (b.m1.process (mod));
                const float a = std::abs (mb);
                b.env += (a > b.env ? envAtt : rel) * (a - b.env);
                const float cb = b.c2.process (b.c1.process (car)) * b.env;
                // alternate bands lean left/right a little for width
                outL += cb * ((k & 1) ? 0.8f : 1.2f);
                outR += cb * ((k & 1) ? 1.2f : 0.8f);
            }
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], outL * 14.0f, m);
            R[i] = blend (R[i], outR * 14.0f, m);
        }
    }

private:
    static float polySaw (float t, float dt)
    {
        float y = 2.0f * t - 1.0f;
        if (t < dt) { const float x = t / dt; y -= x + x - x * x - 1.0f; }
        else if (t > 1.0f - dt) { const float x = (t - 1.0f) / dt; y -= x * x + x + x + 1.0f; }
        return y;
    }

    void setupBands (float formantOct)
    {
        const double lo = 120.0, hi = 8000.0, ratio = std::pow (hi / lo, 1.0 / (kBands - 1));
        const double q = 1.0 / (std::sqrt (ratio) - 1.0 / std::sqrt (ratio)) * 1.2;
        const double shift = std::exp2 ((double) formantOct);
        for (int k = 0; k < kBands; ++k)
        {
            const double f = lo * std::pow (ratio, k);
            auto& b = bands[(size_t) k];
            b.m1.set (2, sr, f, q); b.m2.set (2, sr, f, q);
            b.c1.set (2, sr, f * shift, q); b.c2.set (2, sr, f * shift, q);
        }
        hiss.setCutoff (sr, 4000.0);
    }

    struct Band { Biquad m1, m2, c1, c2; float env = 0.0f; };
    const SideChain& side;
    std::atomic<float> *carrier, *note, *formant, *release, *mix;
    std::array<Band, kBands> bands;
    std::array<double, 4> osc {};
    OnePole hiss;
    juce::Random rng { 77 };
    float envAtt = 0.1f, lastFormant = 99.0f;
};

} // namespace xfx
