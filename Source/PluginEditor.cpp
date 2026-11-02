#include "PluginEditor.h"
#include <map>

using namespace aero;

#ifndef EVIL_VERSION
 #define EVIL_VERSION "1.0.0"
#endif

namespace
{
    constexpr int headerH = 96, toolbarH = 34, statusH = 26, margin = 14, gap = 10, stripH = 150, cols = 5, footerH = 30;
    const char* kMarquee =
        "  *** Welcome to Evil FX v" EVIL_VERSION " ***  hit + ADD FX to build your chain  ~  "
        "drag a card's title bar to change the order  ~  right-click a title bar for more  ~  Ctrl+Z undoes anything  ~  "
        "A / B lets you compare two sounds  ~  set the LFO SYNC dropdowns to lock wobbles to your tempo  ~  "
        "try Blur + Freeze for giant pads  ~  NEW: Auto-Tune, Vocoder, Fuzz, Noise Gate  ~  hit BROWSE to search presets and star favourites  ~  "
        "hit RANDOM for happy accidents  ~  pick a THEME you like  ~  double-click any knob to reset it  ~  "
        "the safety limiter keeps your speakers alive  ~  free & open source forever  ~  sign the guestbook @ xainjaya.com  ";

    std::vector<CardSpec> moduleSpecs()
    {
        std::vector<CardSpec> s {
            { 0, "Noise Gate", "clean", false, "ngate_on",
              { { "ngate_thresh", "Threshold" }, { "ngate_range", "Range" }, { "ngate_release", "Release" } }, { "ngate_key" }, {} },
            { 0, "Transient Shaper", "punch", false, "trans_on",
              { { "trans_attack", "Attack" }, { "trans_sustain", "Sustain" }, { "trans_mix", "Mix" } }, {}, {} },
            { 0, "Auto-Tune", "2009", false, "tune_on",
              { { "tune_speed", "Speed" }, { "tune_amount", "Amount" }, { "tune_mix", "Mix" } }, { "tune_key", "tune_scale" }, {} },
            { 0, "Filter", "sweep", false, "filter_on",
              { { "filter_cutoff", "Cutoff" }, { "filter_reso", "Reso" }, { "filter_lforate", "LFO Rate" }, { "filter_lfodepth", "LFO Depth" } },
              { "filter_mode", "filter_sync" }, {} },
            { 0, "Auto-Wah", "envelope", false, "wah_on",
              { { "wah_sens", "Sens" }, { "wah_range", "Range" }, { "wah_reso", "Reso" }, { "wah_mix", "Mix" } }, {}, {} },
            { 0, "Talk Box", "vowels", false, "talk_on",
              { { "talk_vowel", "Vowel" }, { "talk_rate", "Rate" }, { "talk_sweep", "Sweep" }, { "talk_mix", "Mix" } },
              { "talk_voice", "talk_sync" }, {} },
            { 0, "EQ", "3-band", false, "eq_on",
              { { "eq_low", "Low" }, { "eq_mid", "Mid" }, { "eq_midfreq", "Mid Freq" }, { "eq_high", "High" } }, {}, {} },
            { 0, "Saturation", "drive", false, "gloss_on",
              { { "gloss_drive", "Drive" }, { "gloss_tone", "Tone" }, { "gloss_mix", "Mix" } }, {}, {} },
            { 0, "Fuzz", "distortion", false, "fuzz_on",
              { { "fuzz_drive", "Drive" }, { "fuzz_tone", "Tone" }, { "fuzz_gate", "Gate" }, { "fuzz_mix", "Mix" } }, { "fuzz_type" }, {} },
            { 0, "Pitch Shifter", "", true, "pitch_on",
              { { "pitch_semi", "Pitch" }, { "pitch_window", "Grain" }, { "pitch_mix", "Mix" } }, {}, {} },
            { 0, "Ring Modulator", "", true, "ring_on",
              { { "ring_freq", "Freq" }, { "ring_shape", "Shape" }, { "ring_warble", "Arp" }, { "ring_mix", "Mix" } }, {}, {} },
            { 0, "Freq Shifter", "alien", false, "fshift_on",
              { { "fshift_hz", "Shift" }, { "fshift_fb", "Feedback" }, { "fshift_spread", "Spread" }, { "fshift_mix", "Mix" } }, {}, {} },
            { 0, "Vocoder", "robot", false, "voc_on",
              { { "voc_note", "Note" }, { "voc_formant", "Formant" }, { "voc_release", "Release" }, { "voc_mix", "Mix" } }, { "voc_carrier" }, {} },
            { 0, "Robot Voice", "", true, "tts_on",
              { { "tts_pitch", "Pitch" }, { "tts_metal", "Metal" }, { "tts_buzz", "Buzz" }, { "tts_mix", "Mix" } }, {}, {} },
            { 0, "Lo-Fi Codec", "", true, "codec_on",
              { { "codec_alias", "Aliasing" }, { "codec_mix", "Mix" } }, { "codec_rate", "codec_type" }, {} },
            { 0, "Bitcrusher", "", true, "crush_on",
              { { "crush_bits", "Bits" }, { "crush_rate", "Rate" }, { "crush_mix", "Mix" } }, {}, {} },
            { 0, "Telephone", "", true, "dial_on",
              { { "dial_band", "Band" }, { "dial_noise", "Noise" }, { "dial_crackle", "Crackle" }, { "dial_mix", "Mix" } },
              {}, { { "dial_modem", "Modem squeal" } } },
            { 0, "Vinyl Sim", "", true, "vinyl_on",
              { { "vinyl_crackle", "Crackle" }, { "vinyl_wear", "Wear" }, { "vinyl_wow", "Wow" }, { "vinyl_mix", "Mix" } }, {}, {} },
            { 0, "MP3 Compression", "", true, "lobit_on",
              { { "lobit_kbps", "Bitrate" }, { "lobit_swirl", "Artifacts" }, { "lobit_mix", "Mix" } }, {}, {} },
            { 0, "Packet Loss", "", true, "pkt_on",
              { { "pkt_loss", "Loss" }, { "pkt_size", "Packet" }, { "pkt_mix", "Mix" } }, { "pkt_conceal" }, {} },
            { 0, "Stutter", "", true, "buf_on",
              { { "buf_chance", "Chance" }, { "buf_slice", "Slice" }, { "buf_dropout", "Dropout" }, { "buf_mix", "Mix" } }, {}, {} },
            { 0, "Data Corruption", "", true, "corrupt_on",
              { { "corrupt_amount", "Amount" }, { "corrupt_bits", "Bits" }, { "corrupt_reverse", "Reverse" }, { "corrupt_mix", "Mix" } }, {}, {} },
            { 0, "Varispeed", "", true, "lag_on",
              { { "lag_chance", "Chance" }, { "lag_length", "Length" }, { "lag_depth", "Depth" }, { "lag_mix", "Mix" } }, {}, {} },
            { 0, "Wow & Flutter", "", true, "jit_on",
              { { "jit_wow", "Wow" }, { "jit_flutter", "Flutter" }, { "jit_drift", "Drift" }, { "jit_mix", "Mix" } }, {}, {} },
            { 0, "Beat Repeat", "", true, "glitch_on",
              { { "glitch_chance", "Chance" }, { "glitch_mix", "Mix" } }, { "glitch_div", "glitch_style" }, {} },
            { 0, "Trance Gate", "", true, "gate_on",
              { { "gate_depth", "Depth" }, { "gate_smooth", "Smooth" } }, { "gate_pattern", "gate_div" }, {} },
            { 0, "Sidechain Pump", "", true, "pump_on",
              { { "pump_depth", "Depth" }, { "pump_shape", "Release" } }, { "pump_mode", "pump_div" }, {} },
            { 0, "Volume Swell", "fade-in", false, "swell_on",
              { { "swell_attack", "Attack" }, { "swell_sens", "Sens" }, { "swell_mix", "Mix" } }, {}, {} },
            { 0, "Chorus", "thicken", false, "chorus_on",
              { { "chorus_rate", "Rate" }, { "chorus_depth", "Depth" }, { "chorus_fb", "Feedback" }, { "chorus_mix", "Mix" } }, { "chorus_sync" }, {} },
            { 0, "Flanger", "jet", false, "flange_on",
              { { "flange_rate", "Rate" }, { "flange_depth", "Depth" }, { "flange_fb", "Feedback" }, { "flange_mix", "Mix" } }, { "flange_sync" }, {} },
            { 0, "Phaser", "swirl", false, "phaser_on",
              { { "phaser_rate", "Rate" }, { "phaser_depth", "Depth" }, { "phaser_fb", "Feedback" }, { "phaser_mix", "Mix" } }, { "phaser_sync" }, {} },
            { 0, "Tremolo / Pan", "motion", false, "trem_on",
              { { "trem_rate", "Rate" }, { "trem_depth", "Depth" }, { "trem_shape", "Shape" }, { "trem_pan", "Pan" } }, { "trem_sync" }, {} },
            { 0, "Stereo Widener", "wide", false, "spread_on",
              { { "spread_width", "Width" }, { "spread_motion", "Motion" }, { "spread_mix", "Mix" } }, {}, {} },
            { 0, "Reverse Delay", "backwards", false, "rdelay_on",
              { { "rdelay_time", "Time" }, { "rdelay_fb", "Feedback" }, { "rdelay_mix", "Mix" } }, { "rdelay_sync" }, {} },
            { 0, "Delay", "echo", false, "delay_on",
              { { "delay_time", "Time" }, { "delay_fb", "Feedback" }, { "delay_tone", "Tone" }, { "delay_mix", "Mix" } },
              { "delay_sync" }, { { "delay_pp", "Ping-pong" } } },
            { 0, "Blur", "ambient", false, "blur_on",
              { { "blur_size", "Size" }, { "blur_smear", "Smear" }, { "blur_shimmer", "Shimmer" }, { "blur_mix", "Mix" } },
              {}, { { "blur_freeze", "Freeze" } } },
            { 0, "Reverb", "space", false, "verb_on",
              { { "verb_size", "Size" }, { "verb_damp", "Damp" }, { "verb_width", "Width" }, { "verb_mix", "Mix" } }, {}, {} },
            { 0, "Compressor", "glue", false, "comp_on",
              { { "comp_squash", "Amount" }, { "comp_ratio", "Ratio" }, { "comp_speed", "Speed" }, { "comp_mix", "Mix" } }, {}, {} },
            { 0, "Speaker Sim", "", true, "spk_on",
              { { "spk_drive", "Drive" }, { "spk_mix", "Mix" } }, { "spk_model" }, {} },
        };
        for (int i = 0; i < (int) s.size(); ++i)
        {
            s[(size_t) i].number = i + 1;                                       // badge = position in the chain
            jassert (juce::String (s[(size_t) i].powerId) == Rack::onId (i));   // must match Rack.h order
        }
        return s;
    }

