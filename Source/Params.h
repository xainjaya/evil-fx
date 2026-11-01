#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter in the plugin lives here so the processor, editor and presets
// all agree on IDs. Modules run in this exact order:
//
//   1 Crimson Filter   2 Scorch Drive    3 CODEC.EXE       4 8BIT.EXE
//   5 DIALUP.EXE       6 LOBITRATE.EXE   7 PACKETLOSS.EXE  8 BUFFERING.EXE
//   9 CORRUPT.EXE     10 LAG.EXE        11 JITTER.EXE     12 Demon Chorus
//  13 Boiling Phaser  14 Trem / Pan     15 Hellfire Delay 16 Abyss Verb
//  17 SPEAKER.EXE  -> Master
//
// (The order of the declarations below doesn't matter; Rack.h holds the chain order.)
//
namespace Params
{
    using APF  = juce::AudioParameterFloat;
    using APB  = juce::AudioParameterBool;
    using APC  = juce::AudioParameterChoice;
    using Attr = juce::AudioParameterFloatAttributes;

    inline juce::ParameterID pid (const char* id) { return { id, 1 }; }

    inline juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    // ---- value -> text helpers ------------------------------------------------
    inline Attr pct()  { return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }); }
    inline Attr hz()   { return Attr().withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (v, v < 10.0f ? 2 : 0) + " Hz"; }); }
    inline Attr ms()   { return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }); }
    inline Attr db()   { return Attr().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }); }
    inline Attr bip()  { return Attr().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)); }); }
    inline Attr oct()  { return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " oct"; }); }
    inline Attr bits() { return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " bit"; }); }
    inline Attr kbps() { return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " kbps"; }); }
    inline Attr times(){ return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + "x"; }); }
    inline Attr plain(){ return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); }); }

    inline const juce::StringArray& delaySyncNames()
    {
        static const juce::StringArray n { "Free", "1/16", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1 Bar" };
        return n;
    }
    // length in quarter notes for each sync choice (index 0 = free)
    inline float delaySyncBeats (int i)
    {
        static const float b[] { 0.0f, 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
        return b[juce::jlimit (0, 9, i)];
    }

    inline const juce::StringArray& codecRateNames()
    {
        static const juce::StringArray n { "22 kHz", "16 kHz", "11 kHz", "8 kHz", "5.5 kHz" };
        return n;
    }
    inline double codecRateHz (int i)
    {
        static const double r[] { 22050.0, 16000.0, 11025.0, 8000.0, 5512.5 };
        return r[juce::jlimit (0, 4, i)];
    }
    inline const juce::StringArray& speakerNames()
    {
        static const juce::StringArray n { "PC Speaker", "Laptop", "Flip Phone", "Earbuds", "CRT TV", "Boombox" };
        return n;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        auto onOff = [&] (const char* id, const char* name, bool def = false)
        { p.push_back (std::make_unique<APB> (pid (id), name, def)); };
        auto flt = [&] (const char* id, const char* name, juce::NormalisableRange<float> r, float def, Attr a)
        { p.push_back (std::make_unique<APF> (pid (id), name, r, def, a)); };
        auto lin = [] (float lo, float hi) { return juce::NormalisableRange<float> (lo, hi); };

        // 1 Crimson Filter
        onOff ("filter_on", "Filter On");
        p.push_back (std::make_unique<APC> (pid ("filter_mode"), "Filter Mode", juce::StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));
        flt ("filter_cutoff",   "Filter Cutoff",    skewed (20.0f, 20000.0f, 1000.0f), 6000.0f, hz());
        flt ("filter_reso",     "Filter Resonance", skewed (0.5f, 10.0f, 2.0f), 0.9f, plain());
        flt ("filter_lforate",  "Filter LFO Rate",  skewed (0.01f, 10.0f, 1.0f), 0.3f, hz());
        flt ("filter_lfodepth", "Filter LFO Depth", lin (0.0f, 4.0f), 0.0f, oct());

        // 2 Scorch Drive (saturation)
        onOff ("gloss_on", "Scorch On");
        flt ("gloss_drive", "Scorch Drive", lin (0.0f, 24.0f), 6.0f, db());
        flt ("gloss_tone",  "Scorch Tone",  lin (-1.0f, 1.0f), 0.0f, bip());
        flt ("gloss_mix",   "Scorch Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // 3 Dial-Up (phone line / modem)
        onOff ("dial_on", "Dial-Up On");
        flt ("dial_band",    "Dial-Up Bandwidth", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("dial_noise",   "Dial-Up Line Noise", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("dial_crackle", "Dial-Up Crackle",   lin (0.0f, 1.0f), 0.2f, pct());
        flt ("dial_mix",     "Dial-Up Mix",       lin (0.0f, 1.0f), 1.0f, pct());
        onOff ("dial_modem", "Dial-Up Modem Squeal");

        // 4 8-Bit Crush
        onOff ("crush_on", "Crush On");
        flt ("crush_bits", "Crush Bits",       lin (1.0f, 16.0f), 8.0f, bits());
        flt ("crush_rate", "Crush Downsample", skewed (1.0f, 40.0f, 6.0f), 4.0f, times());
        flt ("crush_mix",  "Crush Mix",        lin (0.0f, 1.0f), 1.0f, pct());

        // 5 Buffering (stream stutter / dropout)
        onOff ("buf_on", "Buffering On");
        flt ("buf_chance",  "Buffering Chance",  lin (0.0f, 1.0f), 0.25f, pct());
        flt ("buf_slice",   "Buffering Slice",   skewed (20.0f, 500.0f, 120.0f), 120.0f, ms());
        flt ("buf_dropout", "Buffering Dropout", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("buf_mix",     "Buffering Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // 6 Lo-Bitrate (spectral codec artifacts)
        onOff ("lobit_on", "Lo-Bitrate On");
        flt ("lobit_kbps",  "Lo-Bitrate Kbps",  skewed (8.0f, 192.0f, 48.0f), 32.0f, kbps());
        flt ("lobit_swirl", "Lo-Bitrate Swirl", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("lobit_mix",   "Lo-Bitrate Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // CODEC.EXE — old-school sample rates + telephony / ADPCM codecs
        onOff ("codec_on", "Codec On");
        p.push_back (std::make_unique<APC> (pid ("codec_rate"), "Codec Sample Rate", codecRateNames(), 2));
        p.push_back (std::make_unique<APC> (pid ("codec_type"), "Codec Type", juce::StringArray { "8-bit PCM", "u-Law", "A-Law", "IMA ADPCM" }, 3));
        flt ("codec_alias", "Codec Aliasing", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("codec_mix",   "Codec Mix",      lin (0.0f, 1.0f), 1.0f, pct());

        // PACKETLOSS.EXE — VoIP packet loss + concealment
        onOff ("pkt_on", "Packet Loss On");
        flt ("pkt_loss", "Packet Loss Amount", lin (0.0f, 1.0f), 0.25f, pct());
        flt ("pkt_size", "Packet Size",        skewed (10.0f, 80.0f, 25.0f), 20.0f, ms());
        p.push_back (std::make_unique<APC> (pid ("pkt_conceal"), "Packet Concealment", juce::StringArray { "Robot", "Silence", "Noise" }, 0));
        flt ("pkt_mix",  "Packet Loss Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // CORRUPT.EXE — bit flips + reversed fragments, like a damaged download
        onOff ("corrupt_on", "Corrupt On");
        flt ("corrupt_amount",  "Corrupt Amount",  lin (0.0f, 1.0f), 0.3f, pct());
        flt ("corrupt_bits",    "Corrupt Bit Depth", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("corrupt_reverse", "Corrupt Reverse", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("corrupt_mix",     "Corrupt Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // LAG.EXE — lag spikes: audio slows to a crawl then fast-forwards to catch up
        onOff ("lag_on", "Lag On");
        flt ("lag_chance", "Lag Chance", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("lag_length", "Lag Length", skewed (80.0f, 1500.0f, 400.0f), 350.0f, ms());
        flt ("lag_depth",  "Lag Depth",  lin (0.0f, 1.0f), 0.7f, pct());
        flt ("lag_mix",    "Lag Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // JITTER.EXE — wow, flutter and clock drift
        onOff ("jit_on", "Jitter On");
        flt ("jit_wow",     "Jitter Wow",     lin (0.0f, 1.0f), 0.3f, pct());
        flt ("jit_flutter", "Jitter Flutter", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("jit_drift",   "Jitter Drift",   lin (0.0f, 1.0f), 0.2f, pct());
        flt ("jit_mix",     "Jitter Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // SPEAKER.EXE — cheap playback devices
        onOff ("spk_on", "Speaker On");
        p.push_back (std::make_unique<APC> (pid ("spk_model"), "Speaker Model", speakerNames(), 1));
        flt ("spk_drive", "Speaker Drive", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("spk_mix",   "Speaker Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // 7 Demon Chorus
        onOff ("chorus_on", "Chorus On");
        flt ("chorus_rate",  "Chorus Rate",     skewed (0.05f, 5.0f, 0.8f), 0.6f, hz());
        flt ("chorus_depth", "Chorus Depth",    lin (0.0f, 1.0f), 0.4f, pct());
        flt ("chorus_fb",    "Chorus Feedback", lin (-0.9f, 0.9f), 0.0f, bip());
        flt ("chorus_mix",   "Chorus Mix",      lin (0.0f, 1.0f), 0.5f, pct());

        // 8 Boiling Phaser
        onOff ("phaser_on", "Phaser On");
        flt ("phaser_rate",  "Phaser Rate",     skewed (0.02f, 5.0f, 0.5f), 0.3f, hz());
        flt ("phaser_depth", "Phaser Depth",    lin (0.0f, 1.0f), 0.6f, pct());
        flt ("phaser_fb",    "Phaser Feedback", lin (-0.9f, 0.9f), 0.4f, bip());
        flt ("phaser_mix",   "Phaser Mix",      lin (0.0f, 1.0f), 0.5f, pct());

        // 9 Trem / Pan
        onOff ("trem_on", "Trem On");
        flt ("trem_rate",  "Trem Rate",  skewed (0.1f, 20.0f, 3.0f), 4.0f, hz());
        flt ("trem_depth", "Trem Depth", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("trem_shape", "Trem Shape", lin (0.0f, 1.0f), 0.0f, pct());
        flt ("trem_pan",   "Trem Pan",   lin (0.0f, 1.0f), 0.0f, pct());

        // 10 Hellfire Delay
        onOff ("delay_on", "Delay On");
        p.push_back (std::make_unique<APC> (pid ("delay_sync"), "Delay Sync", delaySyncNames(), 0));
        flt ("delay_time", "Delay Time",     skewed (10.0f, 2000.0f, 400.0f), 350.0f, ms());
        flt ("delay_fb",   "Delay Feedback", lin (0.0f, 0.95f), 0.35f, pct());
        flt ("delay_tone", "Delay Tone",     lin (0.0f, 1.0f), 0.6f, pct());
        flt ("delay_mix",  "Delay Mix",      lin (0.0f, 1.0f), 0.3f, pct());
        onOff ("delay_pp", "Delay Ping-Pong", true);

        // 11 Abyss Verb
        onOff ("verb_on", "Reverb On");
        flt ("verb_size",  "Reverb Size",  lin (0.0f, 1.0f), 0.6f, pct());
        flt ("verb_damp",  "Reverb Damping", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("verb_width", "Reverb Width", lin (0.0f, 1.0f), 1.0f, pct());
        flt ("verb_mix",   "Reverb Mix",   lin (0.0f, 1.0f), 0.25f, pct());

        // Master
        flt ("in_gain",  "Input Gain",   lin (-24.0f, 24.0f), 0.0f, db());
        flt ("out_gain", "Output Gain",  lin (-24.0f, 24.0f), 0.0f, db());
        flt ("width",    "Stereo Width", lin (0.0f, 2.0f), 1.0f, pct());
        flt ("mix",      "Global Mix",   lin (0.0f, 1.0f), 1.0f, pct());
        onOff ("limiter", "Safety Limiter", true);

        return { p.begin(), p.end() };
    }
}
