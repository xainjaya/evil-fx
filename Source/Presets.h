#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>
#include "Rack.h"

// Factory presets. Each preset starts from the defaults (all modules off,
// master neutral) and only lists what it changes — values are in real units.
namespace Presets
{
    struct Preset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    inline const std::vector<Preset>& all()
    {
        static const std::vector<Preset> p {
            { "Init (Clean)", {} },

            { "Crimson Glass Pad", {
                { "chorus_on", 1 }, { "chorus_rate", 0.4f }, { "chorus_depth", 0.55f }, { "chorus_mix", 0.5f },
                { "verb_on", 1 },   { "verb_size", 0.85f }, { "verb_damp", 0.3f }, { "verb_mix", 0.4f },
                { "delay_on", 1 },  { "delay_sync", 4 }, { "delay_fb", 0.4f }, { "delay_tone", 0.75f }, { "delay_mix", 0.22f },
                { "width", 1.35f } } },

            { "Blood Bath", {
                { "phaser_on", 1 }, { "phaser_rate", 0.18f }, { "phaser_depth", 0.8f }, { "phaser_fb", 0.6f }, { "phaser_mix", 0.55f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 3500 }, { "filter_reso", 2.5f },
                { "filter_lforate", 0.12f }, { "filter_lfodepth", 1.5f },
                { "verb_on", 1 },   { "verb_size", 0.7f }, { "verb_mix", 0.3f } } },

            { "Screensaver Nightmare", {
                { "chorus_on", 1 }, { "chorus_rate", 0.15f }, { "chorus_depth", 0.8f }, { "chorus_fb", 0.3f }, { "chorus_mix", 0.6f },
                { "trem_on", 1 },   { "trem_rate", 0.25f }, { "trem_depth", 0.35f }, { "trem_pan", 1 },
                { "verb_on", 1 },   { "verb_size", 1.0f }, { "verb_damp", 0.2f }, { "verb_mix", 0.55f },
                { "gloss_on", 1 },  { "gloss_drive", 3 }, { "gloss_tone", 0.3f } } },

            { "Dial-Up 1999", {
                { "dial_on", 1 }, { "dial_band", 0.15f }, { "dial_noise", 0.45f }, { "dial_crackle", 0.5f }, { "dial_modem", 1 },
                { "crush_on", 1 }, { "crush_bits", 10 }, { "crush_rate", 2 } } },

            { "Instant Messenger Call", {
                { "dial_on", 1 }, { "dial_band", 0.45f }, { "dial_noise", 0.15f }, { "dial_crackle", 0.1f },
                { "lobit_on", 1 }, { "lobit_kbps", 16 }, { "lobit_swirl", 0.6f },
                { "buf_on", 1 }, { "buf_chance", 0.12f }, { "buf_slice", 80 }, { "buf_dropout", 0.6f } } },

            { "Buffering...", {
                { "buf_on", 1 }, { "buf_chance", 0.45f }, { "buf_slice", 160 }, { "buf_dropout", 0.35f },
                { "lobit_on", 1 }, { "lobit_kbps", 40 }, { "lobit_swirl", 0.4f } } },

            { "Low Bitrate Rip", {
                { "lobit_on", 1 }, { "lobit_kbps", 24 }, { "lobit_swirl", 0.75f },
                { "filter_on", 1 }, { "filter_mode", 2 }, { "filter_cutoff", 90 }, { "filter_reso", 0.7f } } },

            { "Webring Radio", {
                { "lobit_on", 1 }, { "lobit_kbps", 56 }, { "lobit_swirl", 0.35f },
                { "gloss_on", 1 }, { "gloss_drive", 9 }, { "gloss_tone", -0.2f },
                { "filter_on", 1 }, { "filter_mode", 1 }, { "filter_cutoff", 1600 }, { "filter_reso", 0.8f },
                { "dial_on", 1 }, { "dial_band", 0.85f }, { "dial_noise", 0.25f }, { "dial_crackle", 0.35f }, { "dial_mix", 0.5f },
                { "width", 0.4f } } },

            { "Y2K Mall Speaker", {
                { "filter_on", 1 }, { "filter_mode", 1 }, { "filter_cutoff", 1100 }, { "filter_reso", 0.9f },
                { "gloss_on", 1 }, { "gloss_drive", 12 }, { "gloss_tone", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_damp", 0.7f }, { "verb_mix", 0.45f },
                { "width", 0.2f } } },

            { "8-Bit Arcade Lobby", {
                { "crush_on", 1 }, { "crush_bits", 5 }, { "crush_rate", 6 },
                { "delay_on", 1 }, { "delay_sync", 3 }, { "delay_fb", 0.45f }, { "delay_tone", 0.5f }, { "delay_mix", 0.28f },
                { "verb_on", 1 }, { "verb_size", 0.4f }, { "verb_mix", 0.15f } } },

            { "Glossy Tape Stack", {
                { "gloss_on", 1 }, { "gloss_drive", 14 }, { "gloss_tone", -0.35f }, { "gloss_mix", 0.8f },
                { "chorus_on", 1 }, { "chorus_rate", 1.2f }, { "chorus_depth", 0.15f }, { "chorus_mix", 0.35f },
                { "delay_on", 1 }, { "delay_sync", 6 }, { "delay_fb", 0.3f }, { "delay_tone", 0.35f }, { "delay_pp", 0 }, { "delay_mix", 0.2f } } },

            { "RealAudio 1998", {
                { "codec_on", 1 }, { "codec_rate", 2 }, { "codec_type", 3 }, { "codec_alias", 0.3f },
                { "lobit_on", 1 }, { "lobit_kbps", 20 }, { "lobit_swirl", 0.55f },
                { "buf_on", 1 }, { "buf_chance", 0.15f }, { "buf_slice", 220 }, { "buf_dropout", 0.1f },
                { "jit_on", 1 }, { "jit_wow", 0.15f }, { "jit_flutter", 0.1f }, { "jit_drift", 0.4f } } },

            { "VoIP Nightmare", {
                { "codec_on", 1 }, { "codec_rate", 3 }, { "codec_type", 1 }, { "codec_alias", 0.6f },
                { "pkt_on", 1 }, { "pkt_loss", 0.45f }, { "pkt_size", 20 }, { "pkt_conceal", 0 },
                { "spk_on", 1 }, { "spk_model", 2 }, { "spk_drive", 0.3f } } },

            { "Laptop Speaker Stream", {
                { "lobit_on", 1 }, { "lobit_kbps", 64 }, { "lobit_swirl", 0.3f },
                { "spk_on", 1 }, { "spk_model", 1 }, { "spk_drive", 0.15f },
                { "lag_on", 1 }, { "lag_chance", 0.15f }, { "lag_length", 500 }, { "lag_depth", 0.6f } } },

            { "Corrupted Download", {
                { "corrupt_on", 1 }, { "corrupt_amount", 0.45f }, { "corrupt_bits", 0.55f }, { "corrupt_reverse", 0.35f },
                { "buf_on", 1 }, { "buf_chance", 0.2f }, { "buf_slice", 90 }, { "buf_dropout", 0.5f },
                { "codec_on", 1 }, { "codec_rate", 1 }, { "codec_type", 0 }, { "codec_alias", 0.8f } } },

            { "Lag Spike Lobby", {
                { "lag_on", 1 }, { "lag_chance", 0.55f }, { "lag_length", 700 }, { "lag_depth", 0.9f },
                { "jit_on", 1 }, { "jit_wow", 0.2f }, { "jit_flutter", 0.25f }, { "jit_drift", 0.3f },
                { "crush_on", 1 }, { "crush_bits", 9 }, { "crush_rate", 3 },
                { "verb_on", 1 }, { "verb_size", 0.5f }, { "verb_mix", 0.18f } } },

            { "PC Speaker Demon", {
                { "spk_on", 1 }, { "spk_model", 0 }, { "spk_drive", 0.6f },
                { "codec_on", 1 }, { "codec_rate", 4 }, { "codec_type", 0 }, { "codec_alias", 1.0f },
                { "trem_on", 1 }, { "trem_rate", 7 }, { "trem_depth", 0.4f }, { "trem_shape", 0.8f } } },

            { "Warped Cassette Rip", {
                { "jit_on", 1 }, { "jit_wow", 0.6f }, { "jit_flutter", 0.45f }, { "jit_drift", 0.5f },
                { "gloss_on", 1 }, { "gloss_drive", 8 }, { "gloss_tone", -0.45f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 7000 }, { "filter_reso", 0.7f },
                { "spk_on", 1 }, { "spk_model", 5 }, { "spk_drive", 0.2f }, { "spk_mix", 0.6f } } },

            { "CRT TV at 3AM", {
                { "spk_on", 1 }, { "spk_model", 4 }, { "spk_drive", 0.25f },
                { "dial_on", 1 }, { "dial_band", 0.9f }, { "dial_noise", 0.35f }, { "dial_crackle", 0.15f }, { "dial_mix", 0.35f },
                { "verb_on", 1 }, { "verb_size", 0.75f }, { "verb_damp", 0.6f }, { "verb_mix", 0.3f } } },

            { "Total Server Meltdown", {
                { "crush_on", 1 }, { "crush_bits", 3 }, { "crush_rate", 18 }, { "crush_mix", 0.7f },
                { "buf_on", 1 }, { "buf_chance", 0.7f }, { "buf_slice", 60 }, { "buf_dropout", 0.25f },
                { "lobit_on", 1 }, { "lobit_kbps", 8 }, { "lobit_swirl", 1.0f },
                { "corrupt_on", 1 }, { "corrupt_amount", 0.6f }, { "corrupt_bits", 0.8f }, { "corrupt_reverse", 0.5f },
                { "pkt_on", 1 }, { "pkt_loss", 0.5f }, { "pkt_conceal", 0 },
                { "lag_on", 1 }, { "lag_chance", 0.5f }, { "lag_depth", 1.0f },
                { "trem_on", 1 }, { "trem_rate", 9 }, { "trem_depth", 0.8f }, { "trem_shape", 1.0f }, { "trem_pan", 0.5f },
                { "out_gain", -4 } } },
        };
        return p;
    }

    // Resets every parameter to its default, then applies the preset. Call on the message thread.
    inline void apply (juce::AudioProcessorValueTreeState& state, int index)
    {
        const auto& list = all();
        if (! juce::isPositiveAndBelow (index, (int) list.size())) return;

        for (auto* p : state.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rp->getDefaultValue());

        for (auto& [id, value] : list[(size_t) index].values)
            if (auto* rp = state.getParameter (id))
                rp->setValueNotifyingHost (rp->convertTo0to1 (value));

        state.state.setProperty ("preset", index, nullptr);
        Rack::syncFromParams (state);   // the rack shows exactly what the preset uses
    }
}
