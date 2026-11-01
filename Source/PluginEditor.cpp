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
        "GLITCH, GATE and PUMP lock to your project tempo  ~  hit SAVE to keep your own presets  ~  "
        "hit RANDOM for happy accidents  ~  pick a THEME you like  ~  double-click any knob to reset it  ~  "
        "the safety limiter keeps your speakers alive  ~  free & open source forever  ~  sign the guestbook @ xainjaya.com  ";

    std::vector<CardSpec> moduleSpecs()
    {
        std::vector<CardSpec> s {
            { 0, "Crimson Filter", "sweep", false, "filter_on",
              { { "filter_cutoff", "Cutoff" }, { "filter_reso", "Reso" }, { "filter_lforate", "LFO Rate" }, { "filter_lfodepth", "LFO Depth" } },
              { "filter_mode" }, {} },
            { 0, "Glass EQ", "tone", false, "eq_on",
              { { "eq_low", "Low" }, { "eq_mid", "Mid" }, { "eq_midfreq", "Mid Freq" }, { "eq_high", "High" } }, {}, {} },
            { 0, "Scorch Drive", "heat", false, "gloss_on",
              { { "gloss_drive", "Drive" }, { "gloss_tone", "Tone" }, { "gloss_mix", "Mix" } }, {}, {} },
            { 0, "NIGHTCORE.EXE", "", true, "pitch_on",
              { { "pitch_semi", "Pitch" }, { "pitch_window", "Grain" }, { "pitch_mix", "Mix" } }, {}, {} },
            { 0, "RINGTONE.EXE", "", true, "ring_on",
              { { "ring_freq", "Freq" }, { "ring_shape", "Shape" }, { "ring_warble", "Warble" }, { "ring_mix", "Mix" } }, {}, {} },
            { 0, "TTS.EXE", "", true, "tts_on",
              { { "tts_pitch", "Pitch" }, { "tts_metal", "Metal" }, { "tts_buzz", "Buzz" }, { "tts_mix", "Mix" } }, {}, {} },
            { 0, "CODEC.EXE", "", true, "codec_on",
              { { "codec_alias", "Aliasing" }, { "codec_mix", "Mix" } }, { "codec_rate", "codec_type" }, {} },
            { 0, "8BIT.EXE", "", true, "crush_on",
              { { "crush_bits", "Bits" }, { "crush_rate", "Rate" }, { "crush_mix", "Mix" } }, {}, {} },
            { 0, "DIALUP.EXE", "", true, "dial_on",
              { { "dial_band", "Band" }, { "dial_noise", "Noise" }, { "dial_crackle", "Crackle" }, { "dial_mix", "Mix" } },
              {}, { { "dial_modem", "Modem squeal" } } },
            { 0, "VINYL.EXE", "", true, "vinyl_on",
              { { "vinyl_crackle", "Crackle" }, { "vinyl_wear", "Wear" }, { "vinyl_wow", "Wow" }, { "vinyl_mix", "Mix" } }, {}, {} },
            { 0, "LOBITRATE.EXE", "", true, "lobit_on",
              { { "lobit_kbps", "Bitrate" }, { "lobit_swirl", "Swirl" }, { "lobit_mix", "Mix" } }, {}, {} },
            { 0, "PACKETLOSS.EXE", "", true, "pkt_on",
              { { "pkt_loss", "Loss" }, { "pkt_size", "Packet" }, { "pkt_mix", "Mix" } }, { "pkt_conceal" }, {} },
            { 0, "BUFFERING.EXE", "", true, "buf_on",
              { { "buf_chance", "Chance" }, { "buf_slice", "Slice" }, { "buf_dropout", "Dropout" }, { "buf_mix", "Mix" } }, {}, {} },
            { 0, "CORRUPT.EXE", "", true, "corrupt_on",
              { { "corrupt_amount", "Amount" }, { "corrupt_bits", "Bits" }, { "corrupt_reverse", "Reverse" }, { "corrupt_mix", "Mix" } }, {}, {} },
            { 0, "LAG.EXE", "", true, "lag_on",
              { { "lag_chance", "Chance" }, { "lag_length", "Length" }, { "lag_depth", "Depth" }, { "lag_mix", "Mix" } }, {}, {} },
            { 0, "JITTER.EXE", "", true, "jit_on",
              { { "jit_wow", "Wow" }, { "jit_flutter", "Flutter" }, { "jit_drift", "Drift" }, { "jit_mix", "Mix" } }, {}, {} },
            { 0, "GLITCH.EXE", "", true, "glitch_on",
              { { "glitch_chance", "Chance" }, { "glitch_mix", "Mix" } }, { "glitch_div", "glitch_style" }, {} },
            { 0, "GATE.EXE", "", true, "gate_on",
              { { "gate_depth", "Depth" }, { "gate_smooth", "Smooth" } }, { "gate_pattern", "gate_div" }, {} },
            { 0, "PUMP.EXE", "", true, "pump_on",
              { { "pump_depth", "Depth" }, { "pump_shape", "Release" } }, { "pump_div" }, {} },
            { 0, "Angel Chorus", "choir", false, "chorus_on",
              { { "chorus_rate", "Rate" }, { "chorus_depth", "Depth" }, { "chorus_fb", "Feedback" }, { "chorus_mix", "Mix" } }, {}, {} },
            { 0, "Cloud Phaser", "drift", false, "phaser_on",
              { { "phaser_rate", "Rate" }, { "phaser_depth", "Depth" }, { "phaser_fb", "Feedback" }, { "phaser_mix", "Mix" } }, {}, {} },
            { 0, "Trem / Pan", "motion", false, "trem_on",
              { { "trem_rate", "Rate" }, { "trem_depth", "Depth" }, { "trem_shape", "Shape" }, { "trem_pan", "Pan" } }, {}, {} },
            { 0, "Stereo Spread", "wide", false, "spread_on",
              { { "spread_width", "Width" }, { "spread_motion", "Motion" }, { "spread_mix", "Mix" } }, {}, {} },
            { 0, "Halo Delay", "echo", false, "delay_on",
              { { "delay_time", "Time" }, { "delay_fb", "Feedback" }, { "delay_tone", "Tone" }, { "delay_mix", "Mix" } },
              { "delay_sync" }, { { "delay_pp", "Ping-pong" } } },
            { 0, "Cathedral Verb", "space", false, "verb_on",
              { { "verb_size", "Size" }, { "verb_damp", "Damp" }, { "verb_width", "Width" }, { "verb_mix", "Mix" } }, {}, {} },
            { 0, "Glory Comp", "glue", false, "comp_on",
              { { "comp_squash", "Squash" }, { "comp_ratio", "Ratio" }, { "comp_speed", "Speed" }, { "comp_mix", "Mix" } }, {}, {} },
            { 0, "SPEAKER.EXE", "", true, "spk_on",
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
            { "filter_on", "Tone & Dynamics" }, { "eq_on", "Tone & Dynamics" }, { "gloss_on", "Tone & Dynamics" }, { "comp_on", "Tone & Dynamics" },
            { "pitch_on", "Pitch & Voice" }, { "ring_on", "Pitch & Voice" }, { "tts_on", "Pitch & Voice" },
            { "glitch_on", "Rhythm (BPM synced)" }, { "gate_on", "Rhythm (BPM synced)" }, { "pump_on", "Rhythm (BPM synced)" },
            { "chorus_on", "Space & Motion" }, { "phaser_on", "Space & Motion" }, { "trem_on", "Space & Motion" },
            { "spread_on", "Space & Motion" }, { "delay_on", "Space & Motion" }, { "verb_on", "Space & Motion" } };
        auto it = cat.find (onId);
        return it != cat.end() ? it->second : juce::String ("Lo-Fi & Internet Damage");
    }

    bool isBpmModule (const juce::String& onId)
    {
        return onId == "glitch_on" || onId == "gate_on" || onId == "pump_on" || onId == "delay_on";
    }

    //==========================================================================
    // Backgrounds. Each is painted once into an image when chosen.
    void paintFrutigerMetro (juce::Graphics& g, int W, int H)
    {
        const float w = (float) W, h = (float) H;
        juce::ColourGradient base (juce::Colour (0xff4a0412), 0, 0, juce::Colour (0xffff5a3c), w, h, false);
        base.addColour (0.5, juce::Colour (0xffc8102e));
        g.setGradientFill (base);
        g.fillAll();

        // soft glow top-left, like light hitting glass
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.25f), w * 0.15f, h * 0.05f,
                                                 juce::Colours::white.withAlpha (0.0f), w * 0.6f, h * 0.5f, true));
        g.fillRect (0, 0, W, H);

        juce::Random rnd (2012);
        // big bokeh circles
        for (int i = 0; i < 26; ++i)
        {
            const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h, r = 20.0f + rnd.nextFloat() * 110.0f;
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.02f + rnd.nextFloat() * 0.08f), x, y,
                                                     juce::Colours::white.withAlpha (0.0f), x + r, y, true));
            g.fillEllipse (x - r, y - r, r * 2, r * 2);
        }

        // tilted wall of flat "live tiles"
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-0.21f, w * 0.7f, h * 0.5f));
            const float tile = 104.0f, tg = 10.0f;
            const juce::Colour accents[] { juce::Colours::white, juce::Colour (0xffffb36a), juce::Colour (0xff2a0006), juce::Colour (0xffff7a8a) };
            for (int row = -2; row < 9; ++row)
                for (int col = 0; col < 9; ++col)
                {
                    if (rnd.nextFloat() < 0.18f) continue;   // gaps make it feel arranged, not tiled
                    const bool wide = rnd.nextFloat() < 0.2f;
                    const float x = w * 0.42f + (float) col * (tile + tg), y = (float) row * (tile + tg);
                    const auto c = accents[rnd.nextInt (10) < 7 ? 0 : 1 + rnd.nextInt (3)];
                    const float a = c == juce::Colours::white ? 0.04f + rnd.nextFloat() * 0.09f : 0.12f + rnd.nextFloat() * 0.12f;
                    auto rect = juce::Rectangle<float> (x, y, wide ? tile * 2 + tg : tile, tile);
                    g.setColour (c.withAlpha (a));
                    g.fillRect (rect);
                    g.setColour (juce::Colours::white.withAlpha (a * 0.8f));   // a hint of Frutiger gloss on each tile
                    g.fillRect (rect.withHeight (tile * 0.35f));
                    if (wide) col++;
                }
        }

        // thin Metro accent lines + a big ring
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        for (int i = 0; i < 5; ++i)
        {
            const float y0 = h * (0.15f + 0.17f * (float) i);
            g.drawLine (0.0f, y0 + 120.0f, w, y0 - 140.0f, 1.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (w * 0.08f - 260.0f, h * 0.85f - 260.0f, 520.0f, 520.0f, 2.0f);
        g.drawEllipse (w * 0.08f - 300.0f, h * 0.85f - 300.0f, 600.0f, 600.0f, 1.0f);

        // broad diagonal light beam
        juce::Path beam;
        beam.startNewSubPath (w * 0.25f, 0.0f); beam.lineTo (w * 0.45f, 0.0f);
        beam.lineTo (w * 0.05f, h); beam.lineTo (-w * 0.15f, h); beam.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.12f), w * 0.3f, 0.0f,
                                                 juce::Colours::white.withAlpha (0.0f), 0.0f, h, false));
        g.fillPath (beam);
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

    void paintMidnight (juce::Graphics& g, int W, int H)
    {
        const float w = (float) W, h = (float) H;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a0308), w * 0.5f, h * 0.3f, juce::Colour (0xff030001), w * 1.1f, h, true));
        g.fillAll();
        g.setColour (juce::Colour (0xffff3050).withAlpha (0.08f));
        for (int y = 12; y < H; y += 24)
            for (int x = 12; x < W; x += 24)
                g.fillRect ((float) x, (float) y, 1.5f, 1.5f);
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
    static const juce::StringArray n { "Frutiger Metro", "Crimson Grid", "Aero Sky", "Midnight" };
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
    for (auto* c : std::initializer_list<juce::Component*> { &addBtn, &clearBtn, &helpBtn, &themeBox }) desktop.addAndMakeVisible (c);

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
        if (id >= 1000) { if (juce::isPositiveAndBelow (id - 1000, userFiles.size())) Presets::loadUser (proc.apvts, userFiles[id - 1000]); }
        else Presets::apply (proc.apvts, id - 1);
        refreshRack();
    };
    presetBox.setTooltip ("Factory and user presets");
    auto step = [this] (int d)
    {
        juce::Array<int> ids;
        for (int i = 0; i < presetBox.getNumItems(); ++i) ids.add (presetBox.getItemId (i));
        if (ids.isEmpty()) return;
        const int cur = juce::jmax (0, ids.indexOf (presetBox.getSelectedId()));
        presetBox.setSelectedId (ids[(cur + d + ids.size()) % ids.size()]);
    };
    prevBtn.onClick = [step] { step (-1); };
    nextBtn.onClick = [step] { step (1); };
    saveBtn.setTooltip ("Save the current sound as your own preset");
    saveBtn.onClick = [this] { savePresetDialog(); };
    randomBtn.setTooltip ("Switch on a few random effects with random settings");
    randomBtn.onClick = [this] { Presets::randomize (proc.apvts); refreshRack(); syncPresetBox(); showNote ("Randomized! Hit SAVE if you like it."); };
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevBtn, &nextBtn, &saveBtn, &randomBtn }) desktop.addAndMakeVisible (c);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setResizeLimits (baseW / 2, baseH / 2, baseW * 3 / 2, baseH * 3 / 2);
    setSize (baseW * 8 / 10, baseH * 8 / 10);

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
    desktop.setBounds (0, 0, baseW, baseH);
    desktop.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW));

    toolbarArea = { margin, headerH + 2, baseW - 2 * margin, toolbarH };
    auto tb = toolbarArea.reduced (8, 4);
    addBtn.setBounds (tb.removeFromLeft (120)); tb.removeFromLeft (6);
    clearBtn.setBounds (tb.removeFromLeft (76));
    helpBtn.setBounds (tb.removeFromRight (tb.getHeight() + 2)); tb.removeFromRight (8);
    themeBox.setBounds (tb.removeFromRight (170)); tb.removeFromRight (6);

    const int top = toolbarArea.getBottom() + 6, bottom = baseH - statusH - 10;
    viewport.setBounds (margin, top, baseW - 2 * margin, bottom - top - stripH - gap);
    master->setBounds (margin, bottom - stripH, 470, stripH);
    meter.setBounds (master->getOptionSpare().translated (master->getX(), master->getY()).reduced (4, 3));
    viz.setBounds (master->getRight() + gap, bottom - stripH, baseW - 2 * margin - 470 - gap, stripH);
    moreHint.setBounds (viewport.getX() + (viewport.getWidth() - 14) / 2 - 90, viewport.getBottom() - footerH + 2, 180, footerH - 4);

    presetBox.setBounds (baseW / 2 - 200, 40, 300, 30);
    prevBtn.setBounds (presetBox.getX() - 38, 39, 32, 32);
    nextBtn.setBounds (presetBox.getRight() + 6, 39, 32, 32);
    saveBtn.setBounds (nextBtn.getRight() + 10, 41, 66, 28);
    randomBtn.setBounds (saveBtn.getRight() + 6, 41, 78, 28);

    auto right = juce::Rectangle<int> (margin, 6, baseW - 2 * margin, headerH - 10).removeFromRight (318).reduced (10, 14);
    counterArea = right.removeFromLeft (110);
    statusArea  = juce::Rectangle<int> (margin, baseH - statusH - 6, baseW - 2 * margin, statusH);
    auto inner = statusArea.reduced (3);
    grField  = inner.removeFromRight (120); inner.removeFromRight (3);
    latField = inner.removeFromRight (150); inner.removeFromRight (3);
    bpmField = inner.removeFromRight (110); inner.removeFromRight (3);
    marqueeArea = inner;

    renderBackground();
    refreshRack();
}

