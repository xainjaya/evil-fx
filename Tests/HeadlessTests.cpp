// Headless checks for Evil FX:
//   1. Lo-Bitrate STFT reconstructs cleanly at high bitrate (latency + gain correct)
//   2. Every module alone, at defaults and at both extremes, stays finite and under the ceiling
//   3. Random fuzzing of every parameter, including toggling modules mid-stream
//   4. Every factory preset
//   5. Freq Shifter / Reverse Delay behave as advertised
//   6. Optional: renders the editor to a PNG  (EvilFXTests --snapshot out.png)

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr double kRate = 48000.0;
constexpr int kBlock = 512;
int failures = 0;

void check (bool ok, const juce::String& what)
{
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << "\n"; }
}

void setParam (EvilFXProcessor& p, const juce::String& id, float realValue)
{
    if (auto* rp = p.apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
}

void setNorm (EvilFXProcessor& p, juce::RangedAudioParameter* rp, float norm) { rp->setValueNotifyingHost (norm); (void) p; }

void resetAll (EvilFXProcessor& p)
{
    for (auto* prm : p.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm)) rp->setValueNotifyingHost (rp->getDefaultValue());
}

struct Stats { float peak = 0, rms = 0; bool finite = true; };

// Runs `seconds` of signal through the processor. Signal = mix of a sine sweep and noise bursts.
Stats run (EvilFXProcessor& p, double seconds, float level = 0.7f, juce::Random* rnd = nullptr, std::function<void (int)> perBlock = {})
{
    juce::AudioBuffer<float> buf (2, kBlock);
    juce::MidiBuffer midi;
    juce::Random r (7);
    Stats s; double sum = 0; long count = 0;
    const int blocks = (int) (seconds * kRate / kBlock);
    double ph = 0;
    for (int b = 0; b < blocks; ++b)
    {
        if (perBlock) perBlock (b);
        const int n = rnd ? 1 + rnd->nextInt (kBlock) : kBlock;   // random block sizes when fuzzing
        buf.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            const double f = 80.0 + 4000.0 * (0.5 + 0.5 * std::sin (ph * 0.00002));
            ph += 1.0;
            const float x = level * (0.7f * (float) std::sin (juce::MathConstants<double>::twoPi * f * ph / kRate)
                                   + 0.3f * (r.nextFloat() * 2.0f - 1.0f) * (((b / 20) % 2) ? 1.0f : 0.2f));
            buf.setSample (0, i, x);
            buf.setSample (1, i, x * 0.9f);
        }
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float v = buf.getSample (c, i);
                if (! std::isfinite (v)) { s.finite = false; continue; }
                s.peak = juce::jmax (s.peak, std::abs (v));
                sum += (double) v * v; ++count;
            }
    }
    s.rms = count ? (float) std::sqrt (sum / (double) count) : 0.0f;
    return s;
}

juce::String db (float g) { return juce::String (juce::Decibels::gainToDecibels (g, -120.0f), 1) + " dB"; }

//------------------------------------------------------------------------------
void testLoBitrateReconstruction()
{
    std::cout << "[1] Lo-Bitrate reconstruction" << std::endl;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    resetAll (p);
    setParam (p, "lobit_on", 1); setParam (p, "lobit_kbps", 192); setParam (p, "lobit_swirl", 0); setParam (p, "limiter", 0);
    check (p.getLatencySamples() == xfx::LoBitrate::kFFTSize, "latency reported when Lo-Bitrate is on");

    const int total = (int) kRate;
    std::vector<float> in ((size_t) total), out ((size_t) total);
    juce::AudioBuffer<float> buf (2, kBlock); juce::MidiBuffer midi;
    for (int i = 0; i < total; ++i) in[(size_t) i] = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 997.0 * i / kRate);
    for (int start = 0; start < total; start += kBlock)
    {
        const int n = juce::jmin (kBlock, total - start);
        buf.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { buf.setSample (0, i, in[(size_t) (start + i)]); buf.setSample (1, i, in[(size_t) (start + i)]); }
        p.processBlock (buf, midi);
        for (int i = 0; i < n; ++i) out[(size_t) (start + i)] = buf.getSample (0, i);
    }
    const int lat = xfx::LoBitrate::kFFTSize;
    double err = 0, ref = 0;
    for (int i = (int) (kRate * 0.2); i < total; ++i) { const double d = out[(size_t) i] - in[(size_t) (i - lat)]; err += d * d; ref += (double) in[(size_t) (i - lat)] * in[(size_t) (i - lat)]; }
    const double snr = 10.0 * std::log10 (ref / juce::jmax (1e-20, err));
    std::cout << "  SNR vs delayed input: " << juce::String (snr, 1) << " dB\n";
    check (snr > 30.0, "Lo-Bitrate at 192 kbps should be near-transparent (SNR > 30 dB)");

    setParam (p, "lobit_on", 0);
    check (p.getLatencySamples() == 0, "latency back to 0 when Lo-Bitrate is off");
}

