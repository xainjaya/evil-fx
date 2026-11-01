#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Which modules are "in the rack" (shown in the UI) and the order the chain runs in.
//
// Each module has a fixed *index* (its slot in the processor, the order below) and
// is identified in saved state by its power-switch ID ("verb_on" etc.), so adding
// effects in future versions never breaks old projects.
//
//   "rack"  = comma-separated IDs of the modules in the rack
//   "order" = comma-separated IDs of every module, in signal-chain order
//
// The audio only cares about each module's *_on parameter: a module in the rack can
// still be bypassed with its power switch; removing it from the rack turns it off.
namespace Rack
{
    constexpr int numModules = 39;

    inline const char* onId (int i)
    {
        // default chain order — must match the order modules are added in EvilFXProcessor
        static const char* ids[numModules] { "ngate_on", "trans_on", "tune_on", "filter_on", "wah_on", "talk_on", "eq_on",
                                             "gloss_on", "fuzz_on", "pitch_on", "ring_on", "fshift_on", "voc_on", "tts_on",
                                             "codec_on", "crush_on", "dial_on", "vinyl_on", "lobit_on", "pkt_on", "buf_on",
                                             "corrupt_on", "lag_on", "jit_on", "glitch_on", "gate_on", "pump_on", "swell_on",
                                             "chorus_on", "flange_on", "phaser_on", "trem_on", "spread_on", "rdelay_on",
                                             "delay_on", "blur_on", "verb_on", "comp_on", "spk_on" };
        return ids[juce::jlimit (0, numModules - 1, i)];
    }

    inline int indexOf (const juce::String& id)
    {
        for (int i = 0; i < numModules; ++i) if (id == onId (i)) return i;
        return -1;
    }

    //--------------------------------------------------------------------------
    // Older versions stored the rack as a string of 0/1 flags in module order.
    namespace detail
    {
        inline bool isFlagString (const juce::String& s)
        {
            return s.isNotEmpty() && s.containsOnly ("01");
        }

        inline juce::StringArray legacyIds (int length)
        {
            if (length == 31)   // v1.0
                return { "trans_on", "filter_on", "wah_on", "eq_on", "gloss_on", "pitch_on", "ring_on", "tts_on", "codec_on",
                         "crush_on", "dial_on", "vinyl_on", "lobit_on", "pkt_on", "buf_on", "corrupt_on", "lag_on", "jit_on",
                         "glitch_on", "gate_on", "pump_on", "swell_on", "chorus_on", "phaser_on", "trem_on", "spread_on",
                         "delay_on", "blur_on", "verb_on", "comp_on", "spk_on" };
            if (length == 35)   // v1.1 preview
                return { "trans_on", "filter_on", "wah_on", "talk_on", "eq_on", "gloss_on", "pitch_on", "ring_on", "fshift_on",
                         "tts_on", "codec_on", "crush_on", "dial_on", "vinyl_on", "lobit_on", "pkt_on", "buf_on", "corrupt_on",
                         "lag_on", "jit_on", "glitch_on", "gate_on", "pump_on", "swell_on", "chorus_on", "flange_on",
                         "phaser_on", "trem_on", "spread_on", "rdelay_on", "delay_on", "blur_on", "verb_on", "comp_on", "spk_on" };
            return {};
        }
    }

    // flags string, one char per module index ('1' = in the rack)
    inline juce::String get (const juce::ValueTree& state)
    {
        const auto raw = state.getProperty ("rack").toString();
        juce::String flags = juce::String::repeatedString ("0", numModules);
        auto mark = [&flags] (const juce::String& id)
        {
            const int i = indexOf (id);
            if (i >= 0) flags = flags.substring (0, i) + "1" + flags.substring (i + 1);
        };
        if (detail::isFlagString (raw))
        {
            const auto ids = detail::legacyIds (raw.length());
            for (int k = 0; k < ids.size(); ++k) if (raw[k] == '1') mark (ids[k]);
        }
        else
        {
            for (auto& id : juce::StringArray::fromTokens (raw, ",", "")) mark (id.trim());
        }
        return flags;
    }

    inline bool isValid (const juce::ValueTree& state)
    {
        if (! state.hasProperty ("rack")) return false;
        const auto raw = state.getProperty ("rack").toString();
        return ! detail::isFlagString (raw) || detail::legacyIds (raw.length()).size() > 0;
    }

    inline void setFlags (juce::ValueTree& state, const juce::String& flags, juce::UndoManager* um = nullptr)
    {
        juce::StringArray ids;
        for (int i = 0; i < numModules; ++i) if (flags[i] == '1') ids.add (onId (i));
        const auto joined = ids.joinIntoString (",");
        if (state.getProperty ("rack").toString() != joined || ! state.hasProperty ("rack"))
            state.setProperty ("rack", joined, um);
    }

    inline bool contains (const juce::ValueTree& state, int i) { return get (state)[i] == '1'; }

    inline void set (juce::ValueTree& state, int i, bool inRack, juce::UndoManager* um = nullptr)
    {
        auto s = get (state);
        s = s.substring (0, i) + (inRack ? "1" : "0") + s.substring (i + 1);
        setFlags (state, s, um);
    }

    // rewrite an old flag string (or anything odd) in the current format
    inline void upgrade (juce::ValueTree& state) { setFlags (state, get (state)); }

    // rack = exactly the modules that are currently switched on
    inline void syncFromParams (juce::AudioProcessorValueTreeState& apvts, juce::UndoManager* um = nullptr)
    {
        juce::String s;
        for (int i = 0; i < numModules; ++i)
            s << (apvts.getRawParameterValue (onId (i))->load() > 0.5f ? "1" : "0");
        setFlags (apvts.state, s, um);
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
        if (changed) setFlags (apvts.state, s);
    }

    //--------------------------------------------------------------------------
    // Chain order: a permutation of module indices.
    inline juce::Array<int> defaultOrder()
    {
        juce::Array<int> o;
        for (int i = 0; i < numModules; ++i) o.add (i);
        return o;
    }

    // Keeps the saved order for known modules; anything missing (e.g. an effect added
    // in a newer version) slots in next to its default neighbour.
    inline juce::Array<int> getOrder (const juce::ValueTree& state)
    {
        juce::Array<int> o;
        for (auto& id : juce::StringArray::fromTokens (state.getProperty ("order").toString(), ",", ""))
        {
            const int i = indexOf (id.trim());
            if (i >= 0 && ! o.contains (i)) o.add (i);
        }
        for (int m = 0; m < numModules; ++m)
        {
            if (o.contains (m)) continue;
            int insertAt = o.size();
            for (int k = 0; k < o.size(); ++k) if (o[k] > m) { insertAt = k; break; }
            o.insert (insertAt, m);
        }
        return o;
    }

    inline juce::String orderToString (const juce::Array<int>& o)
    {
        juce::StringArray ids;
        for (int i : o) ids.add (onId (i));
        return ids.joinIntoString (",");
    }

    inline void setOrder (juce::ValueTree& state, const juce::Array<int>& o, juce::UndoManager* um = nullptr)
    {
        const auto s = orderToString (o);
        if (state.getProperty ("order").toString() != s) state.setProperty ("order", s, um);
    }

    inline bool isDefaultOrder (const juce::ValueTree& state) { return getOrder (state) == defaultOrder(); }
}