    // which section of the + ADD FX menu each effect lives in
    juce::String categoryOf (const juce::String& onId)
    {
        static const std::map<juce::String, juce::String> cat {
            { "trans_on", "Tone & Dynamics" }, { "filter_on", "Tone & Dynamics" }, { "wah_on", "Tone & Dynamics" }, { "eq_on", "Tone & Dynamics" },
            { "gloss_on", "Tone & Dynamics" }, { "comp_on", "Tone & Dynamics" }, { "ngate_on", "Tone & Dynamics" },
            { "fuzz_on", "Tone & Dynamics" }, { "tune_on", "Pitch & Voice" }, { "voc_on", "Pitch & Voice" },
            { "pitch_on", "Pitch & Voice" }, { "ring_on", "Pitch & Voice" }, { "tts_on", "Pitch & Voice" },
            { "talk_on", "Pitch & Voice" }, { "fshift_on", "Pitch & Voice" },
            { "glitch_on", "Rhythm (BPM synced)" }, { "gate_on", "Rhythm (BPM synced)" }, { "pump_on", "Rhythm (BPM synced)" },
            { "swell_on", "Ambient & Space" }, { "blur_on", "Ambient & Space" }, { "delay_on", "Ambient & Space" }, { "verb_on", "Ambient & Space" },
            { "rdelay_on", "Ambient & Space" }, { "flange_on", "Modulation & Stereo" },
            { "chorus_on", "Modulation & Stereo" }, { "phaser_on", "Modulation & Stereo" }, { "trem_on", "Modulation & Stereo" },
            { "spread_on", "Modulation & Stereo" } };
        auto it = cat.find (onId);
        return it != cat.end() ? it->second : juce::String ("Lo-Fi & Internet Damage");
    }

    bool isBpmModule (const juce::String& onId)
    {
        return onId == "glitch_on" || onId == "gate_on" || onId == "pump_on" || onId == "delay_on" || onId == "rdelay_on";
    }

    //==========================================================================
    // Backgrounds. Each is painted once into an image when chosen.
    // Frutiger Metro: a flat, Windows 8 style start screen. No gradients, no gloss:
    // solid background, square tiles on a grid in groups, flat white glyphs
    // and an oversized panorama title running off the edge.
    void drawMetroGlyph (juce::Graphics& g, int kind, juce::Rectangle<float> b)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        const auto c = b.getCentre();
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        juce::Path p;
        switch (kind)
        {
            case 0:   // note
                g.fillEllipse (c.x - r * 0.55f, c.y + r * 0.2f, r * 0.62f, r * 0.48f);
                g.fillRect (c.x + r * 0.0f, c.y - r * 0.75f, r * 0.12f, r * 1.2f);
                g.fillRect (c.x + r * 0.0f, c.y - r * 0.75f, r * 0.5f, r * 0.12f);
                break;
            case 1:   // waveform
                p.startNewSubPath (c.x - r, c.y);
                for (int i = 1; i <= 24; ++i)
                {
                    const float t = (float) i / 24.0f;
                    p.lineTo (c.x - r + 2.0f * r * t, c.y - std::sin (t * 18.85f) * r * 0.6f * std::sin (t * 3.1416f));
                }
                g.strokePath (p, juce::PathStrokeType (r * 0.1f));
                break;
            case 2:   // equaliser bars
                for (int i = 0; i < 4; ++i)
                {
                    const float hgt = r * (0.6f + 0.35f * (float) ((i * 7) % 4));
                    g.fillRect (c.x - r + (float) i * r * 0.55f, c.y + r * 0.7f - hgt, r * 0.38f, hgt);
                }
                break;
            case 3:   // play
                p.addTriangle (c.x - r * 0.45f, c.y - r * 0.6f, c.x - r * 0.45f, c.y + r * 0.6f, c.x + r * 0.6f, c.y);
                g.fillPath (p);
                break;
            case 4:   // ring
                g.drawEllipse (b.reduced (r * 0.2f), r * 0.12f);
                break;
            case 5:   // smiley (the mascot, Metro-flat)
                g.drawEllipse (b.reduced (r * 0.15f), r * 0.1f);
                g.fillEllipse (c.x - r * 0.35f, c.y - r * 0.3f, r * 0.16f, r * 0.26f);
                g.fillEllipse (c.x + r * 0.19f, c.y - r * 0.3f, r * 0.16f, r * 0.26f);
                p.addCentredArc (c.x, c.y + r * 0.05f, r * 0.42f, r * 0.36f, 0.0f, 2.2f, 4.08f, true);
                g.strokePath (p, juce::PathStrokeType (r * 0.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            default:  // globe
                g.drawEllipse (b.reduced (r * 0.2f), r * 0.09f);
                g.drawEllipse (c.x - r * 0.35f, c.y - r * 0.8f, r * 0.7f, r * 1.6f, r * 0.07f);
                g.fillRect (c.x - r * 0.8f, c.y - r * 0.04f, r * 1.6f, r * 0.08f);
                break;
        }
    }

    void paintFrutigerMetro (juce::Graphics& g, int W, int H)
    {
        const float w = (float) W, h = (float) H;
        g.fillAll (juce::Colour (0xff1f0610));   // flat start-screen background

        // panorama title, huge and thin, cropped by the window edge
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.setFont (juce::Font (juce::FontOptions (330.0f)));
        g.drawText ("evil fx", juce::Rectangle<float> (-30.0f, h - 330.0f, w * 1.4f, 360.0f), juce::Justification::bottomLeft, false);

        // Metro accent palette, warm end (crimson / red / orange / amber / magenta / pink)
        const juce::Colour palette[] { juce::Colour (0xffa20025), juce::Colour (0xffe51400), juce::Colour (0xfffa6800),
                                       juce::Colour (0xfff0a30a), juce::Colour (0xffd80073), juce::Colour (0xffc1004f),
                                       juce::Colour (0xff6d0019) };

        constexpr float unit = 118.0f, gutter = 8.0f, groupGap = 44.0f;
        constexpr int groupCols = 4, rows = 6;
        juce::Random rnd (8);
        float gx = 34.0f;
        const float top = 112.0f;
        while (gx < w)
        {
            bool used[rows][groupCols] {};
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < groupCols; ++c)
                {
                    if (used[r][c]) continue;
                    // pick a tile size that fits: 2x2 (big), 2x1 (wide) or 1x1
                    int tw = 1, th = 1;
                    const float roll = rnd.nextFloat();
                    if (roll < 0.18f && c + 1 < groupCols && r + 1 < rows && ! used[r][c + 1] && ! used[r + 1][c] && ! used[r + 1][c + 1]) { tw = 2; th = 2; }
                    else if (roll < 0.5f && c + 1 < groupCols && ! used[r][c + 1]) tw = 2;
                    for (int dr = 0; dr < th; ++dr) for (int dc = 0; dc < tw; ++dc) used[r + dr][c + dc] = true;
                    if (rnd.nextFloat() < 0.1f) continue;   // the odd empty slot

                    const auto tile = juce::Rectangle<float> (gx + (float) c * (unit + gutter), top + (float) r * (unit + gutter),
                                                              (float) tw * unit + (float) (tw - 1) * gutter,
                                                              (float) th * unit + (float) (th - 1) * gutter);
                    // full-strength accents would shout over the rack, so tiles sit a step darker
                    g.setColour (palette[rnd.nextInt (7)].interpolatedWith (juce::Colour (0xff1f0610), 0.42f));
                    g.fillRect (tile);

                    const float glyph = tile.getHeight() * (th == 2 ? 0.34f : 0.36f);
                    drawMetroGlyph (g, rnd.nextInt (7), juce::Rectangle<float> (glyph, glyph)
                                                            .withCentre ({ tile.getCentreX(), tile.getCentreY() - tile.getHeight() * 0.06f }));
                }
            gx += (float) groupCols * (unit + gutter) - gutter + groupGap;
        }
    }