//------------------------------------------------------------------------------
void testEachModule()
{
    std::cout << "[2] Each module alone: defaults / all-min / all-max" << std::endl;
    const std::vector<std::pair<juce::String, juce::String>> modules {
        { "filter", "filter_" }, { "gloss", "gloss_" }, { "dial", "dial_" }, { "crush", "crush_" }, { "buf", "buf_" },
        { "lobit", "lobit_" }, { "codec", "codec_" }, { "packet", "pkt_" }, { "corrupt", "corrupt_" },
        { "lag", "lag_" }, { "jitter", "jit_" }, { "speaker", "spk_" }, { "pitch", "pitch_" }, { "ring", "ring_" },
        { "glitch", "glitch_" }, { "gate", "gate_" }, { "comp", "comp_" }, { "eq", "eq_" }, { "vinyl", "vinyl_" },
        { "tts", "tts_" }, { "pump", "pump_" }, { "spread", "spread_" }, { "trans", "trans_" }, { "wah", "wah_" },
        { "swell", "swell_" }, { "blur", "blur_" }, { "chorus", "chorus_" }, { "phaser", "phaser_" }, { "trem", "trem_" }, { "delay", "delay_" }, { "verb", "verb_" },
        { "talk", "talk_" }, { "fshift", "fshift_" }, { "flange", "flange_" }, { "rdelay", "rdelay_" },
        { "ngate", "ngate_" }, { "fuzz", "fuzz_" }, { "tune", "tune_" }, { "voc", "voc_" } };

    for (auto& [name, prefix] : modules)
    {
        for (int mode = 0; mode < 3; ++mode)
        {
            EvilFXProcessor p;
            p.setPlayConfigDetails (2, 2, kRate, kBlock);
            p.prepareToPlay (kRate, kBlock);
            resetAll (p);
            if (mode > 0)
                for (auto* prm : p.getParameters())
                    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                        if (rp->getParameterID().startsWith (prefix)) rp->setValueNotifyingHost (mode == 1 ? 0.0f : 1.0f);
            setParam (p, prefix + "on", 1);
            if (mode == 2) setParam (p, "in_gain", 24);   // hammer it
            const auto s = run (p, 4.0);
            std::cout << "  " << name.paddedRight (' ', 7) << (mode == 0 ? "default" : mode == 1 ? "min    " : "max    ")
                      << "  peak " << db (s.peak).paddedRight (' ', 10) << " rms " << db (s.rms) << "\n";
            check (s.finite, name + " produced non-finite output");
            check (s.peak <= 0.967f, name + " exceeded the limiter ceiling");
        }
    }
}

