#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Which modules are "in the rack" (shown in the UI). This is UI/session state,
// stored as a string of 0/1 flags on the APVTS tree so it saves with the project.
// The audio only cares about each module's *_on parameter: a module in the rack
// can still be bypassed with its power switch; removing it from the rack turns it off.
namespace Rack
{
    constexpr int numModules = 35;

    inline const char* onId (int i)
    {
        // chain order — must match the order modules are added in EvilFXProcessor
        static const char* ids[numModules] { "trans_on", "filter_on", "wah_on", "talk_on", "eq_on", "gloss_on", "pitch_on",
                                             "ring_on", "fshift_on", "tts_on", "codec_on", "crush_on", "dial_on", "vinyl_on",
                                             "lobit_on", "pkt_on", "buf_on", "corrupt_on", "lag_on", "jit_on", "glitch_on",
                                             "gate_on", "pump_on", "swell_on", "chorus_on", "flange_on", "phaser_on", "trem_on",
                                             "spread_on", "rdelay_on", "delay_on", "blur_on", "verb_on", "comp_on", "spk_on" };
        return ids[juce::jlimit (0, numModules - 1, i)];
    }

    // Rack strings saved by v1.0 (31 effects, before Talk Box / Freq Shifter / Flanger /
    // Reverse Delay) are converted on the fly so old projects open with the same rack.
    inline juce::String fromLegacy (const juce::String& old)
    {
        static const char* v1[31] { "trans_on", "filter_on", "wah_on", "eq_on", "gloss_on", "pitch_on", "ring_on",
                                    "tts_on", "codec_on", "crush_on", "dial_on", "vinyl_on", "lobit_on", "pkt_on",
                                    "buf_on", "corrupt_on", "lag_on", "jit_on", "glitch_on", "gate_on", "pump_on",
                                    "swell_on", "chorus_on", "phaser_on", "trem_on", "spread_on", "delay_on",
                                    "blur_on", "verb_on", "comp_on", "spk_on" };
        juce::String s;
        for (int i = 0; i < numModules; ++i)
        {
            char flag = '0';
            for (int k = 0; k < 31; ++k)
                if (juce::String (v1[k]) == onId (i)) { flag = old[k] == '1' ? '1' : '0'; break; }
            s << flag;
        }
        return s;
    }

    inline juce::String get (const juce::ValueTree& state)
    {
        auto s = state.getProperty ("rack").toString();
        if (s.length() == numModules) return s;
        if (s.length() == 31) return fromLegacy (s);
        return juce::String::repeatedString ("0", numModules);
    }

    inline bool isValid (const juce::ValueTree& state)
    {
        const int len = state.getProperty ("rack").toString().length();
        return len == numModules || len == 31;
    }

    // upgrade an old rack string in place (call after loading a state)
    inline void upgrade (juce::ValueTree& state)
    {
        if (state.getProperty ("rack").toString().length() == 31) state.setProperty ("rack", get (state), nullptr);
    }

    inline bool contains (const juce::ValueTree& state, int i) { return get (state)[i] == '1'; }

    inline void set (juce::ValueTree& state, int i, bool inRack)
    {
        auto s = get (state);
        s = s.substring (0, i) + (inRack ? "1" : "0") + s.substring (i + 1);
        state.setProperty ("rack", s, nullptr);
    }

    // rack = exactly the modules that are currently switched on
    inline void syncFromParams (juce::AudioProcessorValueTreeState& apvts)
    {
        juce::String s;
        for (int i = 0; i < numModules; ++i)
            s << (apvts.getRawParameterValue (onId (i))->load() > 0.5f ? "1" : "0");
        apvts.state.setProperty ("rack", s, nullptr);
    }

    // add (never remove) any module that got switched on elsewhere, e.g. host automation
    inline void addSwitchedOn (juce::AudioProcessorValueTreeState& apvts)
    {
        auto s = get (apvts.state);
        bool changed = false;
        for (int i = 0; i < numModules; ++i)
            if (s[i] != '1' && apvts.getRawParameterValue (onId (i))->load() > 0.5f)
            {
                s = s.substring (0, i) + "1" + s.substring (i + 1);
                changed = true;
            }
        if (changed) apvts.state.setProperty ("rack", s, nullptr);
    }
}
