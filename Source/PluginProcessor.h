#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Presets.h"
#include "dsp/NetModules.h"
#include "dsp/ExtraModules.h"
#include "Rack.h"

class EvilFXProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    // Order of modules in the chain (also the order of cards in the UI)
    enum ModuleIndex { Filter, EQ, Gloss, Pitch, Ring, TTS, Codec, Crush, Dial, VinylFx, LoBit, Packet, Buffer, Corrupt,
                       Lag, Jitter, GlitchFx, Gate, PumpFx, Chorus, Phaser, Trem, Spread, Delay, Verb, Comp, Speaker,
                       NumModules };

    EvilFXProcessor();
    ~EvilFXProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Evil FX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return (int) Presets::all().size(); }
    int getCurrentProgram() override { return (int) apvts.state.getProperty ("preset", 0); }
    void setCurrentProgram (int index) override { Presets::apply (apvts, index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // read by the UI
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, gainReductionDb { 0.0f }, currentBpm { 120.0f };
    int countActiveModules() const;

    // visualizer tap: costs nothing unless the editor switches it on
    void setVisualizerActive (bool on) { vizActive = on; }
    int pullVisualizerSamples (float* dest, int maxSamples);

private:
    void parameterChanged (const juce::String& id, float value) override;
    void processChunk (float* L, float* R, int n);
    void updateLatency();

    std::vector<std::unique_ptr<xfx::Slot>> slots;
    xfx::HaloDelay* delayModule = nullptr;
    xfx::Transport transport;          // shared with the beat-synced modules
    double freePpq = 0.0;              // our own beat clock while the host is stopped
    bool hostPlaying = false;

    juce::AudioBuffer<float> work, dry, dryDelay;
    int dryDelayPos = 0, maxBlock = 512;
    std::atomic<int> currentLatency { 0 };

    juce::SmoothedValue<float> inGainS, outGainS, widthS, mixS;
    std::atomic<float> *inGain, *outGain, *width, *mix, *limiterOn;
    float limEnv = 0.0f, limRelease = 0.999f;

    std::atomic<bool> vizActive { false };
    juce::AbstractFifo vizFifo { 16384 };
    std::vector<float> vizData = std::vector<float> (16384);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EvilFXProcessor)
};