//------------------------------------------------------------------------------
void testFuzz()
{
    std::cout << "[3] Fuzzing 300 random parameter states with random block sizes" << std::endl;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    juce::Random rnd (42);
    juce::Array<juce::RangedAudioParameter*> params;
    for (auto* prm : p.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (rp->getParameterID() != "limiter") params.add (rp);   // the limiter stays on: that's what we're testing

    float worst = 0; bool finite = true;
    for (int round = 0; round < 300; ++round)
    {
        for (auto* rp : params) if (rnd.nextFloat() < 0.5f) setNorm (p, rp, rnd.nextFloat());
        setParam (p, "limiter", 1);
        const auto s = run (p, 0.25, 0.3f + rnd.nextFloat(), &rnd, [&] (int b)
        {   // flip a couple of things mid-run, like a user would
            if (b % 7 == 3) setNorm (p, params[rnd.nextInt (params.size())], rnd.nextFloat());
        });
        worst = juce::jmax (worst, s.peak); finite = finite && s.finite;
    }
    std::cout << "  worst peak " << db (worst) << (finite ? ", all finite\n" : ", NON-FINITE!\n");
    check (finite, "fuzz produced non-finite output");
    check (worst <= 0.967f, "fuzz exceeded the limiter ceiling");

    // also make sure an unprepared-size block (bigger than prepare) is safe
    juce::AudioBuffer<float> big (2, 4096); big.clear(); juce::MidiBuffer m;
    for (int i = 0; i < 4096; ++i) big.setSample (0, i, 0.5f), big.setSample (1, i, -0.5f);
    p.processBlock (big, m);
    bool ok = true;
    for (int i = 0; i < 4096; ++i) ok = ok && std::isfinite (big.getSample (0, i));
    check (ok, "oversized host block handled");
}

//------------------------------------------------------------------------------
int presetFrom = 0, presetTo = 1 << 20;
double presetSeconds = 3.0;

void testPresets()
{
    std::cout << "[4] Factory presets" << std::endl;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    for (int i = presetFrom; i < juce::jmin (presetTo, p.getNumPrograms()); ++i)
    {
        p.setCurrentProgram (i);
        const auto s = run (p, presetSeconds, 0.5f);
        std::cout << "  " << p.getProgramName (i).paddedRight (' ', 24) << " modules " << p.countActiveModules()
                  << "  peak " << db (s.peak).paddedRight (' ', 10) << " rms " << db (s.rms) << "\n";
        check (s.finite && s.peak <= 0.967f, "preset " + p.getProgramName (i));
        check (s.rms > 1.0e-4f, "preset " + p.getProgramName (i) + " is silent");
    }

    // state round trip
    p.setCurrentProgram (3);
    juce::MemoryBlock mb; p.getStateInformation (mb);
    EvilFXProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
    check (q.countActiveModules() == p.countActiveModules(), "state save/restore");

    // projects from older versions (rack stored as 0/1 flags) keep the same effects in the rack
    for (auto legacy : { juce::String ("0001000000000000000000000010001"),             // v1.0: EQ, Delay, Speaker Sim
                         juce::String ("00001000000000000000000000000010001") })       // v1.1 preview: same three
    {
        juce::ValueTree old ("EvilFX");
        old.setProperty ("rack", legacy, nullptr);
        const auto flags = Rack::get (old);
        check (flags.length() == Rack::numModules, "legacy rack upgraded to the new length");
        check (Rack::contains (old, Rack::indexOf ("eq_on")), "legacy rack keeps EQ");
        check (Rack::contains (old, Rack::indexOf ("delay_on")), "legacy rack keeps Delay");
        check (Rack::contains (old, Rack::indexOf ("spk_on")), "legacy rack keeps Speaker Sim");
        check (flags.retainCharacters ("1").length() == 3, "legacy rack adds nothing new");
        Rack::upgrade (old);
        check (old.getProperty ("rack").toString() == "eq_on,delay_on,spk_on", "legacy rack rewritten as IDs");
    }

    // an order saved before some effect existed: unknown IDs dropped, missing ones slot in
    {
        juce::ValueTree t ("EvilFX");
        t.setProperty ("order", "verb_on,bogus_on,trans_on", nullptr);
        const auto o = Rack::getOrder (t);
        bool perm = o.size() == Rack::numModules;
        for (int i = 0; i < Rack::numModules; ++i) perm = perm && o.contains (i);
        check (perm, "partial order normalised to a full permutation");
        check (o.indexOf (Rack::indexOf ("verb_on")) < o.indexOf (Rack::indexOf ("trans_on")), "saved order kept");
    }
}

//------------------------------------------------------------------------------
// The newer effects do what they say: the frequency shifter moves a sine by the set
// number of Hz (and suppresses the other sideband), the reverse delay plays a
// rising sweep back falling.
void testNewEffects()
{
    std::cout << "[5] Freq Shifter sideband + Reverse Delay direction" << std::endl;
    auto goertzel = [] (const std::vector<float>& x, double f)
    {
        const double w = juce::MathConstants<double>::twoPi * f / kRate, cw = 2.0 * std::cos (w);
        double s1 = 0, s2 = 0;
        for (float v : x) { const double s0 = v + cw * s1 - s2; s2 = s1; s1 = s0; }
        return std::sqrt (s1 * s1 + s2 * s2 - cw * s1 * s2) / (double) x.size();
    };
    auto render = [] (EvilFXProcessor& p, std::function<float (int)> gen, int total)
    {
        std::vector<float> out ((size_t) total);
        juce::AudioBuffer<float> buf (2, kBlock); juce::MidiBuffer midi;
        for (int start = 0; start < total; start += kBlock)
        {
            const int n = juce::jmin (kBlock, total - start);
            buf.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i) { const float v = gen (start + i); buf.setSample (0, i, v); buf.setSample (1, i, v); }
            p.processBlock (buf, midi);
            for (int i = 0; i < n; ++i) out[(size_t) (start + i)] = buf.getSample (0, i);
        }
        return out;
    };

    for (float shift : { 200.0f, -300.0f })
    {
        EvilFXProcessor p;
        p.setPlayConfigDetails (2, 2, kRate, kBlock);
        p.prepareToPlay (kRate, kBlock);
        resetAll (p);
        setParam (p, "fshift_on", 1); setParam (p, "fshift_hz", shift); setParam (p, "fshift_mix", 1); setParam (p, "limiter", 0);
        auto y = render (p, [] (int i) { return 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / kRate); }, (int) kRate);
        y.erase (y.begin(), y.begin() + (int) (kRate * 0.25));
        const double want = goertzel (y, 1000.0 + shift), mirror = goertzel (y, 1000.0 - shift), orig = goertzel (y, 1000.0);
        const double rej = 20.0 * std::log10 (want / juce::jmax (1e-12, juce::jmax (mirror, orig)));
        std::cout << "  shift " << juce::String (shift, 0) << " Hz: wanted vs mirror/original " << juce::String (rej, 1) << " dB\n";
        check (rej > 30.0, "freq shifter should land on 1000 + shift and reject the rest (> 30 dB)");
    }

    {   // reverse delay, fully wet, no feedback: feed a rising sweep, the output must fall inside each chunk
        EvilFXProcessor p;
        p.setPlayConfigDetails (2, 2, kRate, kBlock);
        p.prepareToPlay (kRate, kBlock);
        resetAll (p);
        setParam (p, "rdelay_on", 1); setParam (p, "rdelay_sync", 0); setParam (p, "rdelay_time", 500);
        setParam (p, "rdelay_fb", 0); setParam (p, "rdelay_mix", 1); setParam (p, "limiter", 0);
        double ph = 0;
        auto y = render (p, [&ph] (int i)
        {
            const double t = std::fmod (i / kRate, 0.5);                  // 300 Hz -> 3 kHz every 0.5 s
            ph += juce::MathConstants<double>::twoPi * (300.0 + 5400.0 * t) / kRate;
            return 0.4f * (float) std::sin (ph);
        }, (int) (kRate * 3));
        // zero-crossing rate over short windows: count windows where it falls vs rises
        int falling = 0, rising = 0, prev = -1;
        const int win = (int) (kRate * 0.02);
        for (int start = (int) kRate; start + win < (int) y.size(); start += win)
        {
            int zc = 0;
            for (int i = start + 1; i < start + win; ++i) zc += (y[(size_t) i - 1] < 0) != (y[(size_t) i] < 0);
            if (prev >= 0) { if (zc < prev - 1) ++falling; else if (zc > prev + 1) ++rising; }
            prev = zc;
        }
        std::cout << "  reverse delay: pitch falling in " << falling << " windows, rising in " << rising << "\n";
        check (falling > rising * 2, "reverse delay should play a rising sweep back falling");
    }
}