//==============================================================================
void EvilFXEditor::showAddMenu()
{
    const auto rack = Rack::get (proc.apvts.state);
    juce::PopupMenu menu;
    juce::StringArray order { "Tone & Dynamics", "Pitch & Voice", "Lo-Fi & Internet Damage", "Rhythm (BPM synced)", "Space & Motion" };
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
        Rack::set (proc.apvts.state, module, true);
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
    Rack::set (proc.apvts.state, module, false);
    if (auto* on = proc.apvts.getParameter (Rack::onId (module))) on->setValueNotifyingHost (0.0f);
    refreshRack();
}

void EvilFXEditor::clearRack()
{
    for (int i = 0; i < Rack::numModules; ++i)
        if (auto* on = proc.apvts.getParameter (Rack::onId (i))) on->setValueNotifyingHost (0.0f);
    Rack::syncFromParams (proc.apvts);
    refreshRack();
    showNote ("Rack cleared. Hit + ADD FX to start again.");
}

void EvilFXEditor::refreshRack()
{
    const auto rack = Rack::get (proc.apvts.state);
    lastRack = rack;

    const int areaW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int viewH = viewport.getHeight();
    const int cardW = (areaW - (cols - 1) * gap) / cols;
    const int cardH = (viewH - gap - footerH) / 2;   // footer leaves room for the "more below" button

    int shown = 0;
    for (int i = 0; i < cards.size(); ++i)
    {
        const bool in = rack[i] == '1';
        cards[i]->setVisible (in);
        if (in)
        {
            cards[i]->setBounds ((shown % cols) * (cardW + gap), (shown / cols) * (cardH + gap), cardW, cardH);
            ++shown;
        }
    }
    inRack = shown;
    const int rows = (shown + cols - 1) / cols;
    rackArea.empty = shown == 0;
    rackArea.setSize (areaW, juce::jmax (viewH, rows * (cardH + gap) - gap + footerH));
    rackArea.repaint();
    desktop.repaint (toolbarArea);
    updateMoreHint();
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
    juce::String lastCat;
    const auto& list = Presets::all();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        if (lastCat != list[(size_t) i].category) { lastCat = list[(size_t) i].category; presetBox.addSectionHeading (lastCat); }
        presetBox.addItem (list[(size_t) i].name, i + 1);
    }
    userFiles = Presets::userPresets();
    if (! userFiles.isEmpty())
    {
        presetBox.addSectionHeading ("Your Presets");
        for (int i = 0; i < userFiles.size(); ++i) presetBox.addItem (userFiles[i].getFileNameWithoutExtension(), 1000 + i);
    }
    syncPresetBox();
}

