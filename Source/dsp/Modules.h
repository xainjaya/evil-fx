#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>
#include <array>
#include "../Params.h"

// All effect modules. Each one processes a stereo pair in place and reads its
// parameters straight from the APVTS (lock-free atomics), so the audio thread
// never allocates or locks.

namespace xfx
{
using APVTS = juce::AudioProcessorValueTreeState;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

inline std::atomic<float>* raw (APVTS& s, const char* id)
{
    auto* p = s.getRawParameterValue (id);
    jassert (p != nullptr);
    return p;
}

inline float softClip (float x) { return std::tanh (x); }

//==============================================================================
// Tiny RBJ biquad (no allocation when coefficients change, unlike dsp::IIR).
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    void set (int type, double sr, double f, double q)  // 0 LP, 1 HP, 2 BP
    {
        f = juce::jlimit (10.0, sr * 0.45, f);
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), sw = std::sin (w), alpha = sw / (2.0 * q);
        double nb0, nb1, nb2, na0 = 1.0 + alpha, na1 = -2.0 * cw, na2 = 1.0 - alpha;
        if (type == 0)      { nb0 = (1 - cw) / 2; nb1 = 1 - cw;    nb2 = (1 - cw) / 2; }
        else if (type == 1) { nb0 = (1 + cw) / 2; nb1 = -(1 + cw); nb2 = (1 + cw) / 2; }
        else                { nb0 = alpha;        nb1 = 0;         nb2 = -alpha; }
        b0 = float (nb0 / na0); b1 = float (nb1 / na0); b2 = float (nb2 / na0);
        a1 = float (na1 / na0); a2 = float (na2 / na0);
    }

    void setShelf (bool high, double sr, double f, double gainDb)   // RBJ shelf, slope 1
    {
        f = juce::jlimit (10.0, sr * 0.45, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / 2.0 * std::sqrt (2.0);
        const double sA = 2.0 * std::sqrt (A) * alpha, sign = high ? -1.0 : 1.0;
        const double a0 = (A + 1) + sign * (A - 1) * cw + sA;
        b0 = float (A * ((A + 1) - sign * (A - 1) * cw + sA) / a0);
        b1 = float (2.0 * sign * A * ((A - 1) - sign * (A + 1) * cw) / a0);
        b2 = float (A * ((A + 1) - sign * (A - 1) * cw - sA) / a0);
        a1 = float (-2.0 * sign * ((A - 1) + sign * (A + 1) * cw) / a0);
        a2 = float (((A + 1) + sign * (A - 1) * cw - sA) / a0);
    }

    void setPeak (double sr, double f, double q, double gainDb)
    {
        f = juce::jlimit (10.0, sr * 0.45, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        const double a0 = 1.0 + alpha / A;
        b0 = float ((1.0 + alpha * A) / a0); b1 = float (-2.0 * cw / a0); b2 = float ((1.0 - alpha * A) / a0);
        a1 = float (-2.0 * cw / a0);          a2 = float ((1.0 - alpha / A) / a0);
    }

    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

struct OnePole   // simple lowpass, coefficient from cutoff
{
    float a = 1.0f, z = 0.0f;
    void setCutoff (double sr, double hz) { a = float (1.0 - std::exp (-juce::MathConstants<double>::twoPi * hz / sr)); }
    inline float lp (float x) { z += a * (x - z); return z; }
    inline float hp (float x) { return x - lp (x); }
    void reset() { z = 0.0f; }
};

//==============================================================================
// Host transport, filled in by the processor once per chunk. When the host isn't
// playing, the processor keeps its own free-running beat clock so the BPM effects
// still work (e.g. in the standalone app or with the transport stopped).
struct Transport
{
    double ppq = 0.0;     // position in quarter notes at the start of the chunk
    double bpm = 120.0;
};

// The sidechain input for the current chunk (null when the host isn't feeding one).
struct SideChain
{
    const float* L = nullptr;
    const float* R = nullptr;
    bool active() const { return L != nullptr; }
};

// LFO phase for the modulation effects. Free = runs at the Rate knob in Hz.
// Synced = locked to the beat grid, so the sweep lands in the same place every bar.
struct LfoClock
{
    const Transport* transport = nullptr;
    double phase = 0.0, inc = 0.0;

    // call once per block; returns the per-sample increment in cycles
    void begin (int syncIndex, float freeHz, double sampleRate)
    {
        if (syncIndex <= 0 || transport == nullptr)
        {
            inc = freeHz / sampleRate;
            return;
        }
        const double beats = Params::lfoSyncBeats (syncIndex);
        phase = transport->ppq / beats;
        phase -= std::floor (phase);
        inc = transport->bpm / 60.0 / beats / sampleRate;
    }
    inline float next()   // returns the phase (0..1) for this sample, then advances
    {
        const float p = (float) phase;
        phase += inc;
        if (phase >= 1.0) phase -= std::floor (phase);
        return p;
    }
    double hz (double sampleRate) const { return inc * sampleRate; }
};

//==============================================================================
struct Module
{
    virtual ~Module() = default;
    virtual void prepare (double sampleRate, int maxBlock) = 0;
    virtual void reset() = 0;
    virtual void process (float* L, float* R, int n) = 0;

protected:
    double sr = 44100.0;
    juce::SmoothedValue<float> mixS;

    void prepareMix (double rate) { mixS.reset (rate, 0.03); }
    inline float blend (float dry, float wet, float m) const { return dry + (wet - dry) * m; }
};

//==============================================================================
// 1  CRIMSON FILTER — state-variable filter with LFO sweep
class CrimsonFilter : public Module
{
public:
    CrimsonFilter (APVTS& s, const Transport& t)
        : mode (raw (s, "filter_mode")), cutoff (raw (s, "filter_cutoff")), reso (raw (s, "filter_reso")),
          rate (raw (s, "filter_lforate")), depth (raw (s, "filter_lfodepth")), sync (raw (s, "filter_sync")) { lfo.transport = &t; }

    void prepare (double rate_, int maxBlock) override
    {
        sr = rate_;
        filter.prepare ({ sr, (juce::uint32) maxBlock, 2 });
        cutS.reset (sr, 0.04);
        cutS.setCurrentAndTargetValue (*cutoff);
    }
    void reset() override { filter.reset(); lfo.phase = 0.0; cutS.setCurrentAndTargetValue (*cutoff); }

    void process (float* L, float* R, int n) override
    {
        const int m = (int) mode->load();
        if (m != lastMode)
        {
            filter.setType (m == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                          : m == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                   : juce::dsp::StateVariableTPTFilterType::highpass);
            lastMode = m;
        }
        filter.setResonance (reso->load());
        cutS.setTargetValue (cutoff->load());
        lfo.begin ((int) sync->load(), rate->load(), sr);
        const float d = depth->load();
        const float maxF = (float) (sr * 0.45);

        for (int i = 0; i < n; ++i)
        {
            const float base = cutS.getNextValue();
            const float ph = lfo.next();
            if ((i & 15) == 0)
                filter.setCutoffFrequency (juce::jlimit (20.0f, maxF, base * std::exp2 (d * std::sin (twoPi * ph))));
            L[i] = filter.processSample (0, L[i]);
            R[i] = filter.processSample (1, R[i]);
        }
    }

private:
    std::atomic<float> *mode, *cutoff, *reso, *rate, *depth, *sync;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutS;
    LfoClock lfo;
    int lastMode = -1;
};

//==============================================================================
// 2  SCORCH DRIVE — warm asymmetric saturation with tilt EQ
class ScorchDrive : public Module
{
public:
    explicit ScorchDrive (APVTS& s) : drive (raw (s, "gloss_drive")), tone (raw (s, "gloss_tone")), mix (raw (s, "gloss_mix")) {}

    void prepare (double rate, int) override
    {
        sr = rate; prepareMix (sr);
        driveS.reset (sr, 0.03); toneS.reset (sr, 0.03);
        for (int c = 0; c < 2; ++c) { tilt[c].setCutoff (sr, 900.0); dc[c].setCutoff (sr, 12.0); }
    }
    void reset() override { for (int c = 0; c < 2; ++c) { tilt[c].reset(); dc[c].reset(); } }

    void process (float* L, float* R, int n) override
    {
        driveS.setTargetValue (juce::Decibels::decibelsToGain (drive->load()));
        toneS.setTargetValue (tone->load());
        mixS.setTargetValue (mix->load());
        constexpr float bias = 0.2f;
        const float biasOut = std::tanh (bias);

        for (int i = 0; i < n; ++i)
        {
            const float g = driveS.getNextValue(), t = toneS.getNextValue(), m = mixS.getNextValue();
            const float makeup = 1.0f / std::sqrt (g);
            float* ch[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                const float x = *ch[c];
                float y = (std::tanh (g * x + bias) - biasOut) * makeup;
                y = dc[c].hp (y);
                const float lo = tilt[c].lp (y), hi = y - lo;
                y = lo * (1.0f - 0.6f * t) + hi * (1.0f + 0.8f * t);
                *ch[c] = blend (x, y, m);
            }
        }
    }

private:
    std::atomic<float> *drive, *tone, *mix;
    juce::SmoothedValue<float> driveS, toneS;
    OnePole tilt[2], dc[2];
};

//==============================================================================
// 3  DIAL-UP — mono phone line: band-limit, line compression, hiss, hum,
//    crackle, and optional 56k modem chatter.
class DialUp : public Module
{
public:
    explicit DialUp (APVTS& s)
        : band (raw (s, "dial_band")), noise (raw (s, "dial_noise")), crackle (raw (s, "dial_crackle")),
          mix (raw (s, "dial_mix")), modem (raw (s, "dial_modem")) {}

    void prepare (double rate, int) override { sr = rate; prepareMix (sr); lastBand = -1.0f; reset(); }
    void reset() override
    {
        for (auto* b : { &hp1, &hp2, &lp1, &lp2 }) b->reset();
        crackleEnv = 0.0f; humPhase = 0.0f; tonePhase = 0.0f;
        burstLeft = 0; nextBurst = (int) (sr * 1.5); toneHold = 0;
    }

    void process (float* L, float* R, int n) override
    {
        const float b = band->load();
        if (std::abs (b - lastBand) > 1.0e-4f)
        {
            const double lo = std::exp (juce::jmap ((double) b, std::log (500.0), std::log (110.0)));
            const double hi = std::exp (juce::jmap ((double) b, std::log (2400.0), std::log (7500.0)));
            hp1.set (1, sr, lo, 0.707); hp2.set (1, sr, lo, 0.707);
            lp1.set (0, sr, hi, 0.707); lp2.set (0, sr, hi, 0.707);
            lastBand = b;
        }
        mixS.setTargetValue (mix->load());
        const float nz = noise->load(), cr = crackle->load();
        const bool squeal = modem->load() > 0.5f;
        const float crackleProb = cr * 0.0012f * (float) (48000.0 / sr);
        const float humInc = 60.0f / (float) sr;

        for (int i = 0; i < n; ++i)
        {
            float x = 0.5f * (L[i] + R[i]);

            // line hiss + 60 Hz hum go in *before* the band filter so they sound "on the line"
            humPhase += humInc; if (humPhase >= 1.0f) humPhase -= 1.0f;
            x += nz * (0.035f * (rng.nextFloat() * 2.0f - 1.0f)
                     + 0.012f * (std::sin (twoPi * humPhase) + 0.5f * std::sin (twoPi * 3.0f * humPhase)));

            if (squeal) x += modemSample();

            x = lp2.process (lp1.process (hp2.process (hp1.process (x))));
            x = std::tanh (1.7f * x) / 1.35f;          // carbon-mic style squash

            if (rng.nextFloat() < crackleProb) crackleEnv = (rng.nextFloat() * 2.0f - 1.0f) * 0.4f * cr;
            x += crackleEnv;
            crackleEnv *= 0.55f;

            const float m = mixS.getNextValue();
            L[i] = blend (L[i], x, m);
            R[i] = blend (R[i], x, m);
        }
    }

private:
    float modemSample()
    {
        if (burstLeft <= 0)
        {
            if (--nextBurst > 0) return 0.0f;
            burstLeft = (int) (sr * (0.35 + rng.nextDouble() * 0.9));
            nextBurst = (int) (sr * (2.5 + rng.nextDouble() * 5.0));
            toneHold = 0;
        }
        --burstLeft;
        if (--toneHold <= 0)   // FSK chatter: hop between classic modem carriers
        {
            static constexpr float carriers[] { 1070.0f, 1270.0f, 2025.0f, 2225.0f, 2100.0f, 1800.0f };
            toneFreq = carriers[rng.nextInt (6)];
            toneHold = (int) (sr * (0.012 + rng.nextDouble() * 0.035));
        }
        tonePhase += toneFreq / (float) sr;
        if (tonePhase >= 1.0f) tonePhase -= 1.0f;
        return 0.07f * std::sin (twoPi * tonePhase) + 0.025f * (rng.nextFloat() * 2.0f - 1.0f);
    }

    std::atomic<float> *band, *noise, *crackle, *mix, *modem;
    Biquad hp1, hp2, lp1, lp2;
    juce::Random rng { 1999 };
    float lastBand = -1.0f, crackleEnv = 0.0f, humPhase = 0.0f, tonePhase = 0.0f, toneFreq = 1270.0f;
    int burstLeft = 0, nextBurst = 0, toneHold = 0;
};

//==============================================================================
// 4  8-BIT CRUSH — bit depth + sample-rate reduction
class BitCrush : public Module
{
public:
    explicit BitCrush (APVTS& s) : bits (raw (s, "crush_bits")), rate (raw (s, "crush_rate")), mix (raw (s, "crush_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); reset(); }
    void reset() override { phase = 0.0f; held[0] = held[1] = 0.0f; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float levels = std::pow (2.0f, bits->load() - 1.0f);
        const float factor = rate->load();
        for (int i = 0; i < n; ++i)
        {
            phase += 1.0f;
            if (phase >= factor)
            {
                phase -= factor;
                held[0] = std::round (juce::jlimit (-1.0f, 1.0f, L[i]) * levels) / levels;
                held[1] = std::round (juce::jlimit (-1.0f, 1.0f, R[i]) * levels) / levels;
            }
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], held[0], m);
            R[i] = blend (R[i], held[1], m);
        }
    }

private:
    std::atomic<float> *bits, *rate, *mix;
    float phase = 0.0f, held[2] {};
};

//==============================================================================
// 5  BUFFERING — streaming-audio stutters: random slice repeats & dropouts
class Buffering : public Module
{
public:
    explicit Buffering (APVTS& s)
        : chance (raw (s, "buf_chance")), slice (raw (s, "buf_slice")), dropout (raw (s, "buf_dropout")), mix (raw (s, "buf_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        size = (int) (sr * 5.0) + 1;
        buf.setSize (2, size);
        fadeLen = juce::jmax (8, (int) (sr * 0.002));
        gmCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.002)));
        reset();
    }
    void reset() override
    {
        buf.clear(); w = 0; counter = 0; sliceLen = msToSamples (slice->load());
        state = Pass; slicesLeft = 0; gm = 0.0f;
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        auto* bl = buf.getWritePointer (0);
        auto* br = buf.getWritePointer (1);
        const float pChance = chance->load(), pDrop = dropout->load();

        for (int i = 0; i < n; ++i)
        {
            bl[w] = L[i]; br[w] = R[i];

            if (counter >= sliceLen)
            {
                counter = 0;
                if (state == Pass)
                {
                    sliceLen = msToSamples (slice->load());
                    if (rng.nextFloat() < pChance)
                    {
                        if (rng.nextFloat() < pDrop) { state = Drop;   slicesLeft = 1 + rng.nextInt (2); }
                        else                         { state = Repeat; slicesLeft = 1 + rng.nextInt (4);
                                                       loopStart = (w - sliceLen + size) % size; }
                    }
                }
                else if (--slicesLeft <= 0) state = Pass;
            }

            float gl = 0.0f, gr = 0.0f;
            if (state == Repeat)
            {
                const int idx = (loopStart + counter) % size;
                const float win = juce::jmin (1.0f, (float) juce::jmin (counter, sliceLen - 1 - counter) / (float) fadeLen);
                gl = bl[idx] * win; gr = br[idx] * win;
            }
            gm += gmCoef * ((state == Pass ? 0.0f : 1.0f) - gm);

            const float m = mixS.getNextValue();
            const float wl = L[i] + (gl - L[i]) * gm, wr = R[i] + (gr - R[i]) * gm;
            L[i] = blend (L[i], wl, m);
            R[i] = blend (R[i], wr, m);

            ++counter;
            if (++w >= size) w = 0;
        }
    }

private:
    int msToSamples (float ms) const { return juce::jmax (64, (int) (ms * 0.001 * sr)); }

    enum State { Pass, Repeat, Drop };
    std::atomic<float> *chance, *slice, *dropout, *mix;
    juce::AudioBuffer<float> buf;
    juce::Random rng { 56 };
    int size = 1, w = 0, counter = 0, sliceLen = 4096, loopStart = 0, slicesLeft = 0, fadeLen = 96;
    State state = Pass;
    float gm = 0.0f, gmCoef = 0.01f;
};

