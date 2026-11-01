#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace xfx;

EvilFXProcessor::EvilFXProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "EvilFX", Params::createLayout())
{
    auto add = [this] (auto module, const char* onId)
    {
        slots.push_back (std::make_unique<Slot> (std::move (module), raw (apvts, onId)));
    };
    add (std::make_unique<CrimsonFilter>  (apvts), "filter_on");
    add (std::make_unique<ScorchDrive>    (apvts), "gloss_on");
    add (std::make_unique<CodecSim>       (apvts), "codec_on");
    add (std::make_unique<BitCrush>       (apvts), "crush_on");
    add (std::make_unique<DialUp>         (apvts), "dial_on");
    add (std::make_unique<LoBitrate>      (apvts), "lobit_on");
    add (std::make_unique<PacketLoss>     (apvts), "pkt_on");
    add (std::make_unique<Buffering>      (apvts), "buf_on");
    add (std::make_unique<xfx::Corrupt>      (apvts), "corrupt_on");
    add (std::make_unique<LagSpike>       (apvts), "lag_on");
    add (std::make_unique<xfx::Jitter>      (apvts), "jit_on");
    add (std::make_unique<DemonChorus>    (apvts), "chorus_on");
    add (std::make_unique<BoilingPhaser>  (apvts), "phaser_on");
    add (std::make_unique<TremPan>        (apvts), "trem_on");
    add (std::make_unique<HellfireDelay>  (apvts), "delay_on");
    add (std::make_unique<AbyssVerb>      (apvts), "verb_on");
    add (std::make_unique<SpeakerSim>     (apvts), "spk_on");
    jassert ((int) slots.size() == NumModules && NumModules == Rack::numModules);
    for (int i = 0; i < NumModules; ++i) jassert (juce::String (Rack::onId (i)).isNotEmpty());
    delayModule = static_cast<HellfireDelay*> (slots[Delay]->get());

    inGain    = raw (apvts, "in_gain");
    outGain   = raw (apvts, "out_gain");
    width     = raw (apvts, "width");
    mix       = raw (apvts, "mix");
    limiterOn = raw (apvts, "limiter");

    apvts.addParameterListener ("lobit_on", this);
    apvts.state.setProperty ("preset", 0, nullptr);
    Rack::syncFromParams (apvts);
    updateLatency();
}

EvilFXProcessor::~EvilFXProcessor()
{
    apvts.removeParameterListener ("lobit_on", this);
}

bool EvilFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void EvilFXProcessor::parameterChanged (const juce::String&, float) { updateLatency(); }

void EvilFXProcessor::updateLatency()
{
    const int lat = apvts.getRawParameterValue ("lobit_on")->load() > 0.5f ? LoBitrate::kFFTSize : 0;
    currentLatency = lat;
    setLatencySamples (lat);
}

void EvilFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = juce::jmax (32, juce::jmin (samplesPerBlock, 2048));
    for (auto& s : slots) s->prepare (sampleRate, maxBlock);

    work.setSize (2, maxBlock);
    dry.setSize (2, maxBlock);
    dryDelay.setSize (2, LoBitrate::kFFTSize);
    dryDelay.clear();
    dryDelayPos = 0;

    for (auto* s : { &inGainS, &outGainS, &widthS, &mixS }) s->reset (sampleRate, 0.03);
    inGainS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inGain->load()));
    outGainS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outGain->load()));
    widthS.setCurrentAndTargetValue (width->load());
    mixS.setCurrentAndTargetValue (mix->load());

    limEnv = 0.0f;
    limRelease = (float) std::exp (-1.0 / (sampleRate * 0.12));
    updateLatency();
}

void EvilFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    const int total = buffer.getNumSamples();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                delayModule->setBpm (*bpm);

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        auto* L = work.getWritePointer (0);
        auto* R = work.getWritePointer (1);

        if (numIn > 0)
        {
            std::copy_n (buffer.getReadPointer (0, start), n, L);
            std::copy_n (buffer.getReadPointer (numIn > 1 ? 1 : 0, start), n, R);
        }
        else { std::fill_n (L, n, 0.0f); std::fill_n (R, n, 0.0f); }

        processChunk (L, R, n);

        if (numOut == 1)
        {
            auto* o = buffer.getWritePointer (0, start);
            for (int i = 0; i < n; ++i) o[i] = 0.5f * (L[i] + R[i]);
        }
        else if (numOut >= 2)
        {
            std::copy_n (L, n, buffer.getWritePointer (0, start));
            std::copy_n (R, n, buffer.getWritePointer (1, start));
        }
    }
    for (int c = 2; c < numOut; ++c) buffer.clear (c, 0, total);
}