void EvilFXEditor::syncPresetBox()
{
    const auto name = proc.apvts.state.getProperty ("presetName", "Init (Clean)").toString();
    if (presetBox.getText() == name) return;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == name) { presetBox.setSelectedId (presetBox.getItemId (i), juce::dontSendNotification); return; }
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
        "Signal flows left to right, top to bottom, in the numbered order.\n\n"
        "GLITCH, GATE, PUMP and Halo Delay sync to your project tempo.\n\n"
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
    Rack::addSwitchedOn (proc.apvts);
    if (Rack::get (proc.apvts.state) != lastRack) refreshRack();
    updateMoreHint();
    syncPresetBox();

    for (auto* c : cards) c->tickFlash();

    meter.l = proc.meterL.load(); meter.r = proc.meterR.load();
    meter.repaint();

    const int active = proc.countActiveModules(), lat = proc.getLatencySamples();
    const float gr = std::round (proc.gainReductionDb.load() * 10.0f) / 10.0f;
    const float bpm = std::round (proc.currentBpm.load() * 10.0f) / 10.0f;
    if (active != activeModules) { activeModules = active; desktop.repaint (counterArea); }
    if (lat != latency || gr != shownGr || bpm != shownBpm) { latency = lat; shownGr = gr; shownBpm = bpm; desktop.repaint (statusArea); }

    if (viz.isOn())
        viz.tick ([this] (float* d, int n) { return proc.pullVisualizerSamples (d, n); }, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0);

    if (noteTicks > 0 && --noteTicks == 0) note.clear();
    marqueeX -= viz.isOn() ? 1.3f : 2.6f;
    desktop.repaint (marqueeArea);

    // the delay Time knob does nothing while tempo-synced, so fade it
    if (auto* t = cards[EvilFXProcessor::Delay]->getSlider ("delay_time"))
    {
        const bool synced = proc.apvts.getRawParameterValue ("delay_sync")->load() > 0.5f;
        const bool on = proc.apvts.getRawParameterValue ("delay_on")->load() > 0.5f;
        const float a = (synced ? 0.3f : 1.0f) * (on ? 1.0f : 0.42f);
        if (std::abs (t->getAlpha() - a) > 0.01f) { t->setAlpha (a); cards[EvilFXProcessor::Delay]->repaint(); }
    }
}

