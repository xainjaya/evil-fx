#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>
#include "Rack.h"

// Factory presets, user presets (saved to disk) and Randomize.
// Each factory preset starts from the defaults (all modules off, master neutral)
// and only lists what it changes — values are in real units.
namespace Presets
{
    struct Preset
    {
        const char* category;
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    inline const std::vector<Preset>& all()
    {
        static const std::vector<Preset> p {
            { "Start", "Init (Clean)", {} },

            //==================================================================== Studio Tools
            { "Studio Tools", "Punchy Drums", {
                { "trans_on", 1 }, { "trans_attack", 0.7f }, { "trans_sustain", -0.25f },
                { "comp_on", 1 }, { "comp_squash", 0.35f }, { "comp_ratio", 4 }, { "comp_speed", 0.3f },
                { "eq_on", 1 }, { "eq_low", 3 }, { "eq_high", 2 } } },
            { "Studio Tools", "Soft Keys", {
                { "trans_on", 1 }, { "trans_attack", -0.6f }, { "trans_sustain", 0.35f },
                { "eq_on", 1 }, { "eq_high", -3 },
                { "verb_on", 1 }, { "verb_size", 0.6f }, { "verb_mix", 0.25f } } },
            { "Studio Tools", "Funky Wah", {
                { "wah_on", 1 }, { "wah_sens", 0.65f }, { "wah_range", 0.75f }, { "wah_reso", 0.65f },
                { "gloss_on", 1 }, { "gloss_drive", 3 }, { "gloss_mix", 0.5f } } },
            { "Studio Tools", "Vocal Polish", {
                { "eq_on", 1 }, { "eq_low", -3 }, { "eq_mid", 2 }, { "eq_midfreq", 3000 }, { "eq_high", 3 },
                { "comp_on", 1 }, { "comp_squash", 0.45f }, { "comp_ratio", 4 }, { "comp_speed", 0.35f },
                { "gloss_on", 1 }, { "gloss_drive", 3 }, { "gloss_mix", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.45f }, { "verb_mix", 0.12f } } },
            { "Studio Tools", "Wide Mix Bus", {
                { "spread_on", 1 }, { "spread_width", 0.4f }, { "spread_motion", 0.1f },
                { "comp_on", 1 }, { "comp_squash", 0.3f }, { "comp_ratio", 2.5f }, { "comp_speed", 0.6f },
                { "eq_on", 1 }, { "eq_low", 1.5f }, { "eq_high", 2 } } },

            { "Studio Tools", "Jet Flanger Drums", {
                { "trans_on", 1 }, { "trans_attack", 0.5f },
                { "flange_on", 1 }, { "flange_rate", 0.15f }, { "flange_depth", 0.8f }, { "flange_fb", 0.7f }, { "flange_mix", 0.5f },
                { "comp_on", 1 }, { "comp_squash", 0.35f }, { "comp_ratio", 4 }, { "comp_speed", 0.4f } } },
            { "Studio Tools", "Vocal Thickener", {
                { "fshift_on", 1 }, { "fshift_hz", 6 }, { "fshift_spread", 1.0f }, { "fshift_mix", 0.3f },
                { "spread_on", 1 }, { "spread_width", 0.3f }, { "spread_motion", 0.1f },
                { "comp_on", 1 }, { "comp_squash", 0.4f }, { "comp_ratio", 3 }, { "comp_speed", 0.4f },
                { "eq_on", 1 }, { "eq_low", -2 }, { "eq_high", 2 } } },

            //==================================================================== Heavenly
            { "Heavenly", "Crimson Glass Pad", {
                { "chorus_on", 1 }, { "chorus_rate", 0.4f }, { "chorus_depth", 0.55f }, { "chorus_mix", 0.5f },
                { "verb_on", 1 },   { "verb_size", 0.85f }, { "verb_damp", 0.3f }, { "verb_mix", 0.4f },
                { "delay_on", 1 },  { "delay_sync", 4 }, { "delay_fb", 0.4f }, { "delay_tone", 0.75f }, { "delay_mix", 0.22f },
                { "width", 1.35f } } },
            { "Heavenly", "Angel Choir Pad", {
                { "chorus_on", 1 }, { "chorus_rate", 0.25f }, { "chorus_depth", 0.75f }, { "chorus_fb", 0.25f }, { "chorus_mix", 0.6f },
                { "pitch_on", 1 }, { "pitch_semi", 12 }, { "pitch_window", 0.8f }, { "pitch_mix", 0.25f },
                { "verb_on", 1 }, { "verb_size", 1.0f }, { "verb_damp", 0.15f }, { "verb_mix", 0.55f },
                { "width", 1.5f } } },
            { "Heavenly", "Cathedral Wash", {
                { "verb_on", 1 }, { "verb_size", 1.0f }, { "verb_damp", 0.25f }, { "verb_width", 1.0f }, { "verb_mix", 0.7f },
                { "filter_on", 1 }, { "filter_mode", 2 }, { "filter_cutoff", 220 }, { "filter_reso", 0.7f },
                { "comp_on", 1 }, { "comp_squash", 0.35f }, { "comp_ratio", 3 }, { "comp_speed", 0.6f } } },
            { "Heavenly", "Halo Echoes", {
                { "delay_on", 1 }, { "delay_sync", 7 }, { "delay_fb", 0.55f }, { "delay_tone", 0.7f }, { "delay_mix", 0.35f },
                { "chorus_on", 1 }, { "chorus_rate", 0.5f }, { "chorus_depth", 0.3f }, { "chorus_mix", 0.3f },
                { "verb_on", 1 }, { "verb_size", 0.8f }, { "verb_mix", 0.3f } } },
            { "Heavenly", "Holy Water", {
                { "phaser_on", 1 }, { "phaser_rate", 0.18f }, { "phaser_depth", 0.8f }, { "phaser_fb", 0.6f }, { "phaser_mix", 0.55f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 3500 }, { "filter_reso", 2.5f },
                { "filter_lforate", 0.12f }, { "filter_lfodepth", 1.5f },
                { "verb_on", 1 },   { "verb_size", 0.7f }, { "verb_mix", 0.3f } } },
            { "Heavenly", "Screensaver Dream", {
                { "chorus_on", 1 }, { "chorus_rate", 0.15f }, { "chorus_depth", 0.8f }, { "chorus_fb", 0.3f }, { "chorus_mix", 0.6f },
                { "trem_on", 1 },   { "trem_rate", 0.25f }, { "trem_depth", 0.35f }, { "trem_pan", 1 },
                { "verb_on", 1 },   { "verb_size", 1.0f }, { "verb_damp", 0.2f }, { "verb_mix", 0.55f },
                { "gloss_on", 1 },  { "gloss_drive", 3 }, { "gloss_tone", 0.3f } } },
            { "Heavenly", "Wide Angel Pad", {
                { "spread_on", 1 }, { "spread_width", 0.8f }, { "spread_motion", 0.5f },
                { "chorus_on", 1 }, { "chorus_rate", 0.3f }, { "chorus_depth", 0.5f }, { "chorus_mix", 0.4f },
                { "eq_on", 1 }, { "eq_low", -3 }, { "eq_high", 4 },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.45f } } },
            { "Heavenly", "Smile Curve", {
                { "eq_on", 1 }, { "eq_low", 5 }, { "eq_mid", -4 }, { "eq_midfreq", 800 }, { "eq_high", 5 },
                { "comp_on", 1 }, { "comp_squash", 0.3f }, { "comp_ratio", 3 }, { "comp_speed", 0.4f } } },
            { "Heavenly", "Glory Glue", {
                { "comp_on", 1 }, { "comp_squash", 0.45f }, { "comp_ratio", 4 }, { "comp_speed", 0.35f },
                { "gloss_on", 1 }, { "gloss_drive", 4 }, { "gloss_tone", 0.15f }, { "gloss_mix", 0.6f } } },

            { "Heavenly", "Reverse Halo", {
                { "rdelay_on", 1 }, { "rdelay_sync", 8 }, { "rdelay_fb", 0.45f }, { "rdelay_mix", 0.45f },
                { "chorus_on", 1 }, { "chorus_rate", 0.3f }, { "chorus_depth", 0.4f }, { "chorus_mix", 0.3f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_damp", 0.25f }, { "verb_mix", 0.4f },
                { "width", 1.3f } } },
            { "Heavenly", "Angel Spiral", {
                { "fshift_on", 1 }, { "fshift_hz", 2.5f }, { "fshift_fb", 0.5f }, { "fshift_spread", 1.0f }, { "fshift_mix", 0.5f },
                { "delay_on", 1 }, { "delay_sync", 7 }, { "delay_fb", 0.5f }, { "delay_tone", 0.7f }, { "delay_mix", 0.3f },
                { "verb_on", 1 }, { "verb_size", 1.0f }, { "verb_damp", 0.2f }, { "verb_mix", 0.45f } } },

            //==================================================================== Ambient Pads
            { "Ambient Pads", "Giant Ambient Pad", {
                { "blur_on", 1 }, { "blur_size", 0.9f }, { "blur_smear", 0.85f }, { "blur_mix", 0.6f },
                { "swell_on", 1 }, { "swell_attack", 900 }, { "swell_sens", 0.6f },
                { "spread_on", 1 }, { "spread_width", 0.7f }, { "spread_motion", 0.4f },
                { "verb_on", 1 }, { "verb_size", 1.0f }, { "verb_damp", 0.3f }, { "verb_mix", 0.4f } } },
            { "Ambient Pads", "Shimmer Heaven", {
                { "blur_on", 1 }, { "blur_size", 0.8f }, { "blur_smear", 0.8f }, { "blur_shimmer", 0.6f }, { "blur_mix", 0.55f },
                { "chorus_on", 1 }, { "chorus_rate", 0.2f }, { "chorus_depth", 0.4f }, { "chorus_mix", 0.3f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.45f } } },
            { "Ambient Pads", "Glacier", {
                { "blur_on", 1 }, { "blur_size", 1.0f }, { "blur_smear", 0.92f }, { "blur_shimmer", 0.3f }, { "blur_mix", 0.65f },
                { "eq_on", 1 }, { "eq_low", -4 }, { "eq_high", 3 },
                { "spread_on", 1 }, { "spread_width", 0.8f }, { "spread_motion", 0.3f } } },
            { "Ambient Pads", "Endless Swell", {
                { "swell_on", 1 }, { "swell_attack", 1600 }, { "swell_sens", 0.6f },
                { "blur_on", 1 }, { "blur_size", 0.8f }, { "blur_smear", 0.8f }, { "blur_mix", 0.5f },
                { "delay_on", 1 }, { "delay_sync", 7 }, { "delay_fb", 0.5f }, { "delay_tone", 0.6f }, { "delay_mix", 0.25f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.45f } } },
            { "Ambient Pads", "Angel Swell Strings", {
                { "swell_on", 1 }, { "swell_attack", 700 }, { "swell_sens", 0.55f },
                { "chorus_on", 1 }, { "chorus_rate", 0.35f }, { "chorus_depth", 0.5f }, { "chorus_mix", 0.4f },
                { "blur_on", 1 }, { "blur_size", 0.7f }, { "blur_smear", 0.7f }, { "blur_shimmer", 0.4f }, { "blur_mix", 0.4f },
                { "spread_on", 1 }, { "spread_width", 0.6f } } },
            { "Ambient Pads", "Underwater Pad", {
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 900 }, { "filter_reso", 1.5f },
                { "filter_lforate", 0.1f }, { "filter_lfodepth", 0.8f },
                { "chorus_on", 1 }, { "chorus_rate", 0.3f }, { "chorus_depth", 0.7f }, { "chorus_mix", 0.5f },
                { "blur_on", 1 }, { "blur_size", 0.7f }, { "blur_smear", 0.7f }, { "blur_mix", 0.5f } } },
            { "Ambient Pads", "Lo-Fi Dream Pad", {
                { "blur_on", 1 }, { "blur_size", 0.6f }, { "blur_smear", 0.7f }, { "blur_mix", 0.45f },
                { "jit_on", 1 }, { "jit_wow", 0.4f }, { "jit_flutter", 0.2f }, { "jit_drift", 0.3f },
                { "vinyl_on", 1 }, { "vinyl_crackle", 0.3f }, { "vinyl_wear", 0.4f }, { "vinyl_wow", 0.15f },
                { "eq_on", 1 }, { "eq_high", -4 } } },
            { "Ambient Pads", "Cassette Ambient", {
                { "jit_on", 1 }, { "jit_wow", 0.5f }, { "jit_flutter", 0.3f }, { "jit_drift", 0.4f },
                { "gloss_on", 1 }, { "gloss_drive", 6 }, { "gloss_tone", -0.3f },
                { "blur_on", 1 }, { "blur_size", 0.7f }, { "blur_smear", 0.75f }, { "blur_mix", 0.5f },
                { "verb_on", 1 }, { "verb_size", 0.7f }, { "verb_mix", 0.3f } } },
            { "Ambient Pads", "Dial-Up Dreamscape", {
                { "dial_on", 1 }, { "dial_band", 0.5f }, { "dial_noise", 0.25f }, { "dial_crackle", 0.2f }, { "dial_mix", 0.4f },
                { "blur_on", 1 }, { "blur_size", 0.9f }, { "blur_smear", 0.85f }, { "blur_mix", 0.6f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.35f } } },
            { "Ambient Pads", "Bitcrushed Cathedral", {
                { "crush_on", 1 }, { "crush_bits", 8 }, { "crush_rate", 3 }, { "crush_mix", 0.5f },
                { "blur_on", 1 }, { "blur_size", 0.8f }, { "blur_smear", 0.8f }, { "blur_mix", 0.55f },
                { "verb_on", 1 }, { "verb_size", 1.0f }, { "verb_mix", 0.5f } } },
            { "Ambient Pads", "Sidechained Cloud", {
                { "pump_on", 1 }, { "pump_div", 1 }, { "pump_depth", 0.7f }, { "pump_shape", 0.5f },
                { "blur_on", 1 }, { "blur_size", 0.8f }, { "blur_smear", 0.8f }, { "blur_mix", 0.6f },
                { "verb_on", 1 }, { "verb_size", 0.8f }, { "verb_mix", 0.35f } } },
            { "Ambient Pads", "Nightcore Halo", {
                { "pitch_on", 1 }, { "pitch_semi", 12 }, { "pitch_window", 0.8f }, { "pitch_mix", 0.3f },
                { "blur_on", 1 }, { "blur_size", 0.85f }, { "blur_smear", 0.85f }, { "blur_shimmer", 0.5f }, { "blur_mix", 0.6f } } },

            { "Ambient Pads", "Backwards Dream", {
                { "swell_on", 1 }, { "swell_attack", 800 }, { "swell_sens", 0.55f },
                { "rdelay_on", 1 }, { "rdelay_sync", 9 }, { "rdelay_fb", 0.5f }, { "rdelay_mix", 0.5f },
                { "blur_on", 1 }, { "blur_size", 0.8f }, { "blur_smear", 0.75f }, { "blur_mix", 0.45f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.35f } } },
            { "Ambient Pads", "Talking Clouds", {
                { "talk_on", 1 }, { "talk_vowel", 0.3f }, { "talk_rate", 0.15f }, { "talk_sweep", 0.8f }, { "talk_mix", 0.6f }, { "talk_voice", 2 },
                { "blur_on", 1 }, { "blur_size", 0.85f }, { "blur_smear", 0.8f }, { "blur_shimmer", 0.25f }, { "blur_mix", 0.55f },
                { "verb_on", 1 }, { "verb_size", 0.95f }, { "verb_mix", 0.4f } } },
            { "Ambient Pads", "Jet Stream Pad", {
                { "flange_on", 1 }, { "flange_rate", 0.05f }, { "flange_depth", 1.0f }, { "flange_fb", 0.8f }, { "flange_mix", 0.5f },
                { "blur_on", 1 }, { "blur_size", 0.75f }, { "blur_smear", 0.7f }, { "blur_mix", 0.45f },
                { "spread_on", 1 }, { "spread_width", 0.6f }, { "spread_motion", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.85f }, { "verb_mix", 0.35f } } },

            //==================================================================== Internet Era
            { "Internet Era", "Dial-Up 1999", {
                { "dial_on", 1 }, { "dial_band", 0.15f }, { "dial_noise", 0.45f }, { "dial_crackle", 0.5f }, { "dial_modem", 1 },
                { "crush_on", 1 }, { "crush_bits", 10 }, { "crush_rate", 2 } } },
            { "Internet Era", "Instant Messenger Call", {
                { "dial_on", 1 }, { "dial_band", 0.45f }, { "dial_noise", 0.15f }, { "dial_crackle", 0.1f },
                { "lobit_on", 1 }, { "lobit_kbps", 16 }, { "lobit_swirl", 0.6f },
                { "buf_on", 1 }, { "buf_chance", 0.12f }, { "buf_slice", 80 }, { "buf_dropout", 0.6f } } },
            { "Internet Era", "Buffering...", {
                { "buf_on", 1 }, { "buf_chance", 0.45f }, { "buf_slice", 160 }, { "buf_dropout", 0.35f },
                { "lobit_on", 1 }, { "lobit_kbps", 40 }, { "lobit_swirl", 0.4f } } },
            { "Internet Era", "Low Bitrate Rip", {
                { "lobit_on", 1 }, { "lobit_kbps", 24 }, { "lobit_swirl", 0.75f },
                { "filter_on", 1 }, { "filter_mode", 2 }, { "filter_cutoff", 90 }, { "filter_reso", 0.7f } } },
            { "Internet Era", "Mystery File (128kbps)", {
                { "lobit_on", 1 }, { "lobit_kbps", 128 }, { "lobit_swirl", 0.6f },
                { "corrupt_on", 1 }, { "corrupt_amount", 0.15f }, { "corrupt_bits", 0.2f }, { "corrupt_reverse", 0.0f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 15000 }, { "filter_reso", 0.6f } } },
            { "Internet Era", "Profile Page Song", {
                { "lobit_on", 1 }, { "lobit_kbps", 48 }, { "lobit_swirl", 0.45f },
                { "comp_on", 1 }, { "comp_squash", 0.6f }, { "comp_ratio", 8 }, { "comp_speed", 0.2f },
                { "spk_on", 1 }, { "spk_model", 1 }, { "spk_drive", 0.1f }, { "spk_mix", 0.7f } } },
            { "Internet Era", "Webring Radio", {
                { "lobit_on", 1 }, { "lobit_kbps", 56 }, { "lobit_swirl", 0.35f },
                { "gloss_on", 1 }, { "gloss_drive", 9 }, { "gloss_tone", -0.2f },
                { "filter_on", 1 }, { "filter_mode", 1 }, { "filter_cutoff", 1600 }, { "filter_reso", 0.8f },
                { "dial_on", 1 }, { "dial_band", 0.85f }, { "dial_noise", 0.25f }, { "dial_crackle", 0.35f }, { "dial_mix", 0.5f },
                { "width", 0.4f } } },
            { "Internet Era", "RealAudio 1998", {
                { "codec_on", 1 }, { "codec_rate", 2 }, { "codec_type", 3 }, { "codec_alias", 0.3f },
                { "lobit_on", 1 }, { "lobit_kbps", 20 }, { "lobit_swirl", 0.55f },
                { "buf_on", 1 }, { "buf_chance", 0.15f }, { "buf_slice", 220 }, { "buf_dropout", 0.1f },
                { "jit_on", 1 }, { "jit_wow", 0.15f }, { "jit_flutter", 0.1f }, { "jit_drift", 0.4f } } },
            { "Internet Era", "VoIP Breakup", {
                { "codec_on", 1 }, { "codec_rate", 3 }, { "codec_type", 1 }, { "codec_alias", 0.6f },
                { "pkt_on", 1 }, { "pkt_loss", 0.45f }, { "pkt_size", 20 }, { "pkt_conceal", 0 },
                { "spk_on", 1 }, { "spk_model", 2 }, { "spk_drive", 0.3f } } },
            { "Internet Era", "Party Chat Headset", {
                { "codec_on", 1 }, { "codec_rate", 1 }, { "codec_type", 3 }, { "codec_alias", 0.2f },
                { "pkt_on", 1 }, { "pkt_loss", 0.15f }, { "pkt_size", 30 }, { "pkt_conceal", 2 },
                { "comp_on", 1 }, { "comp_squash", 0.7f }, { "comp_ratio", 10 }, { "comp_speed", 0.15f },
                { "spk_on", 1 }, { "spk_model", 3 }, { "spk_drive", 0.4f } } },
            { "Internet Era", "Corrupted Download", {
                { "corrupt_on", 1 }, { "corrupt_amount", 0.45f }, { "corrupt_bits", 0.55f }, { "corrupt_reverse", 0.35f },
                { "buf_on", 1 }, { "buf_chance", 0.2f }, { "buf_slice", 90 }, { "buf_dropout", 0.5f },
                { "codec_on", 1 }, { "codec_rate", 1 }, { "codec_type", 0 }, { "codec_alias", 0.8f } } },
            { "Internet Era", "Lag Spike Lobby", {
                { "lag_on", 1 }, { "lag_chance", 0.55f }, { "lag_length", 700 }, { "lag_depth", 0.9f },
                { "jit_on", 1 }, { "jit_wow", 0.2f }, { "jit_flutter", 0.25f }, { "jit_drift", 0.3f },
                { "crush_on", 1 }, { "crush_bits", 9 }, { "crush_rate", 3 },
                { "verb_on", 1 }, { "verb_size", 0.5f }, { "verb_mix", 0.18f } } },
            { "Internet Era", "Robot Narrator", {
                { "tts_on", 1 }, { "tts_pitch", 95 }, { "tts_metal", 0.75f }, { "tts_buzz", 0.45f }, { "tts_mix", 0.85f },
                { "codec_on", 1 }, { "codec_rate", 1 }, { "codec_type", 0 }, { "codec_alias", 0.4f },
                { "eq_on", 1 }, { "eq_low", -6 }, { "eq_mid", 4 }, { "eq_midfreq", 1800 } } },
            { "Internet Era", "Nightcore 2009", {
                { "pitch_on", 1 }, { "pitch_semi", 4 }, { "pitch_window", 0.4f },
                { "lobit_on", 1 }, { "lobit_kbps", 96 }, { "lobit_swirl", 0.4f },
                { "comp_on", 1 }, { "comp_squash", 0.5f }, { "comp_ratio", 6 }, { "comp_speed", 0.3f } } },
            { "Internet Era", "Polyphonic Ringtone", {
                { "ring_on", 1 }, { "ring_freq", 660 }, { "ring_shape", 0.6f }, { "ring_warble", 0.8f }, { "ring_mix", 0.45f },
                { "codec_on", 1 }, { "codec_rate", 3 }, { "codec_type", 0 }, { "codec_alias", 0.7f },
                { "spk_on", 1 }, { "spk_model", 2 }, { "spk_drive", 0.35f } } },

            { "Internet Era", "Smiley Says Hi", {
                { "talk_on", 1 }, { "talk_vowel", 0.2f }, { "talk_rate", 3 }, { "talk_sweep", 0.7f }, { "talk_mix", 0.8f }, { "talk_voice", 2 },
                { "pitch_on", 1 }, { "pitch_semi", 5 }, { "pitch_window", 0.4f }, { "pitch_mix", 0.6f },
                { "chorus_on", 1 }, { "chorus_rate", 0.8f }, { "chorus_depth", 0.3f }, { "chorus_mix", 0.3f } } },
            { "Internet Era", "Haunted Webcam", {
                { "fshift_on", 1 }, { "fshift_hz", -40 }, { "fshift_fb", 0.3f }, { "fshift_mix", 0.55f },
                { "dial_on", 1 }, { "dial_band", 0.45f }, { "dial_noise", 0.25f }, { "dial_crackle", 0.2f },
                { "pkt_on", 1 }, { "pkt_loss", 0.15f }, { "pkt_conceal", 0 } } },
            { "Internet Era", "Talk Box Modem", {
                { "talk_on", 1 }, { "talk_vowel", 0.5f }, { "talk_rate", 6 }, { "talk_sweep", 1.0f }, { "talk_voice", 1 },
                { "dial_on", 1 }, { "dial_band", 0.35f }, { "dial_noise", 0.3f }, { "dial_crackle", 0.3f }, { "dial_modem", 1 },
                { "crush_on", 1 }, { "crush_bits", 10 }, { "crush_rate", 3 }, { "crush_mix", 0.5f } } },

            //==================================================================== Rhythm & Glitch (BPM synced)
            { "Rhythm & Glitch", "Beat Repeat", {
                { "glitch_on", 1 }, { "glitch_div", 3 }, { "glitch_style", 1 }, { "glitch_chance", 0.35f } } },
            { "Rhythm & Glitch", "Stutter Edit", {
                { "glitch_on", 1 }, { "glitch_div", 1 }, { "glitch_style", 2 }, { "glitch_chance", 0.45f },
                { "filter_on", 1 }, { "filter_mode", 2 }, { "filter_cutoff", 120 }, { "filter_lforate", 0.5f } } },
            { "Rhythm & Glitch", "Tape Stop Drops", {
                { "glitch_on", 1 }, { "glitch_div", 0 }, { "glitch_style", 4 }, { "glitch_chance", 0.3f },
                { "verb_on", 1 }, { "verb_size", 0.5f }, { "verb_mix", 0.15f } } },
            { "Rhythm & Glitch", "Glitch Mayhem", {
                { "glitch_on", 1 }, { "glitch_div", 3 }, { "glitch_style", 0 }, { "glitch_chance", 0.6f },
                { "crush_on", 1 }, { "crush_bits", 8 }, { "crush_rate", 4 }, { "crush_mix", 0.6f },
                { "delay_on", 1 }, { "delay_sync", 3 }, { "delay_fb", 0.3f }, { "delay_mix", 0.2f } } },
            { "Rhythm & Glitch", "Sidechain Pump", {
                { "pump_on", 1 }, { "pump_div", 1 }, { "pump_depth", 0.8f }, { "pump_shape", 0.4f } } },
            { "Rhythm & Glitch", "Pumping Heaven", {
                { "pump_on", 1 }, { "pump_div", 1 }, { "pump_depth", 0.65f }, { "pump_shape", 0.55f },
                { "spread_on", 1 }, { "spread_width", 0.7f }, { "spread_motion", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.85f }, { "verb_mix", 0.4f },
                { "delay_on", 1 }, { "delay_sync", 4 }, { "delay_fb", 0.35f }, { "delay_mix", 0.2f } } },
            { "Rhythm & Glitch", "Trance Gate Heaven", {
                { "gate_on", 1 }, { "gate_pattern", 3 }, { "gate_div", 3 }, { "gate_depth", 1.0f }, { "gate_smooth", 0.2f },
                { "delay_on", 1 }, { "delay_sync", 4 }, { "delay_fb", 0.45f }, { "delay_tone", 0.7f }, { "delay_mix", 0.3f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_mix", 0.4f } } },
            { "Rhythm & Glitch", "Offbeat Chop", {
                { "gate_on", 1 }, { "gate_pattern", 1 }, { "gate_div", 1 }, { "gate_depth", 1.0f }, { "gate_smooth", 0.1f },
                { "comp_on", 1 }, { "comp_squash", 0.4f }, { "comp_ratio", 4 }, { "comp_speed", 0.2f } } },
            { "Rhythm & Glitch", "Random Gate + Repeat", {
                { "gate_on", 1 }, { "gate_pattern", 7 }, { "gate_div", 3 }, { "gate_depth", 0.85f }, { "gate_smooth", 0.25f },
                { "glitch_on", 1 }, { "glitch_div", 3 }, { "glitch_style", 1 }, { "glitch_chance", 0.25f } } },
            { "Rhythm & Glitch", "8-Bit Arcade Lobby", {
                { "crush_on", 1 }, { "crush_bits", 5 }, { "crush_rate", 6 },
                { "delay_on", 1 }, { "delay_sync", 3 }, { "delay_fb", 0.45f }, { "delay_tone", 0.5f }, { "delay_mix", 0.28f },
                { "verb_on", 1 }, { "verb_size", 0.4f }, { "verb_mix", 0.15f } } },

            { "Rhythm & Glitch", "Reverse Riser", {
                { "rdelay_on", 1 }, { "rdelay_sync", 6 }, { "rdelay_fb", 0.6f }, { "rdelay_mix", 0.6f },
                { "gate_on", 1 }, { "gate_pattern", 3 }, { "gate_div", 3 }, { "gate_depth", 0.6f }, { "gate_smooth", 0.3f },
                { "pump_on", 1 }, { "pump_div", 1 }, { "pump_depth", 0.5f } } },
            { "Rhythm & Glitch", "Talk Box Funk", {
                { "trans_on", 1 }, { "trans_attack", 0.5f },
                { "talk_on", 1 }, { "talk_vowel", 0.1f }, { "talk_rate", 4 }, { "talk_sweep", 0.9f }, { "talk_voice", 0 },
                { "gloss_on", 1 }, { "gloss_drive", 8 }, { "gloss_mix", 0.6f },
                { "pump_on", 1 }, { "pump_div", 2 }, { "pump_depth", 0.45f } } },
            { "Rhythm & Glitch", "Flanged Pump", {
                { "flange_on", 1 }, { "flange_rate", 0.5f }, { "flange_depth", 0.7f }, { "flange_fb", 0.8f }, { "flange_mix", 0.6f },
                { "pump_on", 1 }, { "pump_div", 2 }, { "pump_depth", 0.7f }, { "pump_shape", 0.5f },
                { "comp_on", 1 }, { "comp_squash", 0.3f }, { "comp_ratio", 3 } } },

            //==================================================================== Lo-Fi & Speakers
            { "Lo-Fi & Speakers", "Lo-Fi Study Stream", {
                { "vinyl_on", 1 }, { "vinyl_crackle", 0.3f }, { "vinyl_wear", 0.25f }, { "vinyl_wow", 0.1f },
                { "jit_on", 1 }, { "jit_wow", 0.35f }, { "jit_flutter", 0.15f }, { "jit_drift", 0.3f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 5500 }, { "filter_reso", 0.8f },
                { "crush_on", 1 }, { "crush_bits", 12 }, { "crush_rate", 2 }, { "crush_mix", 0.6f },
                { "comp_on", 1 }, { "comp_squash", 0.4f }, { "comp_ratio", 3 }, { "comp_speed", 0.5f },
                { "verb_on", 1 }, { "verb_size", 0.5f }, { "verb_mix", 0.2f } } },
            { "Lo-Fi & Speakers", "Vinyl Sunday", {
                { "vinyl_on", 1 }, { "vinyl_crackle", 0.5f }, { "vinyl_wear", 0.35f }, { "vinyl_wow", 0.25f },
                { "eq_on", 1 }, { "eq_low", 2 }, { "eq_high", -3 },
                { "comp_on", 1 }, { "comp_squash", 0.3f }, { "comp_ratio", 3 }, { "comp_speed", 0.6f } } },
            { "Lo-Fi & Speakers", "Dusty Sample", {
                { "vinyl_on", 1 }, { "vinyl_crackle", 0.75f }, { "vinyl_wear", 0.7f }, { "vinyl_wow", 0.4f },
                { "crush_on", 1 }, { "crush_bits", 12 }, { "crush_rate", 3 }, { "crush_mix", 0.7f },
                { "pump_on", 1 }, { "pump_div", 1 }, { "pump_depth", 0.35f }, { "pump_shape", 0.6f } } },
            { "Lo-Fi & Speakers", "Warped Cassette Rip", {
                { "jit_on", 1 }, { "jit_wow", 0.6f }, { "jit_flutter", 0.45f }, { "jit_drift", 0.5f },
                { "gloss_on", 1 }, { "gloss_drive", 8 }, { "gloss_tone", -0.45f },
                { "filter_on", 1 }, { "filter_mode", 0 }, { "filter_cutoff", 7000 }, { "filter_reso", 0.7f },
                { "spk_on", 1 }, { "spk_model", 5 }, { "spk_drive", 0.2f }, { "spk_mix", 0.6f } } },
            { "Lo-Fi & Speakers", "Glossy Tape Stack", {
                { "gloss_on", 1 }, { "gloss_drive", 14 }, { "gloss_tone", -0.35f }, { "gloss_mix", 0.8f },
                { "chorus_on", 1 }, { "chorus_rate", 1.2f }, { "chorus_depth", 0.15f }, { "chorus_mix", 0.35f },
                { "delay_on", 1 }, { "delay_sync", 6 }, { "delay_fb", 0.3f }, { "delay_tone", 0.35f }, { "delay_pp", 0 }, { "delay_mix", 0.2f } } },
            { "Lo-Fi & Speakers", "Laptop Speaker Stream", {
                { "lobit_on", 1 }, { "lobit_kbps", 64 }, { "lobit_swirl", 0.3f },
                { "spk_on", 1 }, { "spk_model", 1 }, { "spk_drive", 0.15f },
                { "lag_on", 1 }, { "lag_chance", 0.15f }, { "lag_length", 500 }, { "lag_depth", 0.6f } } },
            { "Lo-Fi & Speakers", "PC Speaker Choir", {
                { "spk_on", 1 }, { "spk_model", 0 }, { "spk_drive", 0.6f },
                { "codec_on", 1 }, { "codec_rate", 4 }, { "codec_type", 0 }, { "codec_alias", 1.0f },
                { "trem_on", 1 }, { "trem_rate", 7 }, { "trem_depth", 0.4f }, { "trem_shape", 0.8f } } },
            { "Lo-Fi & Speakers", "Flash Game Menu", {
                { "codec_on", 1 }, { "codec_rate", 2 }, { "codec_type", 3 }, { "codec_alias", 0.5f },
                { "spk_on", 1 }, { "spk_model", 1 }, { "spk_drive", 0.2f },
                { "verb_on", 1 }, { "verb_size", 0.35f }, { "verb_mix", 0.15f } } },
            { "Lo-Fi & Speakers", "Y2K Mall Speaker", {
                { "filter_on", 1 }, { "filter_mode", 1 }, { "filter_cutoff", 1100 }, { "filter_reso", 0.9f },
                { "gloss_on", 1 }, { "gloss_drive", 12 }, { "gloss_tone", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.9f }, { "verb_damp", 0.7f }, { "verb_mix", 0.45f },
                { "width", 0.2f } } },
            { "Lo-Fi & Speakers", "CRT TV at 3AM", {
                { "spk_on", 1 }, { "spk_model", 4 }, { "spk_drive", 0.25f },
                { "dial_on", 1 }, { "dial_band", 0.9f }, { "dial_noise", 0.35f }, { "dial_crackle", 0.15f }, { "dial_mix", 0.35f },
                { "verb_on", 1 }, { "verb_size", 0.75f }, { "verb_damp", 0.6f }, { "verb_mix", 0.3f } } },

            { "Lo-Fi & Speakers", "Flanged Cassette", {
                { "flange_on", 1 }, { "flange_rate", 0.08f }, { "flange_depth", 0.4f }, { "flange_fb", 0.3f }, { "flange_mix", 0.4f },
                { "jit_on", 1 }, { "jit_wow", 0.35f }, { "jit_flutter", 0.25f }, { "jit_drift", 0.2f },
                { "vinyl_on", 1 }, { "vinyl_crackle", 0.2f }, { "vinyl_wear", 0.45f }, { "vinyl_wow", 0.1f },
                { "eq_on", 1 }, { "eq_high", -3 } } },
            { "Lo-Fi & Speakers", "Boombox Talk Box", {
                { "talk_on", 1 }, { "talk_vowel", 0.35f }, { "talk_rate", 2 }, { "talk_sweep", 0.6f }, { "talk_mix", 0.75f },
                { "spk_on", 1 }, { "spk_model", 5 }, { "spk_drive", 0.4f },
                { "verb_on", 1 }, { "verb_size", 0.3f }, { "verb_mix", 0.12f } } },

            //==================================================================== Chaos
            { "Chaos", "Total Server Meltdown", {
                { "crush_on", 1 }, { "crush_bits", 3 }, { "crush_rate", 18 }, { "crush_mix", 0.7f },
                { "buf_on", 1 }, { "buf_chance", 0.7f }, { "buf_slice", 60 }, { "buf_dropout", 0.25f },
                { "lobit_on", 1 }, { "lobit_kbps", 8 }, { "lobit_swirl", 1.0f },
                { "corrupt_on", 1 }, { "corrupt_amount", 0.6f }, { "corrupt_bits", 0.8f }, { "corrupt_reverse", 0.5f },
                { "pkt_on", 1 }, { "pkt_loss", 0.5f }, { "pkt_conceal", 0 },
                { "lag_on", 1 }, { "lag_chance", 0.5f }, { "lag_depth", 1.0f },
                { "trem_on", 1 }, { "trem_rate", 9 }, { "trem_depth", 0.8f }, { "trem_shape", 1.0f }, { "trem_pan", 0.5f },
                { "out_gain", -4 } } },
            { "Chaos", "Robot Uprising", {
                { "ring_on", 1 }, { "ring_freq", 90 }, { "ring_shape", 0.3f }, { "ring_mix", 0.7f },
                { "pkt_on", 1 }, { "pkt_loss", 0.3f }, { "pkt_size", 15 }, { "pkt_conceal", 0 },
                { "pitch_on", 1 }, { "pitch_semi", -5 }, { "pitch_window", 0.2f }, { "pitch_mix", 0.5f },
                { "glitch_on", 1 }, { "glitch_div", 3 }, { "glitch_style", 2 }, { "glitch_chance", 0.3f } } },
            { "Chaos", "Blue Screen", {
                { "corrupt_on", 1 }, { "corrupt_amount", 0.8f }, { "corrupt_bits", 1.0f }, { "corrupt_reverse", 0.3f },
                { "glitch_on", 1 }, { "glitch_div", 5 }, { "glitch_style", 2 }, { "glitch_chance", 0.5f },
                { "codec_on", 1 }, { "codec_rate", 4 }, { "codec_type", 1 }, { "codec_alias", 1.0f },
                { "out_gain", -6 } } },
            { "Chaos", "Alien Transmission", {
                { "fshift_on", 1 }, { "fshift_hz", 700 }, { "fshift_fb", 0.6f }, { "fshift_spread", 0.7f }, { "fshift_mix", 0.8f },
                { "ring_on", 1 }, { "ring_freq", 60 }, { "ring_mix", 0.4f },
                { "pkt_on", 1 }, { "pkt_loss", 0.3f }, { "pkt_conceal", 0 },
                { "out_gain", -3 } } },
            { "Chaos", "Haunted Server", {
                { "fshift_on", 1 }, { "fshift_hz", -150 }, { "fshift_fb", 0.4f }, { "fshift_mix", 0.6f },
                { "corrupt_on", 1 }, { "corrupt_amount", 0.4f }, { "corrupt_bits", 0.5f }, { "corrupt_reverse", 0.4f },
                { "rdelay_on", 1 }, { "rdelay_sync", 4 }, { "rdelay_fb", 0.7f }, { "rdelay_mix", 0.6f } } },
            { "Chaos", "Smiley Meltdown", {
                { "talk_on", 1 }, { "talk_rate", 9 }, { "talk_sweep", 1.0f }, { "talk_voice", 2 },
                { "fshift_on", 1 }, { "fshift_hz", 300 }, { "fshift_spread", 1.0f }, { "fshift_mix", 0.5f },
                { "flange_on", 1 }, { "flange_rate", 2.5f }, { "flange_fb", -0.9f }, { "flange_mix", 0.7f },
                { "glitch_on", 1 }, { "glitch_div", 4 }, { "glitch_style", 0 }, { "glitch_chance", 0.4f },
                { "rdelay_on", 1 }, { "rdelay_sync", 3 }, { "rdelay_fb", 0.5f }, { "rdelay_mix", 0.4f },
                { "out_gain", -1 } } },
        };
        return p;
    }

    inline void resetAll (juce::AudioProcessorValueTreeState& state)
    {
        for (auto* p : state.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    }

    // Resets every parameter to its default, then applies the preset. Call on the message thread.
    inline void apply (juce::AudioProcessorValueTreeState& state, int index)
    {
        const auto& list = all();
        if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
        resetAll (state);
        for (auto& [id, value] : list[(size_t) index].values)
            if (auto* rp = state.getParameter (id))
                rp->setValueNotifyingHost (rp->convertTo0to1 (value));

        state.state.setProperty ("preset", index, nullptr);
        state.state.setProperty ("presetName", list[(size_t) index].name, nullptr);
        Rack::syncFromParams (state);   // the rack shows exactly what the preset uses
    }

    //==========================================================================
    // Randomize: switches on 2-5 random effects with random (but sane) settings.
    // Master stays neutral and the limiter stays on, so it can never blow up.
    inline void randomize (juce::AudioProcessorValueTreeState& state)
    {
        juce::Random rng;
        resetAll (state);
        const int count = 2 + rng.nextInt (4);
        juce::Array<int> picked;
        while (picked.size() < count)
        {
            const int m = rng.nextInt (Rack::numModules);
            if (! picked.contains (m)) picked.add (m);
        }
        for (int m : picked)
        {
            const juce::String onId (Rack::onId (m));
            const juce::String prefix = onId.upToLastOccurrenceOf ("_on", false, false) + "_";
            for (auto* p : state.processor.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                {
                    const auto id = rp->getParameterID();
                    if (! id.startsWith (prefix) || id == onId) continue;
                    float v = rng.nextFloat();
                    if (id.endsWith ("_mix")) v = 0.35f + 0.65f * v;   // keep every chosen effect audible
                    rp->setValueNotifyingHost (v);
                }
            if (auto* on = state.getParameter (onId)) on->setValueNotifyingHost (1.0f);
        }
        state.state.setProperty ("preset", 0, nullptr);
        state.state.setProperty ("presetName", "Randomized", nullptr);
        Rack::syncFromParams (state);
    }

    //==========================================================================
    // User presets: one XML file per preset in Documents/Evil FX/Presets
    inline juce::File userFolder()
    {
        auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Evil FX").getChildFile ("Presets");
        f.createDirectory();
        return f;
    }

    inline juce::Array<juce::File> userPresets()
    {
        auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*.evilfx");
        std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
                   { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
        return files;
    }

    inline bool saveUser (juce::AudioProcessorValueTreeState& state, const juce::String& name)
    {
        const auto clean = juce::File::createLegalFileName (name.trim());
        if (clean.isEmpty()) return false;
        state.state.setProperty ("presetName", clean, nullptr);
        auto xml = state.copyState().createXml();
        return xml != nullptr && xml->writeTo (userFolder().getChildFile (clean + ".evilfx"));
    }

    inline bool loadUser (juce::AudioProcessorValueTreeState& state, const juce::File& file)
    {
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr || ! xml->hasTagName (state.state.getType())) return false;
        state.replaceState (juce::ValueTree::fromXml (*xml));
        state.state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
        if (! Rack::isValid (state.state)) Rack::syncFromParams (state);
        else Rack::upgrade (state.state);
        return true;
    }
}