//------------------------------------------------------------------------------
void prepare (EvilFXProcessor& p) { p.setPlayConfigDetails (2, 2, kRate, kBlock); p.prepareToPlay (kRate, kBlock); resetAll (p); }

std::vector<float> renderMono (EvilFXProcessor& p, std::function<float (int)> gen, int total)
{
    std::vector<float> out ((size_t) total);
    juce::AudioBuffer<float> buf (2, kBlock); juce::MidiBuffer midi;
    for (int start = 0; start < total; start += kBlock)
    {
        const int n = juce::jmin (kBlock, total - start);
        buf.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { const float v = gen (start + i); buf.setSample (0, i, v); buf.setSample (1, i, v); }
        p.processBlock (buf, midi);
        for (int i = 0; i < n; ++i) out[(size_t) (start + i)] = buf.getSample (0, i);
    }
    return out;
}

// frequency from zero crossings over the tail of the signal
double zcFreq (const std::vector<float>& y, int from)
{
    int first = -1, last = -1, count = 0;
    for (int i = from + 1; i < (int) y.size(); ++i)
        if (y[(size_t) i - 1] <= 0.0f && y[(size_t) i] > 0.0f)
        {
            if (first < 0) first = i; else ++count;
            last = i;
        }
    return count > 0 ? count * kRate / (double) (last - first) : 0.0;
}