    void paintAeroSky (juce::Graphics& g, int W, int H)
    {
        const float w = (float) W, h = (float) H;
        juce::ColourGradient sky (juce::Colour (0xff2fa8ee), 0, 0, juce::Colour (0xffeefbff), 0, h * 0.8f, false);
        sky.addColour (0.45, juce::Colour (0xffa9e2ff));
        g.setGradientFill (sky);
        g.fillAll();
        const juce::Point<float> sun (w * 0.84f, 10.0f);
        for (int i = 0; i < 7; ++i)
        {
            juce::Path ray;
            const float a = 1.75f + (float) i * 0.24f;
            ray.startNewSubPath (sun);
            ray.lineTo (sun.getPointOnCircumference (1300.0f, a - 0.05f));
            ray.lineTo (sun.getPointOnCircumference (1300.0f, a + 0.05f));
            ray.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.fillPath (ray);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.95f), sun.x, sun.y, juce::Colours::white.withAlpha (0.0f), sun.x + 360.0f, sun.y, true));
        g.fillEllipse (sun.x - 360.0f, sun.y - 360.0f, 720.0f, 720.0f);
        juce::Path hill1, hill2;
        hill1.startNewSubPath (0, h * 0.72f);
        hill1.cubicTo (w * 0.25f, h * 0.6f, w * 0.45f, h * 0.82f, w * 0.7f, h * 0.7f);
        hill1.cubicTo (w * 0.85f, h * 0.63f, w * 0.95f, h * 0.66f, w, h * 0.68f);
        hill1.lineTo (w, h); hill1.lineTo (0, h); hill1.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb7f07a), 0, h * 0.62f, juce::Colour (0xff4caf28), 0, h, false));
        g.fillPath (hill1);
        hill2.startNewSubPath (0, h * 0.86f);
        hill2.cubicTo (w * 0.3f, h * 0.78f, w * 0.6f, h * 0.92f, w, h * 0.8f);
        hill2.lineTo (w, h); hill2.lineTo (0, h); hill2.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8fdc4f), 0, h * 0.78f, juce::Colour (0xff2f8f1a), 0, h, false));
        g.fillPath (hill2);
        juce::Random rnd (2003);
        for (int i = 0; i < 18; ++i)
        {
            const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h, r = 4.0f + rnd.nextFloat() * 16.0f;
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), x, y, juce::Colours::white.withAlpha (0.38f), x + r, y, true));
            g.fillEllipse (x - r, y - r, r * 2, r * 2);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawEllipse (x - r, y - r, r * 2, r * 2, 0.8f);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.5f, r * 0.32f);
        }
    }

    void paintCrimsonGrid (juce::Graphics& g, int W, int H)
    {
        const float w = (float) W, h = (float) H, horizon = h * 0.7f;
    juce::ColourGradient sky (juce::Colour (0xff0d0007), 0, 0, juce::Colour (0xffff6a4a), 0, horizon, false);
    sky.addColour (0.38, juce::Colour (0xff3d0414));
    sky.addColour (0.75, juce::Colour (0xffa3122a));
    g.setGradientFill (sky);
    g.fillRect (0.0f, 0.0f, w, horizon + 1.0f);

    juce::Random rnd (777);
    for (int i = 0; i < 140; ++i)   // stars, a few with a little four-point sparkle
    {
        const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * horizon * 0.75f, r = 0.5f + rnd.nextFloat() * 1.3f;
        g.setColour (juce::Colours::white.withAlpha (0.25f + rnd.nextFloat() * 0.65f));
        g.fillEllipse (x - r, y - r, r * 2, r * 2);
        if (i % 17 == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.drawLine (x - 6.0f, y, x + 6.0f, y, 0.8f);
            g.drawLine (x, y - 6.0f, x, y + 6.0f, 0.8f);
        }
    }

    const juce::Point<float> rayOrigin (w * 0.5f, -60.0f);   // heavenly light from above
    for (int i = 0; i < 11; ++i)
    {
        const float a = juce::MathConstants<float>::pi * (0.72f + 0.56f * (float) i / 10.0f);
        juce::Path ray;
        ray.startNewSubPath (rayOrigin);
        ray.lineTo (rayOrigin.getPointOnCircumference (1100.0f, a - 0.035f));
        ray.lineTo (rayOrigin.getPointOnCircumference (1100.0f, a + 0.035f));
        ray.closeSubPath();
        g.setColour (juce::Colour (0xffffe6d0).withAlpha (0.045f));
        g.fillPath (ray);
    }

    // Vista-style aurora swooshes
    struct Swoosh { float y0, y1, amp, thick, phase; juce::Colour c; };
    const Swoosh swooshes[] {
        { 0.18f, 0.30f, 60.0f, 46.0f, 0.0f, juce::Colour (0xffff4d6d) },
        { 0.26f, 0.20f, 45.0f, 30.0f, 1.7f, juce::Colour (0xffffd0d8) },
        { 0.40f, 0.33f, 70.0f, 60.0f, 3.1f, juce::Colour (0xffff8a3d) },
        { 0.50f, 0.46f, 35.0f, 22.0f, 4.4f, juce::Colour (0xffffffff) },
    };
    for (auto& s : swooshes)
    {
        juce::Path top, band;
        const int steps = 48;
        for (int k = 0; k <= steps; ++k)
        {
            const float t = (float) k / steps, x = t * w;
            const float y = h * (s.y0 + (s.y1 - s.y0) * t) + s.amp * std::sin (t * 5.0f + s.phase);
            k == 0 ? top.startNewSubPath (x, y) : top.lineTo (x, y);
            k == 0 ? band.startNewSubPath (x, y) : band.lineTo (x, y);
        }
        for (int k = steps; k >= 0; --k)
        {
            const float t = (float) k / steps, x = t * w;
            const float y = h * (s.y0 + (s.y1 - s.y0) * t) + s.amp * std::sin (t * 5.0f + s.phase) + s.thick * (0.6f + 0.4f * std::sin (t * 3.0f + s.phase));
            band.lineTo (x, y);
        }
        band.closeSubPath();
        juce::ColourGradient fill (s.c.withAlpha (0.0f), 0, 0, s.c.withAlpha (0.0f), w, 0, false);
        fill.addColour (0.3, s.c.withAlpha (0.22f));
        fill.addColour (0.7, s.c.withAlpha (0.16f));
        g.setGradientFill (fill);
        g.fillPath (band);
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.0f), 0, 0, juce::Colours::white.withAlpha (0.0f), w, 0, false);
        edge.addColour (0.45, juce::Colours::white.withAlpha (0.55f));
        g.setGradientFill (edge);
        g.strokePath (top, juce::PathStrokeType (1.2f));
    }

    // retro sun sitting on the horizon, with the classic cut-out stripes
    const juce::Point<float> sun (w * 0.64f, horizon);
    const float sr = 150.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb070).withAlpha (0.55f), sun.x, sun.y - sr * 0.4f,
                                             juce::Colour (0xffff4040).withAlpha (0.0f), sun.x + sr * 2.2f, sun.y, true));
    g.fillEllipse (sun.x - sr * 2.2f, sun.y - sr * 2.2f, sr * 4.4f, sr * 4.4f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<int> (0, 0, W, (int) horizon));
        for (int k = 0; k < 6; ++k)
        {
            const float y = sun.y - sr * 0.08f - (float) k * sr * 0.12f;
            g.excludeClipRegion (juce::Rectangle<float> (sun.x - sr, y - (6.0f - k) * 1.3f, sr * 2.0f, (6.0f - k) * 2.2f).toNearestInt());
        }
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff3b0), sun.x, sun.y - sr, juce::Colour (0xffe01b2a), sun.x, sun.y, false));
        g.fillEllipse (sun.x - sr, sun.y - sr, sr * 2.0f, sr * 2.0f);
    }

    // glowing grid floor
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a0410), 0, horizon, juce::Colour (0xff060002), 0, h, false));
    g.fillRect (0.0f, horizon, w, h - horizon);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff9a60).withAlpha (0.35f), sun.x, horizon,
                                             juce::Colour (0xffff9a60).withAlpha (0.0f), sun.x, h, false));
    g.fillRect (sun.x - sr * 0.8f, horizon, sr * 1.6f, h - horizon);   // sun reflection
    const juce::Point<float> vp (w * 0.5f, horizon);
    for (int i = -20; i <= 20; ++i)
    {
        const juce::Line<float> ln (vp, { vp.x + (float) i * 110.0f, h + 20.0f });
        g.setColour (juce::Colour (0xffff3a5a).withAlpha (0.12f)); g.drawLine (ln, 4.0f);
        g.setColour (juce::Colour (0xffff5a7a).withAlpha (0.55f)); g.drawLine (ln, 1.0f);
    }
    for (int k = 1; k <= 12; ++k)
    {
        const float t = (float) k / 12.0f, y = horizon + (h - horizon) * t * t;
        g.setColour (juce::Colour (0xffff3a5a).withAlpha (0.1f * t + 0.05f)); g.drawLine (0.0f, y, w, y, 4.0f);
        g.setColour (juce::Colour (0xffff5a7a).withAlpha (0.25f + 0.45f * t)); g.drawLine (0.0f, y, w, y, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffd0a0).withAlpha (0.9f), 0, horizon,
                                             juce::Colour (0xffffd0a0).withAlpha (0.0f), 0, horizon + 18.0f, false));
    g.fillRect (0.0f, horizon, w, 18.0f);
    g.setColour (juce::Colour (0xffffe0c0));
    g.drawHorizontalLine ((int) horizon, 0.0f, w);

    for (int i = 0; i < 12; ++i)   // a few glass bubbles
    {
        const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * horizon, r = 4.0f + rnd.nextFloat() * 12.0f;
        juce::ColourGradient fill (juce::Colours::white.withAlpha (0.03f), x, y, juce::Colours::white.withAlpha (0.25f), x + r, y, true);
        g.setGradientFill (fill);
        g.fillEllipse (x - r, y - r, r * 2, r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawEllipse (x - r, y - r, r * 2, r * 2, 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.5f, r * 0.32f);
    }
    }
}