//==============================================================================
// 6  LO-BITRATE — STFT spectral "codec": band cutoff, masking threshold and
//    random bin holes produce the swirly artifacts of a low-kbps rip.
//    Adds kFFTSize samples of latency (reported to the host when enabled).
class LoBitrate : public Module
{
public:
    static constexpr int kOrder = 9, kFFTSize = 1 << kOrder, kHop = kFFTSize / 4;

    explicit LoBitrate (APVTS& s) : kbps (raw (s, "lobit_kbps")), swirl (raw (s, "lobit_swirl")), mix (raw (s, "lobit_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        for (int i = 0; i < kFFTSize; ++i)   // periodic Hann
            window[(size_t) i] = 0.5f - 0.5f * std::cos (twoPi * (float) i / (float) kFFTSize);
        reset();
    }
    void reset() override
    {
        for (auto& c : ch) { std::fill (c.in.begin(), c.in.end(), 0.0f); std::fill (c.out.begin(), c.out.end(), 0.0f); }
        pos = 0; hopCount = 0;
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                const float delayedDry = st.in[(size_t) pos];   // written kFFTSize samples ago
                st.in[(size_t) pos] = io[c][i];
                const float wet = st.out[(size_t) pos];
                st.out[(size_t) pos] = 0.0f;
                io[c][i] = blend (delayedDry, wet, m);
            }
            if (++pos >= kFFTSize) pos = 0;
            if (++hopCount >= kHop) { hopCount = 0; for (auto& st : ch) processFrame (st); }
        }
    }

