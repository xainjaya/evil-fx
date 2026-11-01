#pragma once
#include "Modules.h"

// The "internet era" damage pack: old codecs, VoIP dropouts, corrupted downloads,
// lag spikes, warbly clocks and terrible speakers. All cheap: no FFTs, no allocation
// on the audio thread, a handful of operations per sample.

namespace xfx
{
//==============================================================================
// CODEC.EXE — 22/16/11/8/5.5 kHz sample rates through 8-bit PCM, u-Law, A-Law or
// 4-bit IMA ADPCM (the .wav/.au formats of the 90s web). "Aliasing" crossfades between
// a properly filtered converter and a raw zero-order-hold one.
class CodecSim : public Module
{
public:
    explicit CodecSim (APVTS& s)
        : rate (raw (s, "codec_rate")), type (raw (s, "codec_type")), alias (raw (s, "codec_alias")), mix (raw (s, "codec_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); lastRate = -1; reset(); }
    void reset() override
    {
        for (auto& c : ch) { for (auto* b : { &c.pre1, &c.pre2, &c.post1, &c.post2 }) b->reset(); c.held = 0.0f; c.pred = 0; c.index = 0; }
        phase = 1.0;
    }

    void process (float* L, float* R, int n) override
    {
        const int ri = (int) rate->load();
        if (ri != lastRate)
        {
            target = juce::jmin (Params::codecRateHz (ri), sr);
            for (auto& c : ch)
                for (auto* b : { &c.pre1, &c.pre2, &c.post1, &c.post2 }) b->set (0, sr, target * 0.45, 0.707);
            lastRate = ri;
        }
        const int codec = (int) type->load();
        const float a = alias->load();
        const double step = target / sr;
        mixS.setTargetValue (mix->load());
        float* io[2] = { L, R };

        for (int i = 0; i < n; ++i)
        {
            phase += step;
            const bool newSample = phase >= 1.0;
            if (newSample) phase -= 1.0;
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                const float x = io[c][i];
                const float filtered = st.pre2.process (st.pre1.process (x));
                if (newSample) st.held = encodeDecode (st, filtered + (x - filtered) * a, codec);
                const float smooth = st.post2.process (st.post1.process (st.held));
                io[c][i] = blend (x, smooth + (st.held - smooth) * a, m);
            }
        }
    }

private:
    struct Chan { Biquad pre1, pre2, post1, post2; float held = 0.0f; int pred = 0, index = 0; };

    static float encodeDecode (Chan& st, float x, int codec)
    {
        x = juce::jlimit (-1.0f, 1.0f, x);
        switch (codec)
        {
            case 0: return std::round (x * 127.0f) / 127.0f;
            case 1:
            {
                constexpr float mu = 255.0f;
                const float y = std::round (std::copysign (std::log1p (mu * std::abs (x)) / std::log1p (mu), x) * 127.0f) / 127.0f;
                return std::copysign ((std::pow (1.0f + mu, std::abs (y)) - 1.0f) / mu, y);
            }
            case 2:
            {
                constexpr float A = 87.6f; const float lnA1 = 1.0f + std::log (A);
                const float ax = std::abs (x);
                float y = ax < 1.0f / A ? A * ax / lnA1 : (1.0f + std::log (A * ax)) / lnA1;
                y = std::round (y * 127.0f) / 127.0f;
                const float d = y < 1.0f / lnA1 ? y * lnA1 / A : std::exp (y * lnA1 - 1.0f) / A;
                return std::copysign (d, x);
            }
            default: return adpcm (st, x);
        }
    }

    // IMA ADPCM: encode to a 4-bit code and immediately decode it
    static float adpcm (Chan& st, float x)
    {
        static constexpr int stepTable[89] {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
            130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060,
            1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
            7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
        static constexpr int indexTable[8] { -1, -1, -1, -1, 2, 4, 6, 8 };

        const int s = (int) (x * 32767.0f);
        const int stepSize = stepTable[st.index];
        int diff = s - st.pred, code = 0;
        if (diff < 0) { code = 8; diff = -diff; }
        int delta = stepSize >> 3;
        if (diff >= stepSize)        { code |= 4; diff -= stepSize;        delta += stepSize; }
        if (diff >= stepSize >> 1)   { code |= 2; diff -= stepSize >> 1;   delta += stepSize >> 1; }
        if (diff >= stepSize >> 2)   { code |= 1;                          delta += stepSize >> 2; }
        st.pred = juce::jlimit (-32768, 32767, st.pred + ((code & 8) ? -delta : delta));
        st.index = juce::jlimit (0, 88, st.index + indexTable[code & 7]);
        return (float) st.pred / 32768.0f;
    }

    std::atomic<float> *rate, *type, *alias, *mix;
    std::array<Chan, 2> ch;
    double phase = 1.0, target = 11025.0;
    int lastRate = -1;
};

//==============================================================================
// PACKETLOSS.EXE — VoIP-style packet loss with bursty drops. Lost packets are
// concealed the way cheap voice codecs did it: looping the last 10 ms (robot buzz),
// silence, or comfort noise.
class PacketLoss : public Module
{
public:
    explicit PacketLoss (APVTS& s)
        : loss (raw (s, "pkt_loss")), size (raw (s, "pkt_size")), conceal (raw (s, "pkt_conceal")), mix (raw (s, "pkt_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        histLen = (int) (sr * 0.1) + 1;
        hist.setSize (2, histLen);
        grainLen = juce::jmax (16, (int) (sr * 0.010));
        gCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.0015)));
        rmsCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.05)));
        reset();
    }
    void reset() override
    {
        hist.clear(); w = 0; counter = 0; packetLen = 1024; lost = prevLost = false; g = 0.0f; decay = 1.0f;
        rms[0] = rms[1] = 0.0f;
    }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float pLoss = loss->load();
        const int mode = (int) conceal->load();
        float* io[2] = { L, R };
        float* h[2] = { hist.getWritePointer (0), hist.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            if (counter >= packetLen)
            {
                counter = 0;
                packetLen = juce::jmax (32, (int) (size->load() * 0.001 * sr));
                float p = pLoss * 0.5f;
                if (prevLost) p = juce::jmin (0.92f, p * 1.8f + 0.1f * pLoss);   // losses come in bursts
                prevLost = lost;
                lost = rng.nextFloat() < p;
                if (lost && ! prevLost) { grainStart = (w - grainLen + histLen) % histLen; decay = 1.0f; grainPos = 0; }
                if (lost) decay *= 0.82f;
            }
            g += gCoef * ((lost ? 1.0f : 0.0f) - g);
            const float m = mixS.getNextValue();

            for (int c = 0; c < 2; ++c)
            {
                const float x = io[c][i];
                if (! lost) { h[c][w] = x; rms[c] += rmsCoef * (x * x - rms[c]); }
                float v = 0.0f;
                if (mode == 0)      v = h[c][(grainStart + grainPos) % histLen] * decay;
                else if (mode == 2) v = (rng.nextFloat() * 2.0f - 1.0f) * std::sqrt (rms[c]) * 0.8f;
                io[c][i] = blend (x, x + (v - x) * g, m);
            }
            if (! lost && ++w >= histLen) w = 0;
            if (++grainPos >= grainLen) grainPos = 0;
            ++counter;
        }
    }

