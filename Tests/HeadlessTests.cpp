// Headless checks for Evil FX:
//   1. Lo-Bitrate STFT reconstructs cleanly at high bitrate (latency + gain correct)
//   2. Every module alone, at defaults and at both extremes, stays finite and under the ceiling
//   3. Random fuzzing of every parameter, including toggling modules mid-stream
//   4. Every factory preset
//   5. Optional: renders the editor to a PNG  (EvilFXTests --snapshot out.png)

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
        { "lag", "lag_" }, { "jitter", "jit_" }, { "speaker", "spk_" }, { "chorus", "chorus_" }, { "phaser", "phaser_" }, { "trem", "trem_" }, { "delay", "delay_" }, { "verb", "verb_" } };

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
void testPresets()
{
    std::cout << "[4] Factory presets" << std::endl;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        const auto s = run (p, 3.0, 0.5f);
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
    std::cout << "  " << juce::String ("ALL 17 ON").paddedRight (' ', 18) << juce::String (all, 3) << " %" << std::endl;
}

void snapshot (const juce::File& file)
{
    juce::ScopedJuceInitialiser_GUI gui;
    EvilFXProcessor p;
    p.setPlayConfigDetails (2, 2, kRate, kBlock);
    p.prepareToPlay (kRate, kBlock);
    p.setCurrentProgram (5);   // a preset with retro + aero modules on
    setParam (p, "codec_on", 1); setParam (p, "verb_on", 1); setParam (p, "delay_on", 1); setParam (p, "delay_sync", 4);
    setParam (p, "corrupt_on", 1); setParam (p, "spk_on", 1);
    p.apvts.state.setProperty ("vizOn", true, nullptr);
    run (p, 1.0);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (EvilFXEditor::baseW, EvilFXEditor::baseH);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    file.deleteFile();
    juce::FileOutputStream os (file);
    juce::PNGImageFormat png;
    check (png.writeImageToStream (img, os), "snapshot written");
    std::cout << "[5] UI snapshot -> " << file.getFullPathName() << "" << std::endl;
}
} // namespace

int main (int argc, char** argv)
{
    if (argc > 2 && juce::String (argv[1]) == "--snapshot")
    {
        snapshot (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
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
    std::cout << (failures == 0 ? "\nALL TESTS PASSED\n" : "\nFAILURES: " + std::to_string (failures) + "\n");
    return failures == 0 ? 0 : 1;
}