private:
    struct Chan { std::array<float, kFFTSize> in {}, out {}; };

    void processFrame (Chan& st)
    {
        for (int i = 0; i < kFFTSize; ++i)
            fftData[(size_t) i] = st.in[(size_t) ((pos + i) % kFFTSize)] * window[(size_t) i];
        std::fill (fftData.begin() + kFFTSize, fftData.end(), 0.0f);
        fft.performRealOnlyForwardTransform (fftData.data());

        const float k = kbps->load(), s = swirl->load();
        const float t = (std::log (k) - std::log (8.0f)) / (std::log (192.0f) - std::log (8.0f));   // 0..1
        const float binHz = (float) sr / (float) kFFTSize;
        const int cutBin = juce::jlimit (4, kFFTSize / 2, (int) (juce::jmap (std::pow (t, 0.8f), 2500.0f, 19500.0f) / binHz));
        const float rangeDb = 16.0f + 64.0f * t;
        const float lack = 1.0f - t;
        const float holeProb = s * lack * 0.55f;
        const float stepDb = 0.5f + s * lack * 9.0f;
        const int holeFromBin = (int) (1500.0f / binHz);

        float peak = 1.0e-9f;
        for (int b = 0; b <= kFFTSize / 2; ++b)
            peak = juce::jmax (peak, std::hypot (fftData[(size_t) (2 * b)], fftData[(size_t) (2 * b + 1)]));
        const float threshold = peak * juce::Decibels::decibelsToGain (-rangeDb);

        for (int b = 0; b <= kFFTSize / 2; ++b)
        {
            float& re = fftData[(size_t) (2 * b)];
            float& im = fftData[(size_t) (2 * b + 1)];
            const float mag = std::hypot (re, im);
            bool kill = b > cutBin || mag < threshold || (b > holeFromBin && rng.nextFloat() < holeProb);
            if (kill) { re = im = 0.0f; continue; }
            if (s > 0.0f && mag > 1.0e-9f)   // coarse magnitude quantisation in dB steps
            {
                const float dbv = juce::Decibels::gainToDecibels (mag, -200.0f);
                const float q = juce::Decibels::decibelsToGain (std::round (dbv / stepDb) * stepDb);
                re *= q / mag; im *= q / mag;
            }
        }
        for (int b = 1; b < kFFTSize / 2; ++b)   // keep the spectrum conjugate-symmetric
        {
            fftData[(size_t) (2 * (kFFTSize - b))]     =  fftData[(size_t) (2 * b)];
            fftData[(size_t) (2 * (kFFTSize - b) + 1)] = -fftData[(size_t) (2 * b + 1)];
        }
        fft.performRealOnlyInverseTransform (fftData.data());

        constexpr float olaScale = 1.0f / 1.5f;   // Hann^2 at 75% overlap sums to 1.5
        for (int i = 0; i < kFFTSize; ++i)
            st.out[(size_t) ((pos + i) % kFFTSize)] += fftData[(size_t) i] * window[(size_t) i] * olaScale;
    }

    std::atomic<float> *kbps, *swirl, *mix;
    juce::dsp::FFT fft { kOrder };
    std::array<float, kFFTSize> window {};
    std::array<float, kFFTSize * 2> fftData {};
    std::array<Chan, 2> ch;
    juce::Random rng { 128 };
    int pos = 0, hopCount = 0;
};