private:
    std::atomic<float> *loss, *size, *conceal, *mix;
    juce::AudioBuffer<float> hist;
    juce::Random rng { 404 };
    int histLen = 1, w = 0, counter = 0, packetLen = 1024, grainLen = 480, grainStart = 0, grainPos = 0;
    bool lost = false, prevLost = false;
    float g = 0.0f, gCoef = 0.01f, decay = 1.0f, rms[2] {}, rmsCoef = 0.001f;
};

//==============================================================================
// CORRUPT.EXE — a damaged download: bursts of flipped bits in 16-bit samples
// (low bits = digital fizz, high bits = nasty pops) and little reversed fragments.
class Corrupt : public Module
{
public:
    explicit Corrupt (APVTS& s)
        : amount (raw (s, "corrupt_amount")), bits (raw (s, "corrupt_bits")), reverse (raw (s, "corrupt_reverse")), mix (raw (s, "corrupt_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.3) + 1;
        ring.setSize (2, len);
        reset();
    }
    void reset() override { ring.clear(); w = 0; burstLeft = 0; revLeft = 0; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float a = amount->load(), rv = reverse->load();
        const float burstProb = a * a * 0.0015f * (float) (48000.0 / sr);
        const float revProb = rv * rv * 1.5f / (float) sr;
        const int centreBit = 2 + juce::roundToInt (bits->load() * 11.0f);   // bits 2..13
        float* io[2] = { L, R };
        float* rb[2] = { ring.getWritePointer (0), ring.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            if (burstLeft <= 0 && rng.nextFloat() < burstProb)
            {
                burstLeft = 8 + rng.nextInt (400);
                mask = 0;
                for (int b = juce::jmax (0, centreBit - 2); b <= centreBit + 1; ++b)
                    if (rng.nextBool()) mask |= 1 << b;
                if (mask == 0) mask = 1 << centreBit;
            }
            if (revLeft <= 0 && rng.nextFloat() < revProb)
            {
                revTotal = revLeft = (int) (sr * (0.04 + rng.nextDouble() * 0.11));
                revStart = w;
            }
            float env = 0.0f;
            int revIdx = 0;
            if (revLeft > 0)
            {
                const int k = revTotal - revLeft;
                env = std::sin (juce::MathConstants<float>::pi * (float) k / (float) revTotal);
                revIdx = ((revStart - k) % len + len) % len;
                --revLeft;
            }
            const float m = mixS.getNextValue();
            const bool corrupting = burstLeft > 0;
            if (corrupting) --burstLeft;

            for (int c = 0; c < 2; ++c)
            {
                const float x = io[c][i];
                rb[c][w] = x;
                float y = x;
                if (corrupting)
                {
                    int s16 = (int) (juce::jlimit (-1.0f, 1.0f, y) * 32767.0f);
                    s16 ^= mask ^ ((rng.nextInt (8) == 0) ? (1 << juce::jmax (0, centreBit - 3)) : 0);
                    y = (float) juce::jlimit (-32768, 32767, s16) / 32768.0f;
                }
                if (env > 0.0f) y = y + (rb[c][revIdx] - y) * env;
                io[c][i] = blend (x, y, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    std::atomic<float> *amount, *bits, *reverse, *mix;
    juce::AudioBuffer<float> ring;
    juce::Random rng { 1337 };
    int len = 1, w = 0, burstLeft = 0, mask = 0, revLeft = 0, revTotal = 1, revStart = 0;
};

//==============================================================================
// LAG.EXE — a lag spike: the audio slows down (or freezes), then fast-forwards
// to catch back up, so it always ends up back in sync with zero added latency.
class LagSpike : public Module
{
public:
    explicit LagSpike (APVTS& s)
        : chance (raw (s, "lag_chance")), length (raw (s, "lag_length")), depth (raw (s, "lag_depth")), mix (raw (s, "lag_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 3.0) + 4;
        buf.setSize (2, len);
        speedCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.04)));
        reset();
    }
    void reset() override { buf.clear(); w = 0; delay = 0.0; speed = 1.0f; state = Normal; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float c = chance->load();
        const float startProb = c * c * 0.6f / (float) sr;
        const double maxDelay = len - 8.0;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            b[0][w] = io[0][i]; b[1][w] = io[1][i];

            if (state == Normal && rng.nextFloat() < startProb)
            {
                state = Slow;
                slowLeft = (int) (length->load() * 0.001 * sr);
                minSpeed = 1.0f - depth->load();
            }
            const float target = state == Slow ? minSpeed : state == CatchUp ? 1.8f : 1.0f;
            speed += speedCoef * (target - speed);
            if (state != Normal) delay += 1.0 - (double) speed;

            if (state == Slow && (--slowLeft <= 0 || delay >= maxDelay)) state = CatchUp;
            if (state == CatchUp && delay <= 0.0) { state = Normal; delay = 0.0; speed = 1.0f; }

            const float m = mixS.getNextValue();
            if (state == Normal) { if (++w >= len) w = 0; continue; }   // dry passthrough, nothing to do

            double rp = (double) w - delay;
            if (rp < 0.0) rp += len;
            const int iR = (int) rp, i0 = iR % len, i1 = (i0 + 1) % len;   // rp can round up to exactly len
            const float fr = (float) (rp - iR);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float y = b[ch][i0] + (b[ch][i1] - b[ch][i0]) * fr;
                io[ch][i] = blend (io[ch][i], y, m);
            }
            if (++w >= len) w = 0;
        }
    }

private:
    enum State { Normal, Slow, CatchUp };
    std::atomic<float> *chance, *length, *depth, *mix;
    juce::AudioBuffer<float> buf;
    juce::Random rng { 999 };
    int len = 1, w = 0, slowLeft = 0;
    double delay = 0.0;
    float speed = 1.0f, minSpeed = 0.3f, speedCoef = 0.001f;
    State state = Normal;
};

//==============================================================================
// JITTER.EXE — wow (slow pitch sway), flutter (fast shimmer) and clock drift
// (random wander), like a worn cassette rip or a drifting soundcard clock.
class Jitter : public Module
{
public:
    explicit Jitter (APVTS& s)
        : wow (raw (s, "jit_wow")), flutter (raw (s, "jit_flutter")), drift (raw (s, "jit_drift")), mix (raw (s, "jit_mix")) {}