void testWorkflow()
{
    std::cout << "[7] Chain order, undo, A/B, sidechain, Auto-Tune, Vocoder" << std::endl;
    auto sine = [] (double f, float a = 0.4f) { return [f, a] (int i) { return a * (float) std::sin (juce::MathConstants<double>::twoPi * f * i / kRate); }; };

    {   // the order the chain runs in changes the sound: fuzz -> reverse delay vs reverse delay -> fuzz
        auto render = [&] (bool fuzzFirst)
        {
            EvilFXProcessor p; prepare (p);
            setParam (p, "fuzz_on", 1); setParam (p, "fuzz_drive", 30);
            setParam (p, "rdelay_on", 1); setParam (p, "rdelay_mix", 1); setParam (p, "rdelay_sync", 0); setParam (p, "rdelay_time", 300);
            setParam (p, "limiter", 0);
            auto o = Rack::defaultOrder();
            if (! fuzzFirst) o.move (o.indexOf (Rack::indexOf ("rdelay_on")), 0);
            Rack::setOrder (p.apvts.state, o);
            return renderMono (p, [] (int i) { return 0.5f * (float) std::sin (i * 0.05) * (float) ((i / 4000) % 2); }, (int) kRate);
        };
        const auto a = render (true), b = render (false);
        double diff = 0; for (size_t i = 0; i < a.size(); ++i) diff += std::abs (a[i] - b[i]);
        std::cout << "  order A vs B mean difference " << juce::String (diff / (double) a.size(), 4) << "\n";
        check (diff / (double) a.size() > 0.01, "changing the chain order changes the sound");

        EvilFXProcessor p; prepare (p);
        auto o = Rack::defaultOrder(); o.move (5, 30);
        Rack::setOrder (p.apvts.state, o);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        EvilFXProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.getChainOrder() == o, "chain order survives save/load");
    }

    {   // undo / redo a knob move and a rack change
        EvilFXProcessor p; prepare (p);
        p.undoManager.clearUndoHistory();
        p.undoManager.beginNewTransaction();
        setParam (p, "verb_mix", 0.9f);
        p.apvts.copyState();   // flushes the change into the tree (+ history)
        p.undoManager.beginNewTransaction();
        Rack::set (p.apvts.state, Rack::indexOf ("verb_on"), true, &p.undoManager);
        check (Rack::contains (p.apvts.state, Rack::indexOf ("verb_on")), "rack add");
        p.undoManager.undo();
        check (! Rack::contains (p.apvts.state, Rack::indexOf ("verb_on")), "undo rack add");
        p.undoManager.undo();
        const float v = p.apvts.getParameter ("verb_mix")->convertFrom0to1 (p.apvts.getParameter ("verb_mix")->getValue());
        check (std::abs (v - 0.25f) < 0.01f, "undo knob move (got " + juce::String (v) + ")");
        p.undoManager.redo();
        const float r = p.apvts.getParameter ("verb_mix")->convertFrom0to1 (p.apvts.getParameter ("verb_mix")->getValue());
        check (std::abs (r - 0.9f) < 0.01f, "redo knob move");
    }

    {   // A/B: two independent sounds, survive save/load
        EvilFXProcessor p; prepare (p);
        p.setCurrentProgram (5);
        const auto nameA = p.apvts.state.getProperty ("presetName").toString();
        p.switchAB();                       // B starts as a copy of A
        p.setCurrentProgram (12);
        const auto nameB = p.apvts.state.getProperty ("presetName").toString();
        p.switchAB();
        check (p.apvts.state.getProperty ("presetName").toString() == nameA, "A/B: back to A");
        juce::MemoryBlock mb; p.getStateInformation (mb);
        EvilFXProcessor q; prepare (q); q.setStateInformation (mb.getData(), (int) mb.getSize());
        q.switchAB();
        check (q.apvts.state.getProperty ("presetName").toString() == nameB, "A/B: B restored from saved state");
    }

    {   // sidechain pump: ducks when the sidechain hits, not otherwise
        EvilFXProcessor p;
        auto layout = p.getBusesLayout();
        layout.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();
        check (p.setBusesLayout (layout), "sidechain bus can be enabled");
        p.setRateAndBufferSizeDetails (kRate, kBlock);
        p.prepareToPlay (kRate, kBlock);
        resetAll (p);
        setParam (p, "pump_on", 1); setParam (p, "pump_mode", 1); setParam (p, "pump_depth", 1); setParam (p, "limiter", 0);
        juce::AudioBuffer<float> buf (4, kBlock); juce::MidiBuffer midi;
        double loudQuiet[2] = { 0, 0 };
        for (int b = 0; b < 200; ++b)
        {
            const bool kick = (b / 20) % 2 == 1;
            for (int i = 0; i < kBlock; ++i)
            {
                const float x = 0.3f * (float) std::sin (0.03 * (b * kBlock + i));
                buf.setSample (0, i, x); buf.setSample (1, i, x);
                const float k = kick ? 0.8f : 0.0f;
                buf.setSample (2, i, k); buf.setSample (3, i, k);
            }
            p.processBlock (buf, midi);
            if (b % 20 > 5)
                for (int i = 0; i < kBlock; ++i) loudQuiet[kick ? 1 : 0] += std::abs (buf.getSample (0, i));
        }
        std::cout << "  sidechain pump: level without kick " << juce::String (loudQuiet[0], 1) << ", with kick " << juce::String (loudQuiet[1], 1) << "\n";
        check (p.sidechainConnected.load(), "processor sees the sidechain");
        check (loudQuiet[1] < loudQuiet[0] * 0.2, "sidechain pump ducks under the sidechain signal");
    }

    for (auto [inHz, scale, wantHz] : { std::tuple<double, int, double> { 452.0, 0, 440.0 },     // chromatic: nearest is A4
                                        std::tuple<double, int, double> { 285.0, 1, 293.66 },    // C major: D4
                                        std::tuple<double, int, double> { 320.0, 1, 329.63 } })  // C major: E4
    {
        EvilFXProcessor p; prepare (p);
        setParam (p, "tune_on", 1); setParam (p, "tune_scale", (float) scale); setParam (p, "tune_speed", 0); setParam (p, "limiter", 0);
        const auto y = renderMono (p, sine (inHz), (int) kRate);
        const double f = zcFreq (y, (int) (kRate * 0.4));
        std::cout << "  auto-tune " << juce::String (inHz, 1) << " Hz -> " << juce::String (f, 1) << " Hz (want " << juce::String (wantHz, 1) << ")\n";
        check (std::abs (1200.0 * std::log2 (f / wantHz)) < 15.0, "auto-tune lands within 15 cents of the target note");
    }

    {
        EvilFXProcessor p; prepare (p);
        setParam (p, "voc_on", 1); setParam (p, "voc_mix", 1); setParam (p, "limiter", 0);
        juce::Random r (3);
        const auto y = renderMono (p, [&r] (int i) { return (r.nextFloat() - 0.5f) * 0.6f * (float) (0.5 + 0.5 * std::sin (i * 0.0005)); }, (int) kRate);
        double e = 0; for (auto v : y) e += v * v;
        const double rms = std::sqrt (e / (double) y.size());
        std::cout << "  vocoder rms " << db ((float) rms) << "\n";
        check (rms > 0.01, "vocoder produces sound from a chord carrier");
    }
}

