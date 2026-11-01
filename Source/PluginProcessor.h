#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Presets.h"
#include "dsp/NetModules.h"
#include "dsp/ProModules.h"
#include "Rack.h"

class EvilFXProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::ValueTree::Listener
{
public:
    // Fixed slot of each module (its default position in the chain). The order
    // the chain actually runs in is user-editable and lives in the "order" state.
    enum ModuleIndex { NGate, Transient, Tune, Filter, Wah, Talk, EQ, Gloss, FuzzFx, Pitch, Ring, FShift, VocoderFx, TTS,
                       Codec, Crush, Dial, VinylFx, LoBit, Packet, Buffer, Corrupt, Lag, Jitter, GlitchFx, Gate, PumpFx,
                       Swell, Chorus, FlangerFx, Phaser, Trem, Spread, RevDelay, Delay, BlurFx, Verb, Comp, Speaker, NumModules };

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

    juce::UndoManager undoManager { 30000, 40 };   // must be declared before apvts
    juce::AudioProcessorValueTreeState apvts;

    // read by the UI
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, gainReductionDb { 0.0f }, currentBpm { 120.0f };
    std::atomic<bool> sidechainConnected { false };
    int countActiveModules() const;

    // visualizer tap: costs nothing unless the editor switches it on
    void setVisualizerActive (bool on) { vizActive = on; }
    int pullVisualizerSamples (float* dest, int maxSamples);

    // ---- A/B compare: two full snapshots of the sound, switch between them
    int getActiveAB() const { return abActive; }
    void switchAB();                 // stash the current sound in this slot, load the other one
    void copyActiveToOther();        // make the other slot identical to what you hear now

    // the chain order the audio is using (message thread)
    juce::Array<int> getChainOrder() const { return Rack::getOrder (apvts.state); }

private:
    void parameterChanged (const juce::String& id, float value) override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override { pushOrderToAudio(); }
    void pushOrderToAudio();
    void processChunk (float* L, float* R, int n);
    void updateLatency();
    juce::ValueTree soundSnapshot();
    void loadSnapshot (const juce::ValueTree&);

    std::vector<std::unique_ptr<xfx::Slot>> slots;
    xfx::HaloDelay* delayModule = nullptr;
    xfx::Transport transport;          // shared with the beat-synced modules
    xfx::SideChain sideChain;          // shared with Pump / Noise Gate / Vocoder
    double freePpq = 0.0;              // our own beat clock while the host is stopped
    bool hostPlaying = false;

    // chain order: written on the message thread, picked up by the audio thread
    // with a try-lock (it never waits; worst case it uses last block's order)
    juce::SpinLock orderLock;
    std::array<int, Rack::numModules> pendingOrder {}, runOrder {};
    std::atomic<bool> orderDirty { false };

    juce::AudioBuffer<float> work, dry, dryDelay, side;
    int dryDelayPos = 0, maxBlock = 512;
    std::atomic<int> currentLatency { 0 };

    juce::SmoothedValue<float> inGainS, outGainS, widthS, mixS;
    std::atomic<float> *inGain, *outGain, *width, *mix, *limiterOn;
    float limEnv = 0.0f, limRelease = 0.999f;

    std::atomic<bool> vizActive { false };
    juce::AbstractFifo vizFifo { 16384 };
    std::vector<float> vizData = std::vector<float> (16384);

    juce::ValueTree abSlots[2];
    int abActive = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EvilFXProcessor)
};