const juce::StringArray& EvilFXEditor::themeNames()
{
    static const juce::StringArray n { "Aero Sky", "Frutiger Metro", "Crimson Grid" };
    return n;
}

//==============================================================================
EvilFXEditor::EvilFXEditor (EvilFXProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (desktop);

    // ---- rack (cards are created once and shown/hidden as you add/remove them)
    specs = moduleSpecs();
    jassert ((int) specs.size() == Rack::numModules);
    for (int i = 0; i < (int) specs.size(); ++i)
    {
        auto* card = cards.add (new ModuleCard (proc.apvts, specs[(size_t) i]));
        card->onRemove = [this, i] { removeModule (i); };
        card->onDragStart = [this, i] { beginDrag (i); };
        card->onDragMove = [this, i] (juce::Point<int> p) { dragTo (i, p); };
        card->onDragEnd = [this, i] { endDrag (i); };
        card->onContextMenu = [this, i] { showCardMenu (i); };
        rackArea.addChildComponent (card);
    }
    viewport.setViewedComponent (&rackArea, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (14);
    desktop.addAndMakeVisible (viewport);

    moreHint.onClick = [this]
    {
        viewport.setViewPosition (0, juce::jmin (rackArea.getHeight() - viewport.getHeight(),
                                                 viewport.getViewPositionY() + viewport.getHeight() - footerH));
    };
    desktop.addChildComponent (moreHint);

    // ---- xainjaya.com link (top right)
    siteBtn.setTooltip ("Open xainjaya.com in your browser");
    siteBtn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    siteBtn.onClick = [] { juce::URL ("https://xainjaya.com").launchInDefaultBrowser(); };
    desktop.addAndMakeVisible (siteBtn);

    // ---- toolbar
    addBtn.setTooltip ("Add an effect to your rack (or jump to one that's already in it)");
    addBtn.onClick = [this] { showAddMenu(); };
    addBtn.getProperties().set ("accent", true);
    clearBtn.setTooltip ("Remove every effect from the rack");
    clearBtn.onClick = [this] { clearRack(); };
    helpBtn.setTooltip ("How to use Evil FX");
    helpBtn.onClick = [this] { showHelp(); };
    themeBox.addItemList (themeNames(), 1);
    themeBox.setSelectedItemIndex (juce::jlimit (0, themeNames().size() - 1, (int) proc.apvts.state.getProperty ("theme", 0)), juce::dontSendNotification);
    themeBox.setTooltip ("Background theme");
    themeBox.onChange = [this]
    {
        proc.apvts.state.setProperty ("theme", themeBox.getSelectedItemIndex(), nullptr);
        renderBackground();
        desktop.repaint();
    };
    undoBtn.setTooltip ("Undo (Ctrl/Cmd+Z)");
    redoBtn.setTooltip ("Redo (Ctrl/Cmd+Shift+Z or Ctrl+Y)");
    undoBtn.onClick = [this] { doUndo (false); };
    redoBtn.onClick = [this] { doUndo (true); };
    abA.setTooltip ("Sound A - switch between two sounds to compare them");
    abB.setTooltip ("Sound B - switch between two sounds to compare them");
    abCopy.setTooltip ("Copy what you're hearing into the other slot");
    abA.setClickingTogglesState (false); abB.setClickingTogglesState (false);
    abA.onClick = [this] { if (proc.getActiveAB() != 0) switchAB(); };
    abB.onClick = [this] { if (proc.getActiveAB() != 1) switchAB(); };
    abCopy.onClick = [this]
    {
        proc.copyActiveToOther();
        showNote (juce::String ("Copied ") + (proc.getActiveAB() == 0 ? "A to B" : "B to A") + " - tweak away, then compare");
    };
    for (auto* c : std::initializer_list<juce::Component*> { &addBtn, &clearBtn, &helpBtn, &themeBox, &undoBtn, &redoBtn, &abA, &abB, &abCopy })
        desktop.addAndMakeVisible (c);

    // ---- master + visualizer
    master = std::make_unique<ModuleCard> (proc.apvts, CardSpec { 0, "Master", "output", false, nullptr,
        { { "in_gain", "Input" }, { "out_gain", "Output" }, { "width", "Width" }, { "mix", "Mix" } },
        {}, { { "limiter", "Limiter" } } });
    desktop.addAndMakeVisible (*master);
    desktop.addAndMakeVisible (meter);
    meter.setInterceptsMouseClicks (false, false);

    viz.setState ((bool) proc.apvts.state.getProperty ("vizOn", false), (int) proc.apvts.state.getProperty ("vizMode", 0));
    viz.onChange = [this] (bool on, int mode)
    {
        proc.apvts.state.setProperty ("vizOn", on, nullptr);
        proc.apvts.state.setProperty ("vizMode", mode, nullptr);
        proc.setVisualizerActive (on);
        startTimerHz (on ? 30 : 15);
    };
    proc.setVisualizerActive (viz.isOn());
    desktop.addAndMakeVisible (viz);

    // ---- presets, save, randomize
    rebuildPresetMenu();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        newUndoStep();
        if (id >= 3000) { if (juce::isPositiveAndBelow (id - 3000, userFiles.size())) Presets::loadUser (proc.apvts, userFiles[id - 3000]); }
        else if (id >= 2000) Presets::apply (proc.apvts, id - 2000);   // favourites section
        else if (id >= 1000) { if (juce::isPositiveAndBelow (id - 1000, userFiles.size())) Presets::loadUser (proc.apvts, userFiles[id - 1000]); }
        else Presets::apply (proc.apvts, id - 1);
        afterSoundChange();
    };
    presetBox.setTooltip ("Factory and user presets");
    auto step = [this] (int d)
    {
        juce::Array<int> ids;
        for (int i = 0; i < presetBox.getNumItems(); ++i)
            if (const int id = presetBox.getItemId (i); id < 2000) ids.add (id);
        if (ids.isEmpty()) return;
        const int cur = juce::jmax (0, ids.indexOf (presetBox.getSelectedId()));
        presetBox.setSelectedId (ids[(cur + d + ids.size()) % ids.size()]);
    };
    prevBtn.onClick = [step] { step (-1); };
    nextBtn.onClick = [step] { step (1); };
    saveBtn.setTooltip ("Save the current sound as your own preset");
    saveBtn.onClick = [this] { savePresetDialog(); };
    randomBtn.setTooltip ("Switch on a few random effects with random settings");
    randomBtn.onClick = [this] { newUndoStep(); Presets::randomize (proc.apvts); refreshRack(); syncPresetBox(); showNote ("Randomized! Hit SAVE if you like it.  (Ctrl+Z to go back)"); };
    browseBtn.setTooltip ("Search presets and star your favourites");
    browseBtn.onClick = [this] { browser.isVisible() ? closeBrowser() : openBrowser(); };
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevBtn, &nextBtn, &saveBtn, &randomBtn, &browseBtn }) desktop.addAndMakeVisible (c);

    browser.currentName = [this] { return proc.apvts.state.getProperty ("presetName").toString(); };
    browser.onLoadFactory = [this] (int index) { newUndoStep(); Presets::apply (proc.apvts, index); afterSoundChange(); };
    browser.onLoadUser = [this] (const juce::File& f) { Presets::loadUser (proc.apvts, f); afterSoundChange(); };
    browser.onClose = [this] { closeBrowser(); };
    desktop.addChildComponent (browser);

    desktop.addMouseListener (this, true);   // a new undo step on every click
    setWantsKeyboardFocus (true);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setResizeLimits (baseW / 2, baseH / 2, baseW * 3 / 2, baseH * 3 / 2);
    // reopen at the size you last left it (saved with the project)
    const int savedW = (int) proc.apvts.state.getProperty ("editorW", baseW * 8 / 10);
    const int startW = juce::jlimit (baseW / 2, baseW * 3 / 2, savedW);
    setSize (startW, juce::roundToInt (startW * (double) baseH / baseW));

    timerCallback();   // fill rack, meters and counters right away
    startTimerHz (viz.isOn() ? 30 : 15);
}