//------------------------------------------------------------------------------
// Drives the real editor: drag the first card onto the third slot, check the
// processor's chain order followed, then undo it.
void testEditorReorder()
{
    std::cout << "[9] Editor: drag to reorder + undo" << std::endl;
    EvilFXProcessor p; prepare (p);
    for (auto* id : { "trans_on", "eq_on", "fuzz_on", "chorus_on", "verb_on" }) setParam (p, id, 1);
    Rack::syncFromParams (p.apvts);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (EvilFXEditor::baseW, EvilFXEditor::baseH);

    auto visibleCards = [&ed]
    {
        juce::Array<aero::ModuleCard*> v;
        std::function<void (juce::Component&)> walk = [&] (juce::Component& c)
        {
            for (auto* ch : c.getChildren())
            {
                if (auto* mc = dynamic_cast<aero::ModuleCard*> (ch)) { if (mc->isVisible() && mc->getSpec().powerId != nullptr) v.add (mc); }
                else walk (*ch);
            }
        };
        walk (*ed);
        std::sort (v.begin(), v.end(), [] (auto* a, auto* b) { return a->getY() != b->getY() ? a->getY() < b->getY() : a->getX() < b->getX(); });
        return v;
    };
    auto rackIds = [&p]
    {
        juce::StringArray ids;
        const auto rack = Rack::get (p.apvts.state);
        for (int m : p.getChainOrder()) if (rack[m] == '1') ids.add (Rack::onId (m));
        return ids.joinIntoString (",");
    };

    const auto before = rackIds();
    auto cards = visibleCards();
    check (cards.size() == 5, "five cards visible");
    if (cards.size() == 5)
    {
        auto* first = cards[0];
        const auto target = cards[2]->getPosition();
        p.undoManager.beginNewTransaction();
        first->onDragStart();
        for (int k = 1; k <= 10; ++k)   // glide across like a mouse would
            first->onDragMove (first->getPosition() + (target - first->getPosition()) * k / 10);
        first->onDragEnd();
        const auto after = rackIds();
        std::cout << "  before: " << before << "\n  after:  " << after << "\n";
        check (after == "eq_on,fuzz_on,trans_on,chorus_on,verb_on", "dragging card 1 onto slot 3 reorders the chain");
        auto now = visibleCards();
        check (now.size() == 5 && juce::String (now[2]->getSpec().powerId) == "trans_on", "cards laid out in the new order");
        p.undoManager.undo();
        check (rackIds() == before, "undo puts the order back");
    }
}