//==============================================================================
// 7  ANGEL CHORUS
class AngelChorus : public Module
{
public:
    AngelChorus (APVTS& s, const Transport& t) : rate (raw (s, "chorus_rate")), depth (raw (s, "chorus_depth")), fb (raw (s, "chorus_fb")), mix (raw (s, "chorus_mix")), sync (raw (s, "chorus_sync")) { lfo.transport = &t; }
    void prepare (double r, int maxBlock) override { sr = r; chorus.prepare ({ sr, (juce::uint32) maxBlock, 2 }); chorus.setCentreDelay (8.0f); }
    void reset() override { chorus.reset(); }
    void process (float* L, float* R, int n) override
    {
        lfo.begin ((int) sync->load(), rate->load(), sr);
        chorus.setRate ((float) juce::jlimit (0.005, 50.0, lfo.hz (sr))); chorus.setDepth (depth->load());
        chorus.setFeedback (fb->load()); chorus.setMix (mix->load());
        float* chans[2] = { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
private:
    std::atomic<float> *rate, *depth, *fb, *mix, *sync;
    juce::dsp::Chorus<float> chorus;
    LfoClock lfo;
};

//==============================================================================
// 8  BOILING PHASER
class BoilingPhaser : public Module
{
public:
    BoilingPhaser (APVTS& s, const Transport& t) : rate (raw (s, "phaser_rate")), depth (raw (s, "phaser_depth")), fb (raw (s, "phaser_fb")), mix (raw (s, "phaser_mix")), sync (raw (s, "phaser_sync")) { lfo.transport = &t; }
    void prepare (double r, int maxBlock) override { sr = r; phaser.prepare ({ sr, (juce::uint32) maxBlock, 2 }); phaser.setCentreFrequency (1200.0f); }
    void reset() override { phaser.reset(); }
    void process (float* L, float* R, int n) override
    {
        lfo.begin ((int) sync->load(), rate->load(), sr);
        phaser.setRate ((float) juce::jlimit (0.005, 50.0, lfo.hz (sr))); phaser.setDepth (depth->load());
        phaser.setFeedback (fb->load()); phaser.setMix (mix->load());
        float* chans[2] = { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        phaser.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
private:
    std::atomic<float> *rate, *depth, *fb, *mix, *sync;
    juce::dsp::Phaser<float> phaser;
    LfoClock lfo;
};

//==============================================================================
// 9  TREM / PAN — tremolo that morphs into auto-pan; shape morphs sine -> square
class TremPan : public Module
{
public:
    TremPan (APVTS& s, const Transport& t) : rate (raw (s, "trem_rate")), depth (raw (s, "trem_depth")), shape (raw (s, "trem_shape")), pan (raw (s, "trem_pan")), sync (raw (s, "trem_sync")) { lfo.transport = &t; }
    void prepare (double r, int) override { sr = r; depthS.reset (sr, 0.03); panS.reset (sr, 0.03); }
    void reset() override { lfo.phase = 0.0; }
    void process (float* L, float* R, int n) override
    {
        depthS.setTargetValue (depth->load()); panS.setTargetValue (pan->load());
        lfo.begin ((int) sync->load(), rate->load(), sr);
        const float k = 1.0f + shape->load() * 12.0f, norm = 1.0f / std::tanh (k);
        for (int i = 0; i < n; ++i)
        {
            const float lv = std::tanh (k * std::sin (twoPi * lfo.next())) * norm;
            const float d = depthS.getNextValue(), p = panS.getNextValue();
            const float lfoR = lv * (1.0f - 2.0f * p);
            L[i] *= 1.0f - d * (0.5f - 0.5f * lv);
            R[i] *= 1.0f - d * (0.5f - 0.5f * lfoR);
        }
    }
private:
    std::atomic<float> *rate, *depth, *shape, *pan, *sync;
    juce::SmoothedValue<float> depthS, panS;
    LfoClock lfo;
};

//==============================================================================
// 10  HALO DELAY — tape-ish stereo / ping-pong delay with tempo sync
class HaloDelay : public Module
{
public:
    explicit HaloDelay (APVTS& s)
        : sync (raw (s, "delay_sync")), time (raw (s, "delay_time")), fb (raw (s, "delay_fb")),
          tone (raw (s, "delay_tone")), mix (raw (s, "delay_mix")), pp (raw (s, "delay_pp")) {}

    void setBpm (double b) { if (b > 20.0 && b < 999.0) bpm = b; }

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        size = (int) (sr * 4.5) + 4;
        buf.setSize (2, size);
        timeS.reset (sr, 0.15); fbS.reset (sr, 0.05);
        reset();
    }
    void reset() override
    {
        buf.clear(); w = 0;
        for (int c = 0; c < 2; ++c) { lp[c].reset(); hp[c].reset(); }
        timeS.setCurrentAndTargetValue (targetDelaySamples());
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        timeS.setTargetValue (targetDelaySamples());
        fbS.setTargetValue (fb->load());
        const double toneHz = 700.0 * std::pow (2.0, tone->load() * 4.6);
        for (int c = 0; c < 2; ++c) { lp[c].setCutoff (sr, toneHz); hp[c].setCutoff (sr, 90.0); }
        const bool pingPong = pp->load() > 0.5f;
        auto* bl = buf.getWritePointer (0);
        auto* br = buf.getWritePointer (1);

        for (int i = 0; i < n; ++i)
        {
            const float d = timeS.getNextValue(), f = fbS.getNextValue(), m = mixS.getNextValue();
            float rp = (float) w - d; if (rp < 0.0f) rp += (float) size;
            const int iR = (int) rp, i0 = iR % size, i1 = (i0 + 1) % size;   // rp can round up to exactly len
            const float fr = (float) (rp - iR);
            const float dl = bl[i0] + (bl[i1] - bl[i0]) * fr;
            const float dr = br[i0] + (br[i1] - br[i0]) * fr;

            const float fl = std::tanh (hp[0].hp (lp[0].lp (dl))) * f;
            const float fr2 = std::tanh (hp[1].hp (lp[1].lp (dr))) * f;

            if (pingPong) { bl[w] = 0.5f * (L[i] + R[i]) + fr2; br[w] = fl; }
            else          { bl[w] = L[i] + fl;                 br[w] = R[i] + fr2; }

            L[i] = blend (L[i], dl, m);
            R[i] = blend (R[i], dr, m);
            if (++w >= size) w = 0;
        }
    }

private:
    float targetDelaySamples() const
    {
        const int s = (int) sync->load();
        double ms = s == 0 ? (double) time->load() : 60000.0 / bpm * (double) Params::delaySyncBeats (s);
        return (float) juce::jlimit (1.0, (double) size - 4.0, ms * 0.001 * sr);
    }

    std::atomic<float> *sync, *time, *fb, *tone, *mix, *pp;
    juce::AudioBuffer<float> buf;
    juce::SmoothedValue<float> timeS, fbS;
    OnePole lp[2], hp[2];
    double bpm = 120.0;
    int size = 1, w = 0;
};

//==============================================================================
// 11  CATHEDRAL VERB — Freeverb-style hall
class CathedralVerb : public Module
{
public:
    explicit CathedralVerb (APVTS& s) : size (raw (s, "verb_size")), damp (raw (s, "verb_damp")), width (raw (s, "verb_width")), mix (raw (s, "verb_mix")) {}
    void prepare (double r, int maxBlock) override
    {
        sr = r; prepareMix (sr);
        verb.setSampleRate (sr);
        wet.setSize (2, maxBlock);
    }
    void reset() override { verb.reset(); }
    void process (float* L, float* R, int n) override
    {
        juce::Reverb::Parameters p;
        p.roomSize = 0.3f + 0.69f * size->load(); p.damping = damp->load();
        p.width = width->load(); p.wetLevel = 1.0f; p.dryLevel = 0.0f;
        verb.setParameters (p);
        mixS.setTargetValue (mix->load());
        auto* wl = wet.getWritePointer (0); auto* wr = wet.getWritePointer (1);
        std::copy (L, L + n, wl); std::copy (R, R + n, wr);
        verb.processStereo (wl, wr, n);
        for (int i = 0; i < n; ++i)
        {
            const float m = mixS.getNextValue();
            L[i] = blend (L[i], 0.55f * wl[i], m);
            R[i] = blend (R[i], 0.55f * wr[i], m);
        }
    }
private:
    std::atomic<float> *size, *damp, *width, *mix;
    juce::Reverb verb;
    juce::AudioBuffer<float> wet;
};

//==============================================================================
// Slot = module + its power switch. Fades in/out over 20 ms so toggling never
// clicks, resets the module when it wakes up, and skips it entirely when off.
class Slot
{
public:
    Slot (std::unique_ptr<Module> m, std::atomic<float>* onParam) : module (std::move (m)), on (onParam) {}

    void prepare (double sr, int maxBlock)
    {
        module->prepare (sr, maxBlock);
        dry.setSize (2, maxBlock);
        fade.reset (sr, 0.02);
        const bool isOn = on->load() > 0.5f;
        fade.setCurrentAndTargetValue (isOn ? 1.0f : 0.0f);
        awake = isOn;
        if (isOn) module->reset();
    }

    void reset() { module->reset(); }
    bool isActive() const { return awake; }
    Module* get() const { return module.get(); }

    void process (float* L, float* R, int n)
    {
        fade.setTargetValue (on->load() > 0.5f ? 1.0f : 0.0f);
        if (! fade.isSmoothing() && fade.getCurrentValue() <= 0.0f) { awake = false; return; }
        if (! awake) { module->reset(); awake = true; }

        if (! fade.isSmoothing())   // fully on
        {
            module->process (L, R, n);
            return;
        }
        auto* dl = dry.getWritePointer (0); auto* dr = dry.getWritePointer (1);
        std::copy (L, L + n, dl); std::copy (R, R + n, dr);
        module->process (L, R, n);
        for (int i = 0; i < n; ++i)
        {
            const float g = fade.getNextValue();
            L[i] = dl[i] + (L[i] - dl[i]) * g;
            R[i] = dr[i] + (R[i] - dr[i]) * g;
        }
    }

private:
    std::unique_ptr<Module> module;
    std::atomic<float>* on;
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float> fade;
    bool awake = false;
};

} // namespace xfx