EvilFXEditor::~EvilFXEditor()
{
    stopTimer();
    proc.setVisualizerActive (false);   // no tap while the window is closed
    setLookAndFeel (nullptr);
}

void EvilFXEditor::resized()
{
    if (getWidth() > 0) proc.apvts.state.setProperty ("editorW", getWidth(), nullptr);
    desktop.setBounds (0, 0, baseW, baseH);
    desktop.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW));

    toolbarArea = { margin, headerH + 2, baseW - 2 * margin, toolbarH };
    auto tb = toolbarArea.reduced (8, 4);
    addBtn.setBounds (tb.removeFromLeft (120)); tb.removeFromLeft (6);
    clearBtn.setBounds (tb.removeFromLeft (76));
    helpBtn.setBounds (tb.removeFromRight (tb.getHeight() + 2)); tb.removeFromRight (8);
    themeBox.setBounds (tb.removeFromRight (150)); tb.removeFromRight (58);   // room for the THEME label
    abCopy.setBounds (tb.removeFromRight (58)); tb.removeFromRight (4);
    abB.setBounds (tb.removeFromRight (30)); tb.removeFromRight (2);
    abA.setBounds (tb.removeFromRight (30)); tb.removeFromRight (14);
    redoBtn.setBounds (tb.removeFromRight (56)); tb.removeFromRight (4);
    undoBtn.setBounds (tb.removeFromRight (56));

    const int top = toolbarArea.getBottom() + 6, bottom = baseH - statusH - 10;
    viewport.setBounds (margin, top, baseW - 2 * margin, bottom - top - stripH - gap);
    master->setBounds (margin, bottom - stripH, 470, stripH);
    meter.setBounds (master->getOptionSpare().translated (master->getX(), master->getY()).reduced (4, 3));
    viz.setBounds (master->getRight() + gap, bottom - stripH, baseW - 2 * margin - 470 - gap, stripH);
    moreHint.setBounds (viewport.getX() + (viewport.getWidth() - 14) / 2 - 90, viewport.getBottom() - footerH + 2, 180, footerH - 4);

    presetBox.setBounds (baseW / 2 - 262, 40, 268, 30);
    prevBtn.setBounds (presetBox.getX() - 38, 39, 32, 32);
    nextBtn.setBounds (presetBox.getRight() + 6, 39, 32, 32);
    saveBtn.setBounds (nextBtn.getRight() + 10, 41, 66, 28);
    randomBtn.setBounds (saveBtn.getRight() + 6, 41, 78, 28);
    browseBtn.setBounds (randomBtn.getRight() + 6, 41, 76, 28);
    browser.setBounds (0, 0, baseW, baseH);

    auto right = juce::Rectangle<int> (margin, 6, baseW - 2 * margin, headerH - 10).removeFromRight (318).reduced (10, 14);
    siteBtn.setBounds (right.withSizeKeepingCentre (right.getWidth(), 50));
    statusArea  = juce::Rectangle<int> (margin, baseH - statusH - 6, baseW - 2 * margin, statusH);
    auto inner = statusArea.reduced (3);
    grField  = inner.removeFromRight (120); inner.removeFromRight (3);
    latField = inner.removeFromRight (150); inner.removeFromRight (3);
    bpmField = inner.removeFromRight (140); inner.removeFromRight (3);
    marqueeArea = inner;

    renderBackground();
    refreshRack();
}

//==============================================================================
void EvilFXEditor::showAddMenu()
{
    const auto rack = Rack::get (proc.apvts.state);
    juce::PopupMenu menu;
    juce::StringArray order { "Tone & Dynamics", "Pitch & Voice", "Lo-Fi & Internet Damage", "Rhythm (BPM synced)", "Modulation & Stereo", "Ambient & Space" };
    for (auto& cat : order)
    {
        menu.addSectionHeader (cat);
        for (int i = 0; i < (int) specs.size(); ++i)
        {
            const juce::String id (specs[(size_t) i].powerId);
            if (categoryOf (id) != cat) continue;
            juce::String text = juce::String (i + 1).paddedLeft ('0', 2) + "   " + specs[(size_t) i].title;
            if (isBpmModule (id)) text << "   (BPM)";
            menu.addItem (i + 1, text, true, rack[i] == '1');
        }
    }
    menu.addSeparator();
    menu.addItem (999, "Ticked = already in your rack (click to jump to it)", false, false);

    juce::Component::SafePointer<EvilFXEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addBtn).withMinimumWidth (300),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0 && result <= Rack::numModules) safe->focusModule (result - 1);
                        });
}

void EvilFXEditor::focusModule (int module)
{
    if (! Rack::contains (proc.apvts.state, module))
    {
        newUndoStep();
        Rack::set (proc.apvts.state, module, true, &proc.undoManager);
        if (auto* on = proc.apvts.getParameter (Rack::onId (module))) on->setValueNotifyingHost (1.0f);
        refreshRack();
    }
    auto* c = cards[module];
    const int viewTop = viewport.getViewPositionY(), viewH = viewport.getHeight() - footerH;
    if (c->getY() < viewTop || c->getBottom() > viewTop + viewH)
        viewport.setViewPosition (0, juce::jlimit (0, juce::jmax (0, rackArea.getHeight() - viewport.getHeight()), c->getY()));
    c->flash();
    updateMoreHint();
}

void EvilFXEditor::removeModule (int module)
{
    newUndoStep();
    Rack::set (proc.apvts.state, module, false, &proc.undoManager);
    if (auto* on = proc.apvts.getParameter (Rack::onId (module))) on->setValueNotifyingHost (0.0f);
    refreshRack();
}

void EvilFXEditor::clearRack()
{
    newUndoStep();
    for (int i = 0; i < Rack::numModules; ++i)
        if (auto* on = proc.apvts.getParameter (Rack::onId (i))) on->setValueNotifyingHost (0.0f);
    Rack::setFlags (proc.apvts.state, juce::String::repeatedString ("0", Rack::numModules), &proc.undoManager);
    refreshRack();
    showNote ("Rack cleared. Hit + ADD FX to start again (or Ctrl+Z).");
}

//==============================================================================
// Rack layout + reordering. The rack shows its modules in chain order; dragging a
// card's title bar (or the right-click menu) changes that order.
juce::Array<int> EvilFXEditor::rackSequence() const
{
    const auto rack = Rack::get (proc.apvts.state);
    juce::Array<int> seq;
    for (int m : Rack::getOrder (proc.apvts.state)) if (rack[m] == '1') seq.add (m);
    return seq;
}

juce::Rectangle<int> EvilFXEditor::slotBounds (int slot) const
{
    const int areaW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int cardW = (areaW - (cols - 1) * gap) / cols;
    const int cardH = (viewport.getHeight() - gap - footerH) / 2;   // footer leaves room for the "more below" button
    return { (slot % cols) * (cardW + gap), (slot / cols) * (cardH + gap), cardW, cardH };
}

void EvilFXEditor::layoutCards (const juce::Array<int>& seq, int skipModule)
{
    for (int k = 0; k < seq.size(); ++k)
    {
        auto* c = cards[seq[k]];
        c->setNumber (k + 1);
        if (seq[k] != skipModule) c->setBounds (slotBounds (k));
    }
}

void EvilFXEditor::refreshRack()
{
    if (dragModule >= 0) return;   // don't fight an active drag
    const auto rack = Rack::get (proc.apvts.state);
    lastRack = rack;
    lastOrder = proc.apvts.state.getProperty ("order").toString();

    const auto seq = rackSequence();
    for (int i = 0; i < cards.size(); ++i) cards[i]->setVisible (rack[i] == '1');
    layoutCards (seq);

    inRack = seq.size();
    const int areaW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int viewH = viewport.getHeight();
    const int cardH = slotBounds (0).getHeight();
    const int rows = (inRack + cols - 1) / cols;
    rackArea.empty = inRack == 0;
    rackArea.setSize (areaW, juce::jmax (viewH, rows * (cardH + gap) - gap + footerH));
    rackArea.repaint();
    desktop.repaint (toolbarArea);
    updateMoreHint();
}