//==============================================================================
void EvilFXEditor::renderBackground()
{
    desktop.background = juce::Image (juce::Image::RGB, baseW, baseH, true);
    juce::Graphics g (desktop.background);
    switch (juce::jlimit (0, themeNames().size() - 1, (int) proc.apvts.state.getProperty ("theme", 0)))
    {
        case 1:  paintCrimsonGrid (g, baseW, baseH); break;
        case 2:  paintAeroSky (g, baseW, baseH); break;
        case 3:  paintMidnight (g, baseW, baseH); break;
        default: paintFrutigerMetro (g, baseW, baseH); break;
    }
}

void EvilFXEditor::Desktop::paint (juce::Graphics& g)
{
    if (background.isValid()) g.drawImageAt (background, 0, 0);
    const float W = (float) getWidth();

    // header glass
    auto header = juce::Rectangle<float> ((float) margin, 6.0f, W - 2.0f * margin, (float) headerH - 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.7f), 0, header.getY(),
                                             juce::Colour (0xffffd6d0).withAlpha (0.46f), 0, header.getBottom(), false));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillRoundedRectangle (header.withHeight (header.getHeight() * 0.45f).reduced (6.0f, 2.0f), 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (header, 18.0f, 1.2f);

    owner.drawLogo (g, header.withWidth (330.0f));

    g.setColour (col::ink.withAlpha (0.7f));
    g.setFont (sans (10.5f, true));
    g.drawText ("P R E S E T", owner.presetBox.getBounds().withY (22).withHeight (14), juce::Justification::centred);

    // toolbar glass
    const auto tb = owner.toolbarArea.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.6f), 0, tb.getY(),
                                             juce::Colour (0xffffd6d0).withAlpha (0.4f), 0, tb.getBottom(), false));
    g.fillRoundedRectangle (tb, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.drawRoundedRectangle (tb, 12.0f, 1.0f);
    g.setColour (col::ink.withAlpha (0.8f));
    g.setFont (sans (12.5f, true));
    const int textX = owner.clearBtn.getRight() + 14;
    g.drawText (juce::String (owner.inRack) + (owner.inRack == 1 ? " effect" : " effects") + " in your rack",
                juce::Rectangle<int> (textX, owner.toolbarArea.getY(), 200, owner.toolbarArea.getHeight()), juce::Justification::centredLeft);
    g.setColour (col::ink.withAlpha (0.55f));
    g.setFont (sans (11.5f));
    g.drawText ("signal flows left to right, top to bottom",
                juce::Rectangle<int> (textX + 175, owner.toolbarArea.getY(), 300, owner.toolbarArea.getHeight()), juce::Justification::centredLeft);
    g.setColour (col::ink.withAlpha (0.7f));
    g.setFont (sans (10.5f, true));
    g.drawText ("THEME", owner.themeBox.getBounds().translated (-52, 0).withWidth (48), juce::Justification::centredRight);

    // LED counter + 88x31 buttons
    auto counter = owner.counterArea;
    g.setColour (juce::Colours::black);
    g.fillRect (counter);
    bevel (g, counter, false);
    g.setFont (mono (9.5f, true));
    g.setColour (juce::Colour (0xffff9c9c));
    g.drawText ("FX ONLINE", counter.removeFromTop (18), juce::Justification::centred);
    g.setFont (mono (22.0f, true));
    g.setColour (juce::Colour (0xffff2a2a));
    g.drawText (juce::String (juce::jmax (0, owner.activeModules)).paddedLeft ('0', 2) + "/" + juce::String (Rack::numModules),
                counter.withTrimmedBottom (2), juce::Justification::centred);

    const int bx = owner.counterArea.getRight() + 10, by = owner.counterArea.getY();
    const int bw = 88, bh = (owner.counterArea.getHeight() - 4) / 2;
    owner.drawBadge (g, { bx,      by,          bw, bh }, "XAINJAYA", ".COM",     col::w98Maroon, juce::Colours::white);
    owner.drawBadge (g, { bx + 92, by,          bw, bh }, "ANGEL",    "APPROVED", juce::Colours::white, juce::Colour (0xffb8860b));
    owner.drawBadge (g, { bx,      by + bh + 4, bw, bh }, "OPEN",     "SOURCE",   juce::Colour (0xff111111), col::ember);
    owner.drawBadge (g, { bx + 92, by + bh + 4, bw, bh }, "56K",      "FRIENDLY", col::w98Brick, juce::Colours::white);

    // Win9x status bar
    g.setColour (col::w98Grey);
    g.fillRect (owner.statusArea);
    bevel (g, owner.statusArea, true);
    for (auto f : { owner.grField, owner.latField, owner.bpmField, owner.marqueeArea }) bevel (g, f, false);

    g.setFont (mono (11.5f));
    g.setColour (juce::Colours::black);
    g.drawText ("BPM: " + juce::String (owner.shownBpm, 1), owner.bpmField.reduced (6, 0), juce::Justification::centredLeft);
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
    const juce::Point<float> c (area.getX() + 40.0f, area.getCentreY() + 5.0f);
    const float r = 25.0f;

    g.setColour (col::ember.withAlpha (0.3f));
    g.fillEllipse (c.x - r - 4, c.y - r - 4, (r + 4) * 2, (r + 4) * 2);
    drawOrb (g, c, r, col::redPale, juce::Colour (0xffff3644), col::redDeep);

    g.setColour (col::ink);   // friendly face
    g.fillEllipse (c.x - 10.0f, c.y - 7.0f, 6.0f, 8.0f);
    g.fillEllipse (c.x + 4.0f, c.y - 7.0f, 6.0f, 8.0f);
    juce::Path smile;
    smile.addCentredArc (c.x, c.y + 1.0f, 12.0f, 10.0f, 0.0f, 2.1f, 4.18f, true);
    g.strokePath (smile, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.5f), c.x, c.y - r * 0.92f, juce::Colours::white.withAlpha (0.0f), c.x, c.y - r * 0.1f, false);
    g.setGradientFill (spec);
    g.fillEllipse (c.x - r * 0.64f, c.y - r * 0.9f, r * 1.28f, r * 0.8f);

    const auto halo = juce::Rectangle<float> (c.x - 20.0f, c.y - r - 14.0f, 40.0f, 10.0f);   // golden halo
    g.setColour (juce::Colour (0xffffe08a).withAlpha (0.4f));
    g.drawEllipse (halo.expanded (1.5f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff6c0), halo.getX(), halo.getY(), juce::Colour (0xffd49a1a), halo.getRight(), halo.getBottom(), false));
    g.drawEllipse (halo, 2.6f);

    const float tx = c.x + r + 16.0f;
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