// CPU cost of each module on its own, as % of one core in real time (48 kHz stereo, 512 blocks)
void bench()
{
    std::cout << "CPU benchmark (% of one core, 48 kHz stereo)" << std::endl;
    auto measure = [] (std::function<void (EvilFXProcessor&)> setup) -> double
    {
        EvilFXProcessor p;
        p.setPlayConfigDetails (2, 2, kRate, kBlock);
        p.prepareToPlay (kRate, kBlock);
        resetAll (p);
        setup (p);
        juce::AudioBuffer<float> buf (2, kBlock); juce::MidiBuffer midi; juce::Random r (1);
        const int blocks = (int) (20.0 * kRate / kBlock);
        double best = 1e9;
        for (int rep = 0; rep < 3; ++rep)
        {
            const auto t0 = juce::Time::getHighResolutionTicks();
            for (int b = 0; b < blocks; ++b)
            {
                for (int c = 0; c < 2; ++c) for (int i = 0; i < kBlock; ++i) buf.setSample (c, i, (r.nextFloat() - 0.5f) * 0.5f);
                p.processBlock (buf, midi);
            }
            best = juce::jmin (best, juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0));
        }
        return 100.0 * best / 20.0;
    };
    const double base = measure ([] (EvilFXProcessor&) {});
    std::cout << "  " << juce::String ("(empty chain)").paddedRight (' ', 18) << juce::String (base, 3) << " %" << std::endl;
    for (int i = 0; i < Rack::numModules; ++i)
    {
        const double t = measure ([i] (EvilFXProcessor& p) { setParam (p, Rack::onId (i), 1); });
        std::cout << "  " << juce::String (Rack::onId (i)).upToLastOccurrenceOf ("_on", false, false).paddedRight (' ', 18)
                  << juce::String (juce::jmax (0.0, t - base), 3) << " %" << std::endl;
    }
    const double all = measure ([] (EvilFXProcessor& p) { for (int i = 0; i < Rack::numModules; ++i) setParam (p, Rack::onId (i), 1); });
    std::cout << "  " << juce::String ("ALL ON").paddedRight (' ', 18) << juce::String (all, 3) << " %" << std::endl;
}

template <typename T> T* findChild (juce::Component& root)
{
    if (auto* t = dynamic_cast<T*> (&root)) return t;
    for (auto* c : root.getChildren()) if (auto* t = findChild<T> (*c)) return t;
    return nullptr;
}

void snapshot (const juce::File& file, int theme, bool withBrowser = false)
{
    juce::ScopedJuceInitialiser_GUI gui;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    p.setCurrentProgram (9);   // a preset with retro + aero modules on
    setParam (p, "codec_on", 1); setParam (p, "verb_on", 1); setParam (p, "delay_on", 1); setParam (p, "delay_sync", 4);
    setParam (p, "corrupt_on", 1); setParam (p, "spk_on", 1); setParam (p, "glitch_on", 1); setParam (p, "gate_on", 1);
    setParam (p, "chorus_on", 1); setParam (p, "comp_on", 1); setParam (p, "pitch_on", 1);
    setParam (p, "vinyl_on", 1); setParam (p, "eq_on", 1); setParam (p, "blur_on", 1); setParam (p, "swell_on", 1);
    setParam (p, "trans_on", 1); setParam (p, "wah_on", 1);
    setParam (p, "talk_on", 1); setParam (p, "fshift_on", 1); setParam (p, "flange_on", 1); setParam (p, "rdelay_on", 1);
    setParam (p, "lobit_on", 0); setParam (p, "jit_on", 0);
    p.apvts.state.setProperty ("vizOn", true, nullptr);
    p.apvts.state.setProperty ("theme", theme, nullptr);
    run (p, 1.0);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (EvilFXEditor::baseW, EvilFXEditor::baseH);
    if (withBrowser)
        if (auto* b = findChild<aero::PresetBrowser> (*ed))
        {
            Presets::saveFavourites ({ Presets::favKeyFactory ("Hard Tune (C Major)"), Presets::favKeyFactory ("Classic Vocoder") });
            b->refresh();
            b->setVisible (true);
            b->toFront (false);
        }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    file.deleteFile();
    juce::FileOutputStream os (file);
    juce::PNGImageFormat png;
    check (png.writeImageToStream (img, os), "snapshot written");
    std::cout << "[8] UI snapshot -> " << file.getFullPathName() << "" << std::endl;
}
} // namespace