void EvilFXEditor::commitRackSequence (const juce::Array<int>& seq)
{
    // the rack's modules take over the positions the rack's modules already had in
    // the full chain, in their new sequence; effects not in the rack stay put
    auto order = Rack::getOrder (proc.apvts.state);
    const auto rack = Rack::get (proc.apvts.state);
    int k = 0;
    for (int pos = 0; pos < order.size() && k < seq.size(); ++pos)
        if (rack[order[pos]] == '1') order.set (pos, seq[k++]);
    newUndoStep();
    Rack::setOrder (proc.apvts.state, order, &proc.undoManager);
    refreshRack();
}

void EvilFXEditor::moveModule (int module, int newSlot)
{
    auto seq = rackSequence();
    const int from = seq.indexOf (module);
    if (from < 0) return;
    newSlot = juce::jlimit (0, seq.size() - 1, newSlot);
    if (newSlot == from) return;
    seq.move (from, newSlot);
    commitRackSequence (seq);
    cards[module]->flash();
}

void EvilFXEditor::showCardMenu (int module)
{
    const auto seq = rackSequence();
    const int pos = seq.indexOf (module);
    juce::PopupMenu m;
    m.addSectionHeader (specs[(size_t) module].title + "  (#" + juce::String (pos + 1) + " in the chain)");
    m.addItem (1, "Move to the start", pos > 0);
    m.addItem (2, "Move earlier", pos > 0);
    m.addItem (3, "Move later", pos < seq.size() - 1);
    m.addItem (4, "Move to the end", pos < seq.size() - 1);
    m.addSeparator();
    m.addItem (5, "Remove from rack");
    m.addItem (6, "Reset the whole chain to the default order", ! Rack::isDefaultOrder (proc.apvts.state));
    juce::Component::SafePointer<EvilFXEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (cards[module]), [safe, module, pos] (int r)
    {
        if (safe == nullptr) return;
        switch (r)
        {
            case 1: safe->moveModule (module, 0); break;
            case 2: safe->moveModule (module, pos - 1); break;
            case 3: safe->moveModule (module, pos + 1); break;
            case 4: safe->moveModule (module, 1 << 20); break;
            case 5: safe->removeModule (module); break;
            case 6:
                safe->newUndoStep();
                Rack::setOrder (safe->proc.apvts.state, Rack::defaultOrder(), &safe->proc.undoManager);
                safe->refreshRack();
                safe->showNote ("Chain back in the default order");
                break;
            default: break;
        }
    });
}

void EvilFXEditor::beginDrag (int module)
{
    dragModule = module;
    dragSeq = rackSequence();
    cards[module]->toFront (false);
}

void EvilFXEditor::dragTo (int module, juce::Point<int> topLeft)
{
    if (dragModule != module) return;
    auto* c = cards[module];
    const auto first = slotBounds (0);
    c->setTopLeftPosition (juce::jlimit (-first.getWidth() / 2, rackArea.getWidth() - first.getWidth() / 2, topLeft.x),
                           juce::jlimit (-first.getHeight() / 2, rackArea.getHeight() - first.getHeight() / 2, topLeft.y));

    // which slot is the card's centre over?
    const auto centre = c->getBounds().getCentre();
    const int col = juce::jlimit (0, cols - 1, centre.x / (first.getWidth() + gap));
    const int row = juce::jmax (0, centre.y / (first.getHeight() + gap));
    const int target = juce::jlimit (0, dragSeq.size() - 1, row * cols + col);
    const int from = dragSeq.indexOf (module);
    if (from >= 0 && target != from)
    {
        dragSeq.move (from, target);
        layoutCards (dragSeq, module);   // everyone else slides into place live
    }

    // scroll when dragging near the top/bottom of the rack
    const auto inView = viewport.getLocalPoint (&rackArea, centre);
    viewport.autoScroll (inView.x, inView.y, 40, 14);
}

void EvilFXEditor::endDrag (int module)
{
    if (dragModule != module) return;
    dragModule = -1;
    const auto seq = dragSeq;
    if (seq != rackSequence()) commitRackSequence (seq);
    else refreshRack();
    cards[module]->flash();
}

//==============================================================================
// Undo / redo / A-B
void EvilFXEditor::newUndoStep() { proc.undoManager.beginNewTransaction(); }

void EvilFXEditor::mouseDown (const juce::MouseEvent&) { newUndoStep(); }

void EvilFXEditor::doUndo (bool redo)
{
    proc.apvts.copyState();   // flush pending knob moves into the history first
    const bool ok = redo ? proc.undoManager.redo() : proc.undoManager.undo();
    if (! ok) { showNote (redo ? "Nothing to redo" : "Nothing to undo"); return; }
    Rack::addSwitchedOn (proc.apvts);
    refreshRack();
    syncPresetBox();
    updateUndoButtons();
}

void EvilFXEditor::updateUndoButtons()
{
    undoBtn.setEnabled (proc.undoManager.canUndo());
    redoBtn.setEnabled (proc.undoManager.canRedo());
    const int ab = proc.getActiveAB();
    for (auto [btn, on] : { std::pair<juce::TextButton*, bool> { &abA, ab == 0 }, { &abB, ab == 1 } })
        if ((bool) btn->getProperties().getWithDefault ("accent", false) != on)
        {
            btn->getProperties().set ("accent", on);   // the slot you're hearing glows red
            btn->repaint();
        }
    abCopy.setButtonText (ab == 0 ? "A > B" : "B > A");
}

void EvilFXEditor::switchAB()
{
    proc.switchAB();
    afterSoundChange();
    showNote (juce::String ("Listening to ") + (proc.getActiveAB() == 0 ? "A" : "B"));
}

void EvilFXEditor::afterSoundChange()
{
    refreshRack();
    syncPresetBox();
    updateUndoButtons();
    // the theme can't change through A/B, but a loaded user preset might carry one
    const int theme = juce::jlimit (0, themeNames().size() - 1, (int) proc.apvts.state.getProperty ("theme", 0));
    if (theme != themeBox.getSelectedItemIndex()) { themeBox.setSelectedItemIndex (theme, juce::dontSendNotification); renderBackground(); desktop.repaint(); }
}

bool EvilFXEditor::keyPressed (const juce::KeyPress& k)
{
    const auto mods = k.getModifiers();
    const bool cmd = mods.isCommandDown();
    if (cmd && k.getKeyCode() == 'Z') { doUndo (mods.isShiftDown()); return true; }
    if (cmd && k.getKeyCode() == 'Y') { doUndo (true); return true; }
    if (cmd && k.getKeyCode() == 'F') { openBrowser(); return true; }
    if (! browser.isVisible() && ! mods.isAnyModifierKeyDown())
    {
        if (k.getKeyCode() == juce::KeyPress::leftKey)  { prevBtn.triggerClick(); return true; }
        if (k.getKeyCode() == juce::KeyPress::rightKey) { nextBtn.triggerClick(); return true; }
    }
    return false;
}

//==============================================================================
void EvilFXEditor::openBrowser()
{
    browser.refresh();
    browser.setVisible (true);
    browser.toFront (true);
    browser.focusSearch();
}

void EvilFXEditor::closeBrowser()
{
    browser.setVisible (false);
    rebuildPresetMenu();   // favourites may have changed
    syncPresetBox();
}

void EvilFXEditor::updateMoreHint()
{
    const int bottomVisible = viewport.getViewPositionY() + viewport.getHeight() - footerH;
    int below = 0;
    for (auto* c : cards)
        if (c->isVisible() && c->getY() + c->getHeight() / 2 > bottomVisible) ++below;
    if (below != moreHint.count) { moreHint.count = below; moreHint.repaint(); }
    moreHint.setVisible (below > 0);
}

//==============================================================================
void EvilFXEditor::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& list = Presets::all();
    userFiles = Presets::userPresets();
    const auto favs = Presets::loadFavourites();

    // gather the sections first so they can be split into balanced columns
    struct Section { juce::String heading; std::vector<std::pair<juce::String, int>> items; };
    std::vector<Section> sections;
    {
        Section fav { "Favourites", {} };   // starred presets get their own section first
        for (int i = 0; i < (int) list.size(); ++i)
            if (favs.contains (Presets::favKeyFactory (list[(size_t) i].name))) fav.items.push_back ({ list[(size_t) i].name, 2000 + i });
        for (int i = 0; i < userFiles.size(); ++i)
            if (favs.contains (Presets::favKeyUser (userFiles[i].getFileNameWithoutExtension())))
                fav.items.push_back ({ userFiles[i].getFileNameWithoutExtension(), 3000 + i });
        if (! fav.items.empty()) sections.push_back (fav);
    }
    for (int i = 0; i < (int) list.size(); ++i)
    {
        if (sections.empty() || sections.back().heading != list[(size_t) i].category || sections.back().items.front().second >= 2000)
            sections.push_back ({ list[(size_t) i].category, {} });
        sections.back().items.push_back ({ list[(size_t) i].name, i + 1 });
    }
    if (! userFiles.isEmpty())
    {
        Section mine { "Your Presets", {} };
        for (int i = 0; i < userFiles.size(); ++i) mine.items.push_back ({ userFiles[i].getFileNameWithoutExtension(), 1000 + i });
        sections.push_back (mine);
    }

    // about 26 rows per column, breaking only between categories
    int totalRows = 0;
    for (auto& sec : sections) totalRows += (int) sec.items.size() + 1;
    const int numCols = juce::jlimit (1, 5, (totalRows + 25) / 26);
    const int perCol = (totalRows + numCols - 1) / numCols;

    int rowsInCol = 0, col = 1;
    for (size_t k = 0; k < sections.size(); ++k)
    {
        auto& sec = sections[k];
        presetBox.addSectionHeading (sec.heading);
        for (auto& [name, id] : sec.items) presetBox.addItem (name, id);
        rowsInCol += (int) sec.items.size() + 1;
        if (col < numCols && k + 1 < sections.size())
        {
            const int nextRows = (int) sections[k + 1].items.size() + 1;
            // break here if the next section would overshoot this column by more than it undershoots
            if (rowsInCol + nextRows - perCol > perCol - rowsInCol)
            {
                presetBox.getRootMenu()->addColumnBreak();
                rowsInCol = 0;
                ++col;
            }
        }
    }
    syncPresetBox();
}