void EvilFXEditor::drawBadge (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& top, const juce::String& bottom, juce::Colour bg, juce::Colour fg)
{
    g.setColour (bg);
    g.fillRect (r);
    bevel (g, r, true);
    g.setColour (fg);
    g.setFont (mono (10.5f, true));
    g.drawText (top, r.withHeight (r.getHeight() / 2).translated (0, 2), juce::Justification::centred);
    g.drawText (bottom, r.withTrimmedTop (r.getHeight() / 2).translated (0, -2), juce::Justification::centred);
}

//==============================================================================
void EvilFXEditor::RackArea::paint (juce::Graphics& g)
{
    if (! empty) return;
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (480.0f, 150.0f).translated (0.0f, -footerH / 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.7f), 0, r.getY(),
                                             juce::Colour (0xffffd8d6).withAlpha (0.5f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (r, 18.0f, 1.2f);
    g.setColour (col::ink);
    g.setFont (sans (24.0f, true));
    g.drawText ("Your rack is empty", r.withTrimmedBottom (50.0f), juce::Justification::centred);
    g.setFont (sans (14.0f));
    g.setColour (col::ink.withAlpha (0.75f));
    g.drawText ("Hit + ADD FX to pick your effects,", r.withTrimmedTop (60.0f).withTrimmedBottom (30.0f), juce::Justification::centred);
    g.drawText ("load a preset, or hit RANDOM.", r.withTrimmedTop (86.0f).withTrimmedBottom (8.0f), juce::Justification::centred);
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