    void prepare (double r, int) override
    {
        sr = r; prepareMix (sr);
        len = (int) (sr * 0.05) + 4;
        buf.setSize (2, len);
        driftCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.35)));
        reset();
    }
    void reset() override { buf.clear(); w = 0; ph1 = ph2 = 0.0f; driftVal = driftTarget = 0.0f; driftCount = 0; }

    void process (float* L, float* R, int n) override
    {
        mixS.setTargetValue (mix->load());
        const float ms = (float) sr * 0.001f;
        const float wowD = wow->load() * 4.0f * ms, flutD = flutter->load() * 0.5f * ms, driftD = drift->load() * 3.0f * ms;
        const float base = 8.5f * ms;
        const float inc1 = 0.55f / (float) sr, inc2 = 8.7f / (float) sr;
        float* io[2] = { L, R };
        float* b[2] = { buf.getWritePointer (0), buf.getWritePointer (1) };

        for (int i = 0; i < n; ++i)
        {
            b[0][w] = io[0][i]; b[1][w] = io[1][i];
            if (--driftCount <= 0) { driftTarget = rng.nextFloat() * 2.0f - 1.0f; driftCount = (int) (sr * 0.4); }
            driftVal += driftCoef * (driftTarget - driftVal);
            ph1 += inc1; if (ph1 >= 1.0f) ph1 -= 1.0f;
            ph2 += inc2 * (1.0f + 0.15f * driftVal); if (ph2 >= 1.0f) ph2 -= 1.0f;

            const float d = base + wowD * std::sin (twoPi * ph1) + flutD * std::sin (twoPi * ph2) + driftD * driftVal;
            float rp = (float) w - d; if (rp < 0.0f) rp += (float) len;
            const int iR = (int) rp, i0 = iR % len, i1 = (i0 + 1) % len;   // rp can round up to exactly len
            const float fr = (float) (rp - iR);
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
                io[c][i] = blend (io[c][i], b[c][i0] + (b[c][i1] - b[c][i0]) * fr, m);
            if (++w >= len) w = 0;
        }
    }

