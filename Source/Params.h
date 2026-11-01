#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter in the plugin lives here so the processor, editor and presets
// all agree on IDs. Modules run in this exact order:
//
// The chain order lives in Rack.h (onId) and the processor constructor.
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

    // beat divisions for the BPM-synced modules, in quarter notes
    inline const juce::StringArray& beatDivNames()
    {
        static const juce::StringArray n { "1/4", "1/8", "1/8 T", "1/16", "1/16 T", "1/32" };
        return n;
    }
    inline double beatDivBeats (int i)
    {
        static const double b[] { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 5, i)];
    }
    inline const juce::StringArray& gatePatternNames()
    {
        static const juce::StringArray n { "Straight", "Offbeat", "Dotted", "Trance 1", "Trance 2", "Stutter", "Half-Time", "Random" };
        return n;
    }

    // tempo sync for the LFOs (index 0 = Free, uses the Rate knob in Hz)
    inline const juce::StringArray& lfoSyncNames()
    {
        static const juce::StringArray n { "Free", "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/4 T", "1/8", "1/8 T", "1/16" };
        return n;
    }
    inline double lfoSyncBeats (int i)   // one LFO cycle, in quarter notes
    {
        static const double b[] { 4.0, 16.0, 8.0, 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25 };
        return b[juce::jlimit (0, 9, i)];
    }

    inline const juce::StringArray& noteNames()
    {
        static const juce::StringArray n { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return n;
    }
    inline const juce::StringArray& scaleNames()
    {
        static const juce::StringArray n { "Chromatic", "Major", "Minor", "Pentatonic", "Minor Pent.", "Blues" };
        return n;
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

        // Filter
        onOff ("filter_on", "Filter On");
        p.push_back (std::make_unique<APC> (pid ("filter_mode"), "Filter Mode", juce::StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));
        flt ("filter_cutoff",   "Filter Cutoff",    skewed (20.0f, 20000.0f, 1000.0f), 6000.0f, hz());
        flt ("filter_reso",     "Filter Resonance", skewed (0.5f, 10.0f, 2.0f), 0.9f, plain());
        flt ("filter_lforate",  "Filter LFO Rate",  skewed (0.01f, 10.0f, 1.0f), 0.3f, hz());
        flt ("filter_lfodepth", "Filter LFO Depth", lin (0.0f, 4.0f), 0.0f, oct());
        p.push_back (std::make_unique<APC> (pid ("filter_sync"), "Filter LFO Sync", lfoSyncNames(), 0));

        // Saturation (saturation)
        onOff ("gloss_on", "Saturation On");
        flt ("gloss_drive", "Saturation Drive", lin (0.0f, 24.0f), 6.0f, db());
        flt ("gloss_tone",  "Saturation Tone",  lin (-1.0f, 1.0f), 0.0f, bip());
        flt ("gloss_mix",   "Saturation Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // 3 Dial-Up (phone line / modem)
        onOff ("dial_on", "Telephone On");
        flt ("dial_band",    "Telephone Bandwidth", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("dial_noise",   "Telephone Line Noise", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("dial_crackle", "Telephone Crackle",   lin (0.0f, 1.0f), 0.2f, pct());
        flt ("dial_mix",     "Telephone Mix",       lin (0.0f, 1.0f), 1.0f, pct());
        onOff ("dial_modem", "Telephone Modem Squeal");

        // 4 8-Bit Crush
        onOff ("crush_on", "Bitcrusher On");
        flt ("crush_bits", "Bitcrusher Bits",       lin (1.0f, 16.0f), 8.0f, bits());
        flt ("crush_rate", "Bitcrusher Downsample", skewed (1.0f, 40.0f, 6.0f), 4.0f, times());
        flt ("crush_mix",  "Bitcrusher Mix",        lin (0.0f, 1.0f), 1.0f, pct());

        // 5 Buffering (stream stutter / dropout)
        onOff ("buf_on", "Stutter On");
        flt ("buf_chance",  "Stutter Chance",  lin (0.0f, 1.0f), 0.25f, pct());
        flt ("buf_slice",   "Stutter Slice",   skewed (20.0f, 500.0f, 120.0f), 120.0f, ms());
        flt ("buf_dropout", "Stutter Dropout", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("buf_mix",     "Stutter Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // 6 Lo-Bitrate (spectral codec artifacts)
        onOff ("lobit_on", "MP3 Compression On");
        flt ("lobit_kbps",  "MP3 Bitrate",  skewed (8.0f, 192.0f, 48.0f), 32.0f, kbps());
        flt ("lobit_swirl", "MP3 Artifacts", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("lobit_mix",   "MP3 Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // CODEC.EXE — old-school sample rates + telephony / ADPCM codecs
        onOff ("codec_on", "Lo-Fi Codec On");
        p.push_back (std::make_unique<APC> (pid ("codec_rate"), "Lo-Fi Codec Sample Rate", codecRateNames(), 2));
        p.push_back (std::make_unique<APC> (pid ("codec_type"), "Lo-Fi Codec Type", juce::StringArray { "8-bit PCM", "u-Law", "A-Law", "IMA ADPCM" }, 3));
        flt ("codec_alias", "Lo-Fi Codec Aliasing", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("codec_mix",   "Lo-Fi Codec Mix",      lin (0.0f, 1.0f), 1.0f, pct());

        // PACKETLOSS.EXE — VoIP packet loss + concealment
        onOff ("pkt_on", "Packet Loss On");
        flt ("pkt_loss", "Packet Loss Amount", lin (0.0f, 1.0f), 0.25f, pct());
        flt ("pkt_size", "Packet Size",        skewed (10.0f, 80.0f, 25.0f), 20.0f, ms());
        p.push_back (std::make_unique<APC> (pid ("pkt_conceal"), "Packet Concealment", juce::StringArray { "Robot", "Silence", "Noise" }, 0));
        flt ("pkt_mix",  "Packet Loss Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // CORRUPT.EXE — bit flips + reversed fragments, like a damaged download
        onOff ("corrupt_on", "Data Corruption On");
        flt ("corrupt_amount",  "Corruption Amount",  lin (0.0f, 1.0f), 0.3f, pct());
        flt ("corrupt_bits",    "Corruption Bits", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("corrupt_reverse", "Corruption Reverse", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("corrupt_mix",     "Corruption Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // LAG.EXE — lag spikes: audio slows to a crawl then fast-forwards to catch up
        onOff ("lag_on", "Varispeed On");
        flt ("lag_chance", "Varispeed Chance", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("lag_length", "Varispeed Length", skewed (80.0f, 1500.0f, 400.0f), 350.0f, ms());
        flt ("lag_depth",  "Varispeed Depth",  lin (0.0f, 1.0f), 0.7f, pct());
        flt ("lag_mix",    "Varispeed Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // JITTER.EXE — wow, flutter and clock drift
        onOff ("jit_on", "Wow & Flutter On");
        flt ("jit_wow",     "Wow",     lin (0.0f, 1.0f), 0.3f, pct());
        flt ("jit_flutter", "Flutter", lin (0.0f, 1.0f), 0.2f, pct());
        flt ("jit_drift",   "Drift",   lin (0.0f, 1.0f), 0.2f, pct());
        flt ("jit_mix",     "Wow & Flutter Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // NIGHTCORE.EXE — pitch shifter
        onOff ("pitch_on", "Pitch Shifter On");
        flt ("pitch_semi", "Pitch Shift", lin (-12.0f, 12.0f), 4.0f,
             Attr().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; }));
        flt ("pitch_window", "Pitch Shifter Grain", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("pitch_mix",  "Pitch Shifter Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // RINGTONE.EXE — ring modulator with ringtone warble
        onOff ("ring_on", "Ring Mod On");
        flt ("ring_freq",   "Ring Mod Frequency",   skewed (20.0f, 4000.0f, 400.0f), 440.0f, hz());
        flt ("ring_shape",  "Ring Mod Shape",  lin (0.0f, 1.0f), 0.0f, pct());
        flt ("ring_warble", "Ring Mod Arpeggio", lin (0.0f, 1.0f), 0.0f, pct());
        flt ("ring_mix",    "Ring Mod Mix",    lin (0.0f, 1.0f), 0.5f, pct());

        // GLITCH.EXE — beat-synced repeat / stutter / reverse / tape stop
        onOff ("glitch_on", "Beat Repeat On");
        p.push_back (std::make_unique<APC> (pid ("glitch_div"), "Beat Repeat Division", beatDivNames(), 3));
        p.push_back (std::make_unique<APC> (pid ("glitch_style"), "Beat Repeat Style", juce::StringArray { "Random", "Repeat", "Stutter", "Reverse", "Tape Stop" }, 0));
        flt ("glitch_chance", "Beat Repeat Chance", lin (0.0f, 1.0f), 0.35f, pct());
        flt ("glitch_mix",    "Beat Repeat Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // GATE.EXE — beat-synced rhythmic gate
        onOff ("gate_on", "Trance Gate On");
        p.push_back (std::make_unique<APC> (pid ("gate_pattern"), "Trance Gate Pattern", gatePatternNames(), 3));
        p.push_back (std::make_unique<APC> (pid ("gate_div"), "Trance Gate Division", beatDivNames(), 3));
        flt ("gate_depth",  "Trance Gate Depth",  lin (0.0f, 1.0f), 0.9f, pct());
        flt ("gate_smooth", "Trance Gate Smooth", lin (0.0f, 1.0f), 0.3f, pct());

        // Glory Comp — easy compressor with auto makeup
        onOff ("comp_on", "Compressor On");
        flt ("comp_squash", "Compressor Amount", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("comp_ratio",  "Compressor Ratio",  skewed (1.0f, 20.0f, 4.0f), 4.0f,
             Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + ":1"; }));
        flt ("comp_speed",  "Compressor Speed",  lin (0.0f, 1.0f), 0.4f, pct());
        flt ("comp_mix",    "Compressor Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // Glass EQ — 3-band tone shaping
        onOff ("eq_on", "EQ On");
        flt ("eq_low",     "EQ Low",      lin (-12.0f, 12.0f), 0.0f, db());
        flt ("eq_mid",     "EQ Mid",      lin (-12.0f, 12.0f), 0.0f, db());
        flt ("eq_midfreq", "EQ Mid Freq", skewed (200.0f, 6000.0f, 1000.0f), 1000.0f, hz());
        flt ("eq_high",    "EQ High",     lin (-12.0f, 12.0f), 0.0f, db());

        // VINYL.EXE — crackle, dust, hiss, wear and 33rpm wow
        onOff ("vinyl_on", "Vinyl Sim On");
        flt ("vinyl_crackle", "Vinyl Crackle", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("vinyl_wear",    "Vinyl Wear",    lin (0.0f, 1.0f), 0.3f, pct());
        flt ("vinyl_wow",     "Vinyl Wow",     lin (0.0f, 1.0f), 0.2f, pct());
        flt ("vinyl_mix",     "Vinyl Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // TTS.EXE — robotic text-to-speech voice
        onOff ("tts_on", "Robot Voice On");
        flt ("tts_pitch", "Robot Pitch", skewed (40.0f, 400.0f, 120.0f), 110.0f, hz());
        flt ("tts_metal", "Robot Metal", lin (0.0f, 0.95f), 0.6f, pct());
        flt ("tts_buzz",  "Robot Buzz",  lin (0.0f, 1.0f), 0.3f, pct());
        flt ("tts_mix",   "Robot Mix",   lin (0.0f, 1.0f), 0.7f, pct());

        // PUMP.EXE — tempo-synced sidechain pumping
        onOff ("pump_on", "Sidechain Pump On");
        p.push_back (std::make_unique<APC> (pid ("pump_div"), "Sidechain Rate", juce::StringArray { "1/2", "1/4", "1/8", "1/16" }, 1));
        flt ("pump_depth", "Sidechain Depth", lin (0.0f, 1.0f), 0.7f, pct());
        flt ("pump_shape", "Sidechain Release", lin (0.0f, 1.0f), 0.4f, pct());
        p.push_back (std::make_unique<APC> (pid ("pump_mode"), "Sidechain Source", juce::StringArray { "Tempo", "Sidechain In" }, 0));

        // Stereo Spread — mono-safe widener / doubler
        onOff ("spread_on", "Stereo Widener On");
        flt ("spread_width",  "Widener Width",  lin (0.0f, 1.0f), 0.5f, pct());
        flt ("spread_motion", "Widener Motion", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("spread_mix",    "Widener Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // Transient Shaper
        onOff ("trans_on", "Transient Shaper On");
        flt ("trans_attack",  "Transient Attack",  lin (-1.0f, 1.0f), 0.4f, bip());
        flt ("trans_sustain", "Transient Sustain", lin (-1.0f, 1.0f), 0.0f, bip());
        flt ("trans_mix",     "Transient Mix",     lin (0.0f, 1.0f), 1.0f, pct());

        // Auto-Wah (envelope filter)
        onOff ("wah_on", "Auto-Wah On");
        flt ("wah_sens",  "Auto-Wah Sensitivity", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("wah_range", "Auto-Wah Range",       lin (0.0f, 1.0f), 0.6f, pct());
        flt ("wah_reso",  "Auto-Wah Resonance",   lin (0.0f, 1.0f), 0.5f, pct());
        flt ("wah_mix",   "Auto-Wah Mix",         lin (0.0f, 1.0f), 1.0f, pct());

        // Volume Swell (auto-swell / slow gear)
        onOff ("swell_on", "Volume Swell On");
        flt ("swell_attack", "Swell Attack",      skewed (20.0f, 3000.0f, 400.0f), 500.0f, ms());
        flt ("swell_sens",   "Swell Sensitivity", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("swell_mix",    "Swell Mix",         lin (0.0f, 1.0f), 1.0f, pct());

        // Blur — diffused smear for huge ambient pads, with shimmer and freeze
        onOff ("blur_on", "Blur On");
        flt ("blur_size",    "Blur Size",    lin (0.0f, 1.0f), 0.6f, pct());
        flt ("blur_smear",   "Blur Smear",   lin (0.0f, 1.0f), 0.6f, pct());
        flt ("blur_shimmer", "Blur Shimmer", lin (0.0f, 1.0f), 0.0f, pct());
        flt ("blur_mix",     "Blur Mix",     lin (0.0f, 1.0f), 0.5f, pct());
        onOff ("blur_freeze", "Blur Freeze");

        // SPEAKER.EXE — cheap playback devices
        onOff ("spk_on", "Speaker Sim On");
        p.push_back (std::make_unique<APC> (pid ("spk_model"), "Speaker Sim Model", speakerNames(), 1));
        flt ("spk_drive", "Speaker Sim Drive", lin (0.0f, 1.0f), 0.3f, pct());
        flt ("spk_mix",   "Speaker Sim Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // Chorus
        onOff ("chorus_on", "Chorus On");
        flt ("chorus_rate",  "Chorus Rate",     skewed (0.05f, 5.0f, 0.8f), 0.6f, hz());
        flt ("chorus_depth", "Chorus Depth",    lin (0.0f, 1.0f), 0.4f, pct());
        flt ("chorus_fb",    "Chorus Feedback", lin (-0.9f, 0.9f), 0.0f, bip());
        flt ("chorus_mix",   "Chorus Mix",      lin (0.0f, 1.0f), 0.5f, pct());
        p.push_back (std::make_unique<APC> (pid ("chorus_sync"), "Chorus Sync", lfoSyncNames(), 0));

        // Phaser
        onOff ("phaser_on", "Phaser On");
        flt ("phaser_rate",  "Phaser Rate",     skewed (0.02f, 5.0f, 0.5f), 0.3f, hz());
        flt ("phaser_depth", "Phaser Depth",    lin (0.0f, 1.0f), 0.6f, pct());
        flt ("phaser_fb",    "Phaser Feedback", lin (-0.9f, 0.9f), 0.4f, bip());
        flt ("phaser_mix",   "Phaser Mix",      lin (0.0f, 1.0f), 0.5f, pct());
        p.push_back (std::make_unique<APC> (pid ("phaser_sync"), "Phaser Sync", lfoSyncNames(), 0));

        // 9 Trem / Pan
        onOff ("trem_on", "Tremolo On");
        flt ("trem_rate",  "Tremolo Rate",  skewed (0.1f, 20.0f, 3.0f), 4.0f, hz());
        flt ("trem_depth", "Tremolo Depth", lin (0.0f, 1.0f), 0.5f, pct());
        flt ("trem_shape", "Tremolo Shape", lin (0.0f, 1.0f), 0.0f, pct());
        flt ("trem_pan",   "Tremolo Pan",   lin (0.0f, 1.0f), 0.0f, pct());
        p.push_back (std::make_unique<APC> (pid ("trem_sync"), "Tremolo Sync", lfoSyncNames(), 0));

        // Delay
        onOff ("delay_on", "Delay On");
        p.push_back (std::make_unique<APC> (pid ("delay_sync"), "Delay Sync", delaySyncNames(), 0));
        flt ("delay_time", "Delay Time",     skewed (10.0f, 2000.0f, 400.0f), 350.0f, ms());
        flt ("delay_fb",   "Delay Feedback", lin (0.0f, 0.95f), 0.35f, pct());
        flt ("delay_tone", "Delay Tone",     lin (0.0f, 1.0f), 0.6f, pct());
        flt ("delay_mix",  "Delay Mix",      lin (0.0f, 1.0f), 0.3f, pct());
        onOff ("delay_pp", "Delay Ping-Pong", true);

        // Reverb
        onOff ("verb_on", "Reverb On");
        flt ("verb_size",  "Reverb Size",  lin (0.0f, 1.0f), 0.6f, pct());
        flt ("verb_damp",  "Reverb Damping", lin (0.0f, 1.0f), 0.4f, pct());
        flt ("verb_width", "Reverb Width", lin (0.0f, 1.0f), 1.0f, pct());
        flt ("verb_mix",   "Reverb Mix",   lin (0.0f, 1.0f), 0.25f, pct());

        // Talk Box — vowel formant filter that "talks"
        onOff ("talk_on", "Talk Box On");
        flt ("talk_vowel", "Talk Box Vowel", lin (0.0f, 1.0f), 0.0f,
             Attr().withStringFromValueFunction ([] (float v, int)
             {
                 static const char* names[] { "A", "A-E", "E", "E-I", "I", "I-O", "O", "O-U", "U" };
                 return juce::String (names[juce::jlimit (0, 8, juce::roundToInt (v * 8.0f))]);
             }));
        flt ("talk_rate",  "Talk Box Rate",  skewed (0.05f, 12.0f, 1.5f), 1.2f, hz());
        flt ("talk_sweep", "Talk Box Sweep", lin (0.0f, 1.0f), 0.6f, pct());
        flt ("talk_mix",   "Talk Box Mix",   lin (0.0f, 1.0f), 1.0f, pct());
        p.push_back (std::make_unique<APC> (pid ("talk_voice"), "Talk Box Voice", juce::StringArray { "Deep", "Normal", "High" }, 1));
        p.push_back (std::make_unique<APC> (pid ("talk_sync"), "Talk Box Sync", lfoSyncNames(), 0));

        // Frequency Shifter — every partial moves by the same number of Hz
        onOff ("fshift_on", "Freq Shifter On");
        flt ("fshift_hz", "Freq Shift", juce::NormalisableRange<float> (-2000.0f, 2000.0f, 0.0f, 0.35f, true), 40.0f,
             Attr().withStringFromValueFunction ([] (float v, int)
             { return (v > 0 ? "+" : "") + (std::abs (v) < 10.0f ? juce::String (v, 2) : juce::String (juce::roundToInt (v))) + " Hz"; }));
        flt ("fshift_fb",     "Freq Shifter Feedback", lin (0.0f, 1.0f), 0.0f, pct());
        flt ("fshift_spread", "Freq Shifter Spread",   lin (0.0f, 1.0f), 0.0f, pct());
        flt ("fshift_mix",    "Freq Shifter Mix",      lin (0.0f, 1.0f), 0.5f, pct());

        // Flanger — jet whoosh
        onOff ("flange_on", "Flanger On");
        flt ("flange_rate",  "Flanger Rate",     skewed (0.02f, 5.0f, 0.4f), 0.25f, hz());
        flt ("flange_depth", "Flanger Depth",    lin (0.0f, 1.0f), 0.7f, pct());
        flt ("flange_fb",    "Flanger Feedback", lin (-0.95f, 0.95f), 0.6f, bip());
        flt ("flange_mix",   "Flanger Mix",      lin (0.0f, 1.0f), 0.6f, pct());
        p.push_back (std::make_unique<APC> (pid ("flange_sync"), "Flanger Sync", lfoSyncNames(), 0));

        // Reverse Delay — plays chunks backwards, tempo-syncable
        onOff ("rdelay_on", "Reverse Delay On");
        p.push_back (std::make_unique<APC> (pid ("rdelay_sync"), "Reverse Delay Sync", delaySyncNames(), 6));
        flt ("rdelay_time", "Reverse Delay Time",     skewed (60.0f, 2000.0f, 500.0f), 500.0f, ms());
        flt ("rdelay_fb",   "Reverse Delay Feedback", lin (0.0f, 0.9f), 0.3f, pct());
        flt ("rdelay_mix",  "Reverse Delay Mix",      lin (0.0f, 1.0f), 0.4f, pct());

        // Noise Gate
        onOff ("ngate_on", "Noise Gate On");
        flt ("ngate_thresh",  "Gate Threshold", lin (-80.0f, 0.0f), -45.0f, db());
        flt ("ngate_range",   "Gate Range",     lin (0.0f, 80.0f), 60.0f,
             Attr().withStringFromValueFunction ([] (float v, int) { return "-" + juce::String (juce::roundToInt (v)) + " dB"; }));
        flt ("ngate_release", "Gate Release",   skewed (5.0f, 1000.0f, 120.0f), 120.0f, ms());
        p.push_back (std::make_unique<APC> (pid ("ngate_key"), "Gate Key", juce::StringArray { "Input", "Sidechain In" }, 0));

        // Fuzz — proper distortion, four flavours
        onOff ("fuzz_on", "Fuzz On");
        p.push_back (std::make_unique<APC> (pid ("fuzz_type"), "Fuzz Type", juce::StringArray { "Fuzz", "Hard Clip", "Octave Up", "Wavefolder" }, 0));
        flt ("fuzz_drive", "Fuzz Drive", lin (0.0f, 48.0f), 20.0f, db());
        flt ("fuzz_tone",  "Fuzz Tone",  skewed (400.0f, 14000.0f, 3000.0f), 4000.0f, hz());
        flt ("fuzz_gate",  "Fuzz Gate",  lin (0.0f, 1.0f), 0.1f, pct());
        flt ("fuzz_mix",   "Fuzz Mix",   lin (0.0f, 1.0f), 1.0f, pct());

        // Auto-Tune — pitch correction to a key and scale (0 speed = hard robot snap)
        onOff ("tune_on", "Auto-Tune On");
        p.push_back (std::make_unique<APC> (pid ("tune_key"), "Auto-Tune Key", noteNames(), 0));
        p.push_back (std::make_unique<APC> (pid ("tune_scale"), "Auto-Tune Scale", scaleNames(), 0));
        flt ("tune_speed",  "Auto-Tune Speed",  lin (0.0f, 1.0f), 0.0f,
             Attr().withStringFromValueFunction ([] (float v, int) { return v < 0.005f ? juce::String ("Robot") : juce::String (juce::roundToInt (v * 400.0f)) + " ms"; }));
        flt ("tune_amount", "Auto-Tune Amount", lin (0.0f, 1.0f), 1.0f, pct());
        flt ("tune_mix",    "Auto-Tune Mix",    lin (0.0f, 1.0f), 1.0f, pct());

        // Vocoder — your audio "speaks" through a synth (or the sidechain)
        onOff ("voc_on", "Vocoder On");
        p.push_back (std::make_unique<APC> (pid ("voc_carrier"), "Vocoder Carrier", juce::StringArray { "Saw", "Chord", "Noise", "Sidechain In" }, 1));
        flt ("voc_note", "Vocoder Note", juce::NormalisableRange<float> (24.0f, 72.0f, 1.0f), 45.0f,
             Attr().withStringFromValueFunction ([] (float v, int)
             {
                 const int n = juce::roundToInt (v);
                 return noteNames()[n % 12] + juce::String (n / 12 - 1);
             }));
        flt ("voc_formant", "Vocoder Formant", lin (-1.0f, 1.0f), 0.0f, oct());
        flt ("voc_release", "Vocoder Release", skewed (5.0f, 300.0f, 40.0f), 40.0f, ms());
        flt ("voc_mix",     "Vocoder Mix",     lin (0.0f, 1.0f), 0.85f, pct());

        // Master
        flt ("in_gain",  "Input Gain",   lin (-24.0f, 24.0f), 0.0f, db());
        flt ("out_gain", "Output Gain",  lin (-24.0f, 24.0f), 0.0f, db());
        flt ("width",    "Stereo Width", lin (0.0f, 2.0f), 1.0f, pct());
        flt ("mix",      "Global Mix",   lin (0.0f, 1.0f), 1.0f, pct());
        onOff ("limiter", "Safety Limiter", true);

        return { p.begin(), p.end() };
    }
}