void EvilFXProcessor::processChunk (float* L, float* R, int n)
{
    // --- input gain, keep a dry copy for the global mix
    inGainS.setTargetValue (juce::Decibels::decibelsToGain (inGain->load()));
    for (int i = 0; i < n; ++i) { const float g = inGainS.getNextValue(); L[i] *= g; R[i] *= g; }
    auto* dl = dry.getWritePointer (0); auto* dr = dry.getWritePointer (1);
    std::copy_n (L, n, dl); std::copy_n (R, n, dr);

    // --- the chain
    for (auto& s : slots) s->process (L, R, n);

    // --- delay the dry signal to line up with the Lo-Bitrate latency
    if (currentLatency.load() > 0)
    {
        auto* yl = dryDelay.getWritePointer (0); auto* yr = dryDelay.getWritePointer (1);
        const int len = dryDelay.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            std::swap (dl[i], yl[dryDelayPos]);
            std::swap (dr[i], yr[dryDelayPos]);
            if (++dryDelayPos >= len) dryDelayPos = 0;
        }
    }

    // --- width, global mix, output gain
    widthS.setTargetValue (width->load());
    mixS.setTargetValue (mix->load());
    outGainS.setTargetValue (juce::Decibels::decibelsToGain (outGain->load()));
    for (int i = 0; i < n; ++i)
    {
        const float wdt = widthS.getNextValue(), m = mixS.getNextValue(), g = outGainS.getNextValue();
        const float mid = 0.5f * (L[i] + R[i]), side = 0.5f * (L[i] - R[i]) * wdt;
        const float wl = mid + side, wr = mid - side;
        L[i] = (dl[i] + (wl - dl[i]) * m) * g;
        R[i] = (dr[i] + (wr - dr[i]) * m) * g;
    }

    // --- safety: if anything ever goes non-finite, reset the chain instead of screaming
    bool finite = true;
    for (int i = 0; i < n && finite; ++i) finite = std::isfinite (L[i]) && std::isfinite (R[i]);
    if (! finite)
    {
        for (auto& s : slots) s->reset();
        std::fill_n (L, n, 0.0f); std::fill_n (R, n, 0.0f);
        limEnv = 0.0f;
    }

    // --- safety limiter: peak follower to -0.3 dBFS + soft clip for anything that slips past
    float peakL = 0.0f, peakR = 0.0f, maxGr = 0.0f;
    if (limiterOn->load() > 0.5f)
    {
        static constexpr float ceiling = 0.966f, knee = 0.8f;
        auto clip = [] (float y)   // transparent below the knee, smooth into the ceiling above it
        {
            const float a = std::abs (y);
            if (a <= knee) return y;
            return std::copysign (knee + (ceiling - knee) * std::tanh ((a - knee) / (ceiling - knee)), y);
        };
        for (int i = 0; i < n; ++i)
        {
            const float pk = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            limEnv = pk > limEnv ? pk : limEnv * limRelease + pk * (1.0f - limRelease);
            const float g = limEnv > ceiling ? ceiling / limEnv : 1.0f;
            maxGr = juce::jmax (maxGr, 1.0f - g);
            L[i] = clip (L[i] * g);
            R[i] = clip (R[i] * g);
            peakL = juce::jmax (peakL, std::abs (L[i]));
            peakR = juce::jmax (peakR, std::abs (R[i]));
        }
    }
    else
    {
        for (int i = 0; i < n; ++i) { peakL = juce::jmax (peakL, std::abs (L[i])); peakR = juce::jmax (peakR, std::abs (R[i])); }
    }

    if (vizActive.load())   // lock-free single-producer push; drops samples if the UI falls behind
    {
        int s1, n1, s2, n2;
        vizFifo.prepareToWrite (n, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) vizData[(size_t) (s1 + i)] = 0.5f * (L[i] + R[i]);
        for (int i = 0; i < n2; ++i) vizData[(size_t) (s2 + i)] = 0.5f * (L[n1 + i] + R[n1 + i]);
        vizFifo.finishedWrite (n1 + n2);
    }

    meterL = juce::jmax (peakL, meterL.load() * 0.94f);
    meterR = juce::jmax (peakR, meterR.load() * 0.94f);
    gainReductionDb = juce::Decibels::gainToDecibels (1.0f - maxGr);
}

int EvilFXProcessor::countActiveModules() const
{
    int c = 0;
    for (int i = 0; i < Rack::numModules; ++i) c += apvts.getRawParameterValue (Rack::onId (i))->load() > 0.5f ? 1 : 0;
    return c;
}

int EvilFXProcessor::pullVisualizerSamples (float* dest, int maxSamples)
{
    int s1, n1, s2, n2;
    vizFifo.prepareToRead (juce::jmin (maxSamples, vizFifo.getNumReady()), s1, n1, s2, n2);
    std::copy_n (vizData.data() + s1, n1, dest);
    std::copy_n (vizData.data() + s2, n2, dest + n1);
    vizFifo.finishedRead (n1 + n2);
    return n1 + n2;
}

const juce::String EvilFXProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) Presets::all().size()) ? Presets::all()[(size_t) index].name : "";
}

void EvilFXProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void EvilFXProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            if (! Rack::isValid (apvts.state)) Rack::syncFromParams (apvts);
        }
    updateLatency();
}

juce::AudioProcessorEditor* EvilFXProcessor::createEditor() { return new EvilFXEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EvilFXProcessor(); }