// renders the smiley mascot on its own (used for docs/icon.png, the app icon)
void renderIcon (const juce::File& file, int size)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::Image img (juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g (img);
        const float r = (float) size / 3.4f;   // cap + brim span about 3.2r wide, 2.4r tall
        aero::drawSmiley (g, { (float) size * 0.42f, (float) size * 0.58f }, r);
    }
    file.deleteFile();
    juce::FileOutputStream os (file);
    juce::PNGImageFormat png;
    check (png.writeImageToStream (img, os), "icon written");
    std::cout << "icon -> " << file.getFullPathName() << std::endl;
}

int main (int argc, char** argv)
{
    if (argc > 2 && juce::String (argv[1]) == "--mascot")   // the animation states, side by side
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const int cell = 220;
        const std::pair<float, float> states[] { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.5f }, { 0.0f, 1.0f } };
        juce::Image img (juce::Image::ARGB, cell * 4, cell, true);
        {
            juce::Graphics g (img);
            g.fillAll (juce::Colour (0xffeaf2ff));
            for (int k = 0; k < 4; ++k)
                aero::drawSmiley (g, { cell * k + cell * 0.42f, cell * 0.6f }, cell / 3.6f, states[k].first, states[k].second);
        }
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "--icon")
    {
        renderIcon (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]), argc > 3 ? juce::String (argv[3]).getIntValue() : 512);
        return failures;
    }
    if (argc > 2 && juce::String (argv[1]) == "--snapshot")
    {
        snapshot (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]), argc > 3 ? juce::String (argv[3]).getIntValue() : 0,
                  argc > 4 && juce::String (argv[4]) == "browser");
        return failures;
    }
    if (argc > 3 && juce::String (argv[1]) == "--module")   // debugging aid: --module prefix mode(0 default,1 min,2 max)
    {
        juce::ScopedJuceInitialiser_GUI g;
        const juce::String prefix (argv[2]);
        const int mode = juce::String (argv[3]).getIntValue();
        EvilFXProcessor p;
        p.setPlayConfigDetails (2, 2, kRate, kBlock);
        p.prepareToPlay (kRate, kBlock);
        resetAll (p);
        if (mode > 0)
            for (auto* prm : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                    if (rp->getParameterID().startsWith (prefix)) rp->setValueNotifyingHost (mode == 1 ? 0.0f : 1.0f);
        setParam (p, prefix + "on", 1);
        const auto st = run (p, argc > 4 ? juce::String (argv[4]).getDoubleValue() : 1.0);
        std::cout << prefix << " peak " << db (st.peak) << " rms " << db (st.rms) << std::endl;
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "--presets")   // debugging aid: --presets from to [seconds]
    {
        juce::ScopedJuceInitialiser_GUI g;
        presetFrom = juce::String (argv[2]).getIntValue(); presetTo = juce::String (argv[3]).getIntValue();
        if (argc > 4) presetSeconds = juce::String (argv[4]).getDoubleValue();
        testPresets();
        return failures;
    }
    if (argc > 1 && juce::String (argv[1]) == "--bench")
    {
        juce::ScopedJuceInitialiser_GUI g;
        bench();
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI gui;   // APVTS/ValueTree want a message manager
    testLoBitrateReconstruction();
    testEachModule();
    testFuzz();
    testPresets();
    testNewEffects();
    testWorkflow();
    testEditorReorder();
    std::cout << (failures == 0 ? "\nALL TESTS PASSED\n" : "\nFAILURES: " + std::to_string (failures) + "\n");
    return failures == 0 ? 0 : 1;
}