void EvilFXEditor::syncPresetBox()
{
    const auto name = proc.apvts.state.getProperty ("presetName", "Init (Clean)").toString();
    if (presetBox.getText() == name) return;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == name && presetBox.getItemId (i) < 2000) { presetBox.setSelectedId (presetBox.getItemId (i), juce::dontSendNotification); return; }
    presetBox.setText (name, juce::dontSendNotification);
}

void EvilFXEditor::savePresetDialog()
{
    auto* aw = new juce::AlertWindow ("Save Preset", "Name your preset. It's saved to Documents/Evil FX/Presets.", juce::MessageBoxIconType::NoIcon, this);
    auto current = proc.apvts.state.getProperty ("presetName", "My Preset").toString();
    if (current == "Init (Clean)" || current == "Randomized") current = "My Preset";
    aw->addTextEditor ("name", current, "Preset name:");
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<EvilFXEditor> safe (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = aw->getTextEditorContents ("name");
        if (Presets::saveUser (safe->proc.apvts, name))
        {
            safe->rebuildPresetMenu();
            safe->showNote ("Saved \"" + name.trim() + "\" to Documents/Evil FX/Presets");
        }
        else safe->showNote ("Couldn't save that preset - try another name");
    }), true);
}

void EvilFXEditor::showHelp()
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Evil FX v" EVIL_VERSION,
        "Free & open-source multi-FX by Xain Jaya.\n\n"
        "+ ADD FX: pick effects for your rack. Ticked ones are already in it; picking one jumps to its card.\n"
        "X on a card removes it, the power button bypasses it without removing it.\n"
        "Signal flows left to right, top to bottom, in the numbered order.\n"
        "Drag a card's title bar to move it in the chain; right-click a title bar for more.\n\n"
        "UNDO / REDO (Ctrl/Cmd+Z) step back through any change. A / B holds two sounds to compare; A > B copies.\n"
        "BROWSE (Ctrl/Cmd+F) searches every preset; click a star to favourite it. Left/Right arrows step through presets.\n"
        "Double-click a card's title bar to bypass it.\n\n"
        "Beat Repeat, Trance Gate, Sidechain Pump, Delay and Reverse Delay sync to your project tempo,\n"
        "and the SYNC dropdowns lock Filter, Talk Box, Chorus, Flanger, Phaser and Tremolo to it too.\n"
        "Sidechain Pump, Noise Gate and Vocoder can use your DAW's sidechain input (SC shows in the status bar).\nFor huge ambient pads try Blur (Freeze holds the sound forever) with Volume Swell.\n\n"
        "Presets: pick one up top, SAVE your own (Documents/Evil FX/Presets), or hit RANDOM.\n"
        "Knobs: drag up/down, double-click to reset, click the value to type one in.\n"
        "The Limiter on Master keeps the output safe; leave it on unless you know why.\n\n"
        "xainjaya.com  -  AGPL-3.0", "Got it", this);
}

void EvilFXEditor::showNote (const juce::String& text)
{
    note = text;
    noteTicks = 60;   // ~4 s at 15 Hz
    desktop.repaint (marqueeArea);
}

//==============================================================================
void EvilFXEditor::timerCallback()
{
    // rack: pick up anything switched on by automation or a preset change from the host
    if (dragModule < 0)
    {
        Rack::addSwitchedOn (proc.apvts);
        if (Rack::get (proc.apvts.state) != lastRack || proc.apvts.state.getProperty ("order").toString() != lastOrder) refreshRack();
    }
    updateMoreHint();
    syncPresetBox();
    updateUndoButtons();


    for (auto* c : cards) c->tickFlash();

    meter.l = proc.meterL.load(); meter.r = proc.meterR.load();
    meter.repaint();

    const int active = proc.countActiveModules(), lat = proc.getLatencySamples();
    const float gr = std::round (proc.gainReductionDb.load() * 10.0f) / 10.0f;
    const float bpm = std::round (proc.currentBpm.load() * 10.0f) / 10.0f;
    activeModules = active;
    const bool sc = proc.sidechainConnected.load();
    if (lat != latency || gr != shownGr || bpm != shownBpm || sc != shownSc) { latency = lat; shownGr = gr; shownBpm = bpm; shownSc = sc; desktop.repaint (statusArea); }

    if (viz.isOn())
        viz.tick ([this] (float* d, int n) { return proc.pullVisualizerSamples (d, n); }, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0);

    if (noteTicks > 0 && --noteTicks == 0) note.clear();
    marqueeX -= viz.isOn() ? 1.3f : 2.6f;
    desktop.repaint (marqueeArea);

    // Time / Rate knobs do nothing while tempo-synced, so fade them
    auto fadeSynced = [this] (int module, const char* knobId, const char* syncId, const char* onId)
    {
        if (auto* t = cards[module]->getSlider (knobId))
        {
            const bool synced = proc.apvts.getRawParameterValue (syncId)->load() > 0.5f;
            const bool on = proc.apvts.getRawParameterValue (onId)->load() > 0.5f;
            const float a = (synced ? 0.3f : 1.0f) * (on ? 1.0f : 0.42f);
            if (std::abs (t->getAlpha() - a) > 0.01f) { t->setAlpha (a); cards[module]->repaint(); }
        }
    };
    fadeSynced (EvilFXProcessor::Delay,     "delay_time",     "delay_sync",  "delay_on");
    fadeSynced (EvilFXProcessor::RevDelay,  "rdelay_time",    "rdelay_sync", "rdelay_on");
    fadeSynced (EvilFXProcessor::Filter,    "filter_lforate", "filter_sync", "filter_on");
    fadeSynced (EvilFXProcessor::Talk,      "talk_rate",      "talk_sync",   "talk_on");
    fadeSynced (EvilFXProcessor::Chorus,    "chorus_rate",    "chorus_sync", "chorus_on");
    fadeSynced (EvilFXProcessor::FlangerFx, "flange_rate",    "flange_sync", "flange_on");
    fadeSynced (EvilFXProcessor::Phaser,    "phaser_rate",    "phaser_sync", "phaser_on");
    fadeSynced (EvilFXProcessor::Trem,      "trem_rate",      "trem_sync",   "trem_on");
}

//==============================================================================
void EvilFXEditor::renderBackground()
{
    desktop.background = juce::Image (juce::Image::RGB, baseW, baseH, true);
    juce::Graphics g (desktop.background);
    switch (juce::jlimit (0, themeNames().size() - 1, (int) proc.apvts.state.getProperty ("theme", 0)))
    {
        case 1:  paintFrutigerMetro (g, baseW, baseH); break;
        case 2:  paintCrimsonGrid (g, baseW, baseH); break;
        default: paintAeroSky (g, baseW, baseH); break;   // Aero Sky is the default
    }
}