private:
    std::atomic<float> *wow, *flutter, *drift, *mix;
    juce::AudioBuffer<float> buf;
    juce::Random rng { 33 };
    int len = 1, w = 0, driftCount = 0;
    float ph1 = 0.0f, ph2 = 0.0f, driftVal = 0.0f, driftTarget = 0.0f, driftCoef = 0.0001f;
};

//==============================================================================
// SPEAKER.EXE — what your mix sounded like on the hardware of the day.
class SpeakerSim : public Module
{
public:
    explicit SpeakerSim (APVTS& s) : model (raw (s, "spk_model")), drive (raw (s, "spk_drive")), mix (raw (s, "spk_mix")) {}

    void prepare (double r, int) override { sr = r; prepareMix (sr); lastModel = -1; driveS.reset (sr, 0.03); reset(); }
    void reset() override { for (auto& c : ch) for (auto* b : { &c.hp1, &c.hp2, &c.lp1, &c.lp2, &c.pk }) b->reset(); }

    void process (float* L, float* R, int n) override
    {
        const int mi = (int) model->load();
        if (mi != lastModel) { applyModel (mi); lastModel = mi; }
        const auto& md = models()[(size_t) mi];
        mixS.setTargetValue (mix->load());
        driveS.setTargetValue (1.0f + (md.drive + drive->load()) * 6.0f);

        for (int i = 0; i < n; ++i)
        {
            const float g = driveS.getNextValue(), makeup = 1.0f / std::sqrt (g), m = mixS.getNextValue();
            float y[2];
            const float in[2] = { L[i], R[i] };
            for (int c = 0; c < 2; ++c)
            {
                auto& st = ch[(size_t) c];
                float v = st.pk.process (st.lp2.process (st.lp1.process (st.hp2.process (st.hp1.process (in[c])))));
                v = std::tanh (v * g) * makeup;
                if (md.crushLevels > 0.0f) v = std::round (v * md.crushLevels) / md.crushLevels;
                y[c] = v;
            }
            const float mid = 0.5f * (y[0] + y[1]), side = 0.5f * (y[0] - y[1]) * md.width;
            L[i] = blend (L[i], (mid + side) * md.level, m);
            R[i] = blend (R[i], (mid - side) * md.level, m);
        }
    }

private:
    struct Model { float hp, lp, pkF, pkG, pkQ, width, drive, crushLevels, level; };
    static const std::array<Model, 6>& models()
    {
        static const std::array<Model, 6> m { {
            { 450.0f, 3800.0f, 1900.0f, 9.0f, 2.0f, 0.0f, 0.9f, 12.0f, 0.8f },   // PC Speaker
            { 380.0f, 12000.0f, 2600.0f, 5.0f, 1.2f, 0.45f, 0.25f, 0.0f, 1.1f }, // Laptop
            { 650.0f, 4200.0f, 1500.0f, 6.0f, 1.5f, 0.0f, 0.5f, 0.0f, 1.1f },    // Flip Phone
            { 130.0f, 15000.0f, 3800.0f, 4.0f, 1.0f, 1.0f, 0.1f, 0.0f, 1.0f },   // Earbuds
            { 170.0f, 7200.0f, 1100.0f, 4.0f, 0.8f, 0.0f, 0.3f, 0.0f, 1.0f },    // CRT TV
            { 70.0f, 9000.0f, 120.0f, 6.0f, 0.9f, 0.7f, 0.4f, 0.0f, 0.85f },     // Boombox
        } };
        return m;
    }
    void applyModel (int i)
    {
        const auto& md = models()[(size_t) juce::jlimit (0, 5, i)];
        for (auto& c : ch)
        {
            c.hp1.set (1, sr, md.hp, 0.707); c.hp2.set (1, sr, md.hp, 0.707);
            c.lp1.set (0, sr, md.lp, 0.707); c.lp2.set (0, sr, md.lp, 0.707);
            c.pk.setPeak (sr, md.pkF, md.pkQ, md.pkG);
        }
    }

    struct Chan { Biquad hp1, hp2, lp1, lp2, pk; };
    std::atomic<float> *model, *drive, *mix;
    std::array<Chan, 2> ch;
    juce::SmoothedValue<float> driveS;
    int lastModel = -1;
};

} // namespace xfx