void EvilFXEditor::Desktop::paint (juce::Graphics& g)
{
    if (background.isValid()) g.drawImageAt (background, 0, 0);
    const float W = (float) getWidth();

    // header glass
    auto header = juce::Rectangle<float> ((float) margin, 6.0f, W - 2.0f * margin, (float) headerH - 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.9f), 0, header.getY(),
                                             juce::Colour (0xffffe0dc).withAlpha (0.82f), 0, header.getBottom(), false));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillRoundedRectangle (header.withHeight (header.getHeight() * 0.45f).reduced (6.0f, 2.0f), 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (header, 18.0f, 1.2f);

    owner.drawLogo (g, header.withWidth (330.0f));

    g.setColour (col::ink.withAlpha (0.85f));
    g.setFont (sans (10.5f, true));
    g.drawText ("P R E S E T", owner.presetBox.getBounds().withY (22).withHeight (14), juce::Justification::centred);

    // toolbar glass
    const auto tb = owner.toolbarArea.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.88f), 0, tb.getY(),
                                             juce::Colour (0xffffe0dc).withAlpha (0.8f), 0, tb.getBottom(), false));
    g.fillRoundedRectangle (tb, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.drawRoundedRectangle (tb, 12.0f, 1.0f);
    g.setColour (col::ink.withAlpha (0.8f));
    g.setFont (sans (12.5f, true));
    const int textX = owner.clearBtn.getRight() + 14;
    g.drawText (juce::String (owner.inRack) + (owner.inRack == 1 ? " effect" : " effects") + " in your rack",
                juce::Rectangle<int> (textX, owner.toolbarArea.getY(), 200, owner.toolbarArea.getHeight()), juce::Justification::centredLeft);
    g.setColour (col::ink.withAlpha (0.75f));
    g.setFont (sans (11.5f));
    g.drawText ("drag title bars to reorder",
                juce::Rectangle<int> (textX + 165, owner.toolbarArea.getY(), owner.undoBtn.getX() - textX - 170, owner.toolbarArea.getHeight()),
                juce::Justification::centredLeft);
    g.setColour (col::ink.withAlpha (0.85f));
    g.setFont (sans (10.5f, true));
    g.drawText ("THEME", owner.themeBox.getBounds().translated (-52, 0).withWidth (48), juce::Justification::centredRight);

    // Win9x status bar
    g.setColour (col::w98Grey);
    g.fillRect (owner.statusArea);
    bevel (g, owner.statusArea, true);
    for (auto f : { owner.grField, owner.latField, owner.bpmField, owner.marqueeArea }) bevel (g, f, false);

    g.setFont (mono (11.5f));
    g.setColour (juce::Colours::black);
    g.drawText ("BPM: " + juce::String (owner.shownBpm, 1) + (owner.proc.sidechainConnected.load() ? "  SC" : ""),
                owner.bpmField.reduced (6, 0), juce::Justification::centredLeft);
    g.drawText ("LATENCY: " + juce::String (juce::jmax (0, owner.latency)) + " smp", owner.latField.reduced (6, 0), juce::Justification::centredLeft);
    g.drawText ("LIMIT: " + juce::String (juce::jmin (0.0f, owner.shownGr), 1) + " dB", owner.grField.reduced (6, 0), juce::Justification::centredLeft);

    auto inner = owner.marqueeArea;
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (inner.reduced (2));
    if (owner.note.isNotEmpty())
    {
        g.setColour (col::w98Maroon);
        g.setFont (mono (11.5f, true));
        g.drawText (owner.note, inner.reduced (8, 0), juce::Justification::centredLeft);
        return;
    }
    const juce::String text (kMarquee);
    const float tw = juce::GlyphArrangement::getStringWidth (mono (11.5f), text);
    if (owner.marqueeX < -tw) owner.marqueeX += tw;
    for (float x = (float) inner.getX() + owner.marqueeX; x < (float) inner.getRight(); x += tw)
        g.drawText (text, juce::Rectangle<float> (x, (float) inner.getY(), tw + 4.0f, (float) inner.getHeight()), juce::Justification::centredLeft, false);
}

void EvilFXEditor::drawLogo (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float r = 27.0f;
    drawSmiley (g, { area.getX() + 40.0f, area.getCentreY() + 9.0f }, r);

    const float tx = area.getX() + 40.0f + r * 1.62f + 10.0f;   // clear of the cap brim
    juce::GlyphArrangement ga;
    ga.addLineOfText (sans (40.0f, true), "EVIL", tx, area.getCentreY() + 4.0f);
    juce::Path evil; ga.createPath (evil);
    const auto eb = evil.getBounds();
    juce::GlyphArrangement gb;
    gb.addLineOfText (sans (40.0f), "FX", eb.getRight() + 8.0f, area.getCentreY() + 4.0f);
    juce::Path fx; gb.createPath (fx);
    const auto fb = fx.getBounds();

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillPath (evil, juce::AffineTransform::translation (0.0f, 2.0f));
    g.fillPath (fx, juce::AffineTransform::translation (0.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a0208), 0, eb.getY(), col::red, 0, eb.getBottom(), false));
    g.fillPath (evil);
    g.setGradientFill (juce::ColourGradient (col::ember, 0, fb.getY(), juce::Colour (0xffc0101c), 0, fb.getBottom(), false));
    g.fillPath (fx);

    g.setFont (sans (11.0f, true));
    g.setColour (col::ink.withAlpha (0.75f));
    g.drawText ("v" EVIL_VERSION "  -  open-source fx by xain jaya", juce::Rectangle<float> (tx, area.getCentreY() + 10.0f, 260.0f, 16.0f), juce::Justification::centredLeft);
}

//==============================================================================
void EvilFXEditor::RackArea::paint (juce::Graphics& g)
{
    if (! empty) return;
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (520.0f, 170.0f).translated (0.0f, -footerH / 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.92f), 0, r.getY(),
                                             juce::Colour (0xffffe4e2).withAlpha (0.88f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.0f);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (r, 18.0f, 1.2f);
    drawSmiley (g, { r.getX() + 78.0f, r.getCentreY() + 14.0f }, 34.0f);   // the mascot keeps you company
    auto text = r.withTrimmedLeft (150.0f).reduced (8.0f, 0.0f);
    g.setColour (col::ink);
    g.setFont (sans (24.0f, true));
    g.drawText ("Your rack is empty", text.withTrimmedBottom (60.0f), juce::Justification::centredLeft);
    g.setFont (sans (14.0f));
    g.setColour (col::ink.withAlpha (0.85f));
    g.drawText ("Hit + ADD FX to pick your effects,", text.withTrimmedTop (66.0f).withTrimmedBottom (34.0f), juce::Justification::centredLeft);
    g.drawText ("load a preset, or hit RANDOM.", text.withTrimmedTop (92.0f).withTrimmedBottom (8.0f), juce::Justification::centredLeft);
}

void EvilFXEditor::SiteLink::paintButton (juce::Graphics& g, bool hover, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float corner = r.getHeight() * 0.5f;
    g.setGradientFill (juce::ColourGradient (down ? col::red : juce::Colour (0xffff6a5a), 0, r.getY(),
                                             hover ? col::red : juce::Colour (0xffb00c18), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (hover ? 0.5f : 0.38f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (corner * 0.6f, 1.5f), corner * 0.4f);
    g.setColour (col::redDeep);
    g.drawRoundedRectangle (r, corner, 1.2f);

    // little globe
    const juce::Point<float> c (r.getX() + corner + 4.0f, r.getCentreY());
    const float gr = r.getHeight() * 0.3f;
    g.setColour (juce::Colours::white);
    g.drawEllipse (c.x - gr, c.y - gr, gr * 2, gr * 2, 2.0f);
    g.drawEllipse (c.x - gr * 0.45f, c.y - gr, gr * 0.9f, gr * 2, 1.4f);
    g.drawLine (c.x - gr, c.y, c.x + gr, c.y, 1.4f);

    g.setFont (sans (22.0f, true));
    auto textArea = r.withTrimmedLeft (corner * 2.0f + 6.0f).withTrimmedRight (corner * 0.6f);
    g.drawText ("xainjaya.com", textArea, juce::Justification::centred);
    if (hover)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (sans (22.0f, true), "xainjaya.com");
        g.fillRect (textArea.getCentreX() - tw * 0.5f, textArea.getCentreY() + 12.0f, tw, 1.5f);
    }
}

void EvilFXEditor::MoreHint::paintButton (juce::Graphics& g, bool hover, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float corner = r.getHeight() * 0.5f;
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), corner);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff6a5a), 0, r.getY(), down ? col::redDeep : juce::Colour (0xffb00c18), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (hover ? 0.55f : 0.4f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (corner * 0.5f, 1.0f), corner * 0.4f);
    g.setColour (juce::Colours::white);
    g.setFont (sans (12.5f, true));
    juce::Path arrow;
    arrow.addTriangle (r.getX() + 16.0f, r.getCentreY() - 3.0f, r.getX() + 26.0f, r.getCentreY() - 3.0f, r.getX() + 21.0f, r.getCentreY() + 4.0f);
    g.fillPath (arrow);
    g.drawText (juce::String (count) + " more FX below", r.withTrimmedLeft (22.0f), juce::Justification::centred);
}

void EvilFXEditor::Meter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float barH = (area.getHeight() - 3.0f) * 0.5f;
    auto drawBar = [&g, barH] (juce::Rectangle<float> b, float level)
    {
        g.setColour (col::ink.withAlpha (0.18f));
        g.fillRoundedRectangle (b, barH * 0.5f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto fill = b.withWidth (b.getWidth() * frac);
        juce::ColourGradient grad (col::redDeep, b.getX(), 0, col::ember, b.getRight() * 0.85f, 0, false);
        grad.addColour (0.97, juce::Colour (0xffffe040));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, barH * 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillRoundedRectangle (fill.withHeight (barH * 0.45f).reduced (1.0f, 0.0f), barH * 0.25f);
    };
    drawBar (area.removeFromTop (barH), l);
    area.removeFromTop (3.0f);
    drawBar (area.removeFromTop (barH), r);
}
