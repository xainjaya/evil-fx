#include "PluginEditor.h"

using namespace aero;

namespace
{
    constexpr int headerH = 96, statusH = 26, margin = 14, gap = 10, sidebarW = 252;
    constexpr int libHeaderH = 38, libRowH = 28;
    const char* kMarquee =
        "  *** Welcome to Evil FX ***  pick your poison from the FX Library on the left  ~  "
        "signal flows left to right, top to bottom  ~  double-click any knob to reset it  ~  "
        "click a value to type it in  ~  the safety limiter keeps your speakers alive  ~  "
        "free & open source forever  ~  sign the guestbook @ xainjaya.com  ";

    // card definitions, in chain order (index == module index in the processor)
    std::vector<CardSpec> moduleSpecs()
    {
        return {
            { 1, "Crimson Filter", "sweep", false, "filter_on",
              { { "filter_cutoff", "Cutoff" }, { "filter_reso", "Reso" }, { "filter_lforate", "LFO Rate" }, { "filter_lfodepth", "LFO Depth" } },
              { "filter_mode" }, {} },
            { 2, "Scorch Drive", "heat", false, "gloss_on",
              { { "gloss_drive", "Drive" }, { "gloss_tone", "Tone" }, { "gloss_mix", "Mix" } }, {}, {} },
            { 3, "CODEC.EXE", "", true, "codec_on",
              { { "codec_alias", "Aliasing" }, { "codec_mix", "Mix" } }, { "codec_rate", "codec_type" }, {} },
            { 4, "8BIT.EXE", "", true, "crush_on",
              { { "crush_bits", "Bits" }, { "crush_rate", "Rate" }, { "crush_mix", "Mix" } }, {}, {} },
            { 5, "DIALUP.EXE", "", true, "dial_on",
              { { "dial_band", "Band" }, { "dial_noise", "Noise" }, { "dial_crackle", "Crackle" }, { "dial_mix", "Mix" } },
              {}, { { "dial_modem", "Modem squeal" } } },
            { 6, "LOBITRATE.EXE", "", true, "lobit_on",
              { { "lobit_kbps", "Bitrate" }, { "lobit_swirl", "Swirl" }, { "lobit_mix", "Mix" } }, {}, {} },
            { 7, "PACKETLOSS.EXE", "", true, "pkt_on",
              { { "pkt_loss", "Loss" }, { "pkt_size", "Packet" }, { "pkt_mix", "Mix" } }, { "pkt_conceal" }, {} },
            { 8, "BUFFERING.EXE", "", true, "buf_on",
              { { "buf_chance", "Chance" }, { "buf_slice", "Slice" }, { "buf_dropout", "Dropout" }, { "buf_mix", "Mix" } }, {}, {} },
            { 9, "CORRUPT.EXE", "", true, "corrupt_on",
              { { "corrupt_amount", "Amount" }, { "corrupt_bits", "Bits" }, { "corrupt_reverse", "Reverse" }, { "corrupt_mix", "Mix" } }, {}, {} },
            { 10, "LAG.EXE", "", true, "lag_on",
              { { "lag_chance", "Chance" }, { "lag_length", "Length" }, { "lag_depth", "Depth" }, { "lag_mix", "Mix" } }, {}, {} },
            { 11, "JITTER.EXE", "", true, "jit_on",
              { { "jit_wow", "Wow" }, { "jit_flutter", "Flutter" }, { "jit_drift", "Drift" }, { "jit_mix", "Mix" } }, {}, {} },
            { 12, "Demon Chorus", "possess", false, "chorus_on",
              { { "chorus_rate", "Rate" }, { "chorus_depth", "Depth" }, { "chorus_fb", "Feedback" }, { "chorus_mix", "Mix" } }, {}, {} },
            { 13, "Boiling Phaser", "boil", false, "phaser_on",
              { { "phaser_rate", "Rate" }, { "phaser_depth", "Depth" }, { "phaser_fb", "Feedback" }, { "phaser_mix", "Mix" } }, {}, {} },
            { 14, "Trem / Pan", "motion", false, "trem_on",
              { { "trem_rate", "Rate" }, { "trem_depth", "Depth" }, { "trem_shape", "Shape" }, { "trem_pan", "Pan" } }, {}, {} },
            { 15, "Hellfire Delay", "echo", false, "delay_on",
              { { "delay_time", "Time" }, { "delay_fb", "Feedback" }, { "delay_tone", "Tone" }, { "delay_mix", "Mix" } },
              { "delay_sync" }, { { "delay_pp", "Ping-pong" } } },
            { 16, "Abyss Verb", "void", false, "verb_on",
              { { "verb_size", "Size" }, { "verb_damp", "Damp" }, { "verb_width", "Width" }, { "verb_mix", "Mix" } }, {}, {} },
            { 17, "SPEAKER.EXE", "", true, "spk_on",
              { { "spk_drive", "Drive" }, { "spk_mix", "Mix" } }, { "spk_model" }, {} },
        };
    }
}

EvilFXEditor::EvilFXEditor (EvilFXProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (desktop);

    // ---- library + rack
    auto specs = moduleSpecs();
    for (int i = 0; i < (int) specs.size(); ++i)
    {
        auto* row = library.rows.add (new LibraryRow (specs[(size_t) i].number, specs[(size_t) i].title, specs[(size_t) i].retro));
        row->onClick = [this, i] { toggleInRack (i); };
        library.addAndMakeVisible (row);

        auto* card = cards.add (new ModuleCard (proc.apvts, specs[(size_t) i]));
        card->onRemove = [this, i] { toggleInRack (i); };
        rackArea.addChildComponent (card);
    }
    desktop.addAndMakeVisible (library);

    viewport.setViewedComponent (&rackArea, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    desktop.addAndMakeVisible (viewport);

    master = std::make_unique<ModuleCard> (proc.apvts, CardSpec { 0, "Master", "output", false, nullptr,
        { { "in_gain", "Input" }, { "out_gain", "Output" }, { "width", "Width" }, { "mix", "Mix" } },
        {}, { { "limiter", "Limiter" } } });
    desktop.addAndMakeVisible (*master);
    desktop.addAndMakeVisible (meter);
    meter.setInterceptsMouseClicks (false, false);

    // ---- visualizer (state lives on the session tree, off by default)
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

    // ---- presets
    for (int i = 0; i < (int) Presets::all().size(); ++i)
        presetBox.addItem (Presets::all()[(size_t) i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { Presets::apply (proc.apvts, presetBox.getSelectedId() - 1); refreshRack(); };
    presetBox.setTooltip ("Factory presets");
    auto step = [this] (int d)
    {
        const int n = presetBox.getNumItems();
        presetBox.setSelectedId ((presetBox.getSelectedId() - 1 + d + n) % n + 1);
    };
    prevBtn.onClick = [step] { step (-1); };
    nextBtn.onClick = [step] { step (1); };
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevBtn, &nextBtn }) desktop.addAndMakeVisible (c);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setResizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 14 / 10, baseH * 14 / 10);
    setSize (baseW * 85 / 100, baseH * 85 / 100);

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

    const int top = headerH + 4, bottom = baseH - statusH - 10;
    library.setBounds (margin, top, sidebarW, bottom - top);

    const int vx = margin + sidebarW + gap, vw = baseW - vx - margin;
    const int stripH = 150;
    viewport.setBounds (vx, top, vw, bottom - top - stripH - gap);
    master->setBounds (vx, bottom - stripH, 430, stripH);
    meter.setBounds (master->getOptionSpare().translated (master->getX(), master->getY()).reduced (4, 3));
    viz.setBounds (master->getRight() + gap, bottom - stripH, vw - 430 - gap, stripH);

    presetBox.setBounds (baseW / 2 - 150, 40, 300, 30);
    prevBtn.setBounds (presetBox.getX() - 38, 39, 32, 32);
    nextBtn.setBounds (presetBox.getRight() + 6, 39, 32, 32);

    auto right = juce::Rectangle<int> (margin, 6, baseW - 2 * margin, headerH - 10).removeFromRight (330).reduced (10, 14);
    counterArea = right.removeFromLeft (118);
    statusArea  = juce::Rectangle<int> (margin, baseH - statusH - 6, baseW - 2 * margin, statusH);
    marqueeArea = statusArea.reduced (3).withTrimmedRight (276);

    refreshRack();
}

//==============================================================================
void EvilFXEditor::toggleInRack (int module)
{
    const bool adding = ! Rack::contains (proc.apvts.state, module);
    Rack::set (proc.apvts.state, module, adding);
    if (auto* on = proc.apvts.getParameter (Rack::onId (module)))
        on->setValueNotifyingHost (adding ? 1.0f : 0.0f);
    refreshRack();

    if (adding)   // scroll the new card into view
    {
        auto* c = cards[module];
        const int viewTop = viewport.getViewPositionY(), viewH = viewport.getMaximumVisibleHeight();
        if (c->getBottom() > viewTop + viewH || c->getY() < viewTop)
            viewport.setViewPosition (0, juce::jmax (0, c->getBottom() - viewH));
    }
}

void EvilFXEditor::refreshRack()
{
    const auto rack = Rack::get (proc.apvts.state);
    lastRack = rack;

    const int areaW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int viewH = viewport.getHeight();
    const int cardW = (areaW - 2 * gap) / 3;
    const int cardH = (viewH - gap) / 2;

    int shown = 0;
    for (int i = 0; i < cards.size(); ++i)
    {
        const bool in = rack[i] == '1';
        library.rows[i]->setToggleState (in, juce::dontSendNotification);
        cards[i]->setVisible (in);
        if (in)
        {
            cards[i]->setBounds ((shown % 3) * (cardW + gap), (shown / 3) * (cardH + gap), cardW, cardH);
            ++shown;
        }
    }
    const int rows = (shown + 2) / 3;
    rackArea.empty = shown == 0;
    rackArea.setSize (areaW, juce::jmax (viewH, rows * (cardH + gap) - gap));
    rackArea.repaint();
}

void EvilFXEditor::timerCallback()
{
    // rack: pick up anything switched on by automation or a preset change from the host
    Rack::addSwitchedOn (proc.apvts);
    if (Rack::get (proc.apvts.state) != lastRack) refreshRack();

    const int preset = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != preset) presetBox.setSelectedId (preset, juce::dontSendNotification);

    meter.l = proc.meterL.load(); meter.r = proc.meterR.load();
    meter.repaint();

    const int active = proc.countActiveModules(), lat = proc.getLatencySamples();
    const float gr = std::round (proc.gainReductionDb.load() * 10.0f) / 10.0f;
    if (active != activeModules) { activeModules = active; desktop.repaint (counterArea); }
    if (lat != latency || gr != shownGr) { latency = lat; shownGr = gr; desktop.repaint (statusArea); }

    if (viz.isOn())
        viz.tick ([this] (float* d, int n) { return proc.pullVisualizerSamples (d, n); }, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0);

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
void EvilFXEditor::Desktop::paint (juce::Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();

    // blood-red sunset sky
    juce::ColourGradient sky (juce::Colour (0xff3d0006), 0, 0, juce::Colour (0xffffa184), 0, H * 0.78f, false);
    sky.addColour (0.42, juce::Colour (0xffc4141e));
    sky.addColour (0.72, juce::Colour (0xffff5a3c));
    g.setGradientFill (sky);
    g.fillAll();

    // evil sun + rays
    const juce::Point<float> sun (W * 0.84f, 10.0f);
    for (int i = 0; i < 7; ++i)
    {
        juce::Path ray;
        const float a = 1.75f + (float) i * 0.24f;
        ray.startNewSubPath (sun);
        ray.lineTo (sun.getPointOnCircumference (1100.0f, a - 0.05f));
        ray.lineTo (sun.getPointOnCircumference (1100.0f, a + 0.05f));
        ray.closeSubPath();
        g.setColour (juce::Colour (0xffffd0a0).withAlpha (0.07f));
        g.fillPath (ray);
    }
    juce::ColourGradient glow (juce::Colour (0xffffe0b0).withAlpha (0.95f), sun.x, sun.y,
                               juce::Colour (0xffff6a2a).withAlpha (0.0f), sun.x + 330.0f, sun.y, true);
    g.setGradientFill (glow);
    g.fillEllipse (sun.x - 330.0f, sun.y - 330.0f, 660.0f, 660.0f);

    // scorched hills
    juce::Path hill1, hill2;
    hill1.startNewSubPath (0, H * 0.72f);
    hill1.cubicTo (W * 0.25f, H * 0.6f, W * 0.45f, H * 0.82f, W * 0.7f, H * 0.7f);
    hill1.cubicTo (W * 0.85f, H * 0.63f, W * 0.95f, H * 0.66f, W, H * 0.68f);
    hill1.lineTo (W, H); hill1.lineTo (0, H); hill1.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8e1218), 0, H * 0.62f, juce::Colour (0xff2a0306), 0, H, false));
    g.fillPath (hill1);
    hill2.startNewSubPath (0, H * 0.86f);
    hill2.cubicTo (W * 0.3f, H * 0.78f, W * 0.6f, H * 0.92f, W, H * 0.8f);
    hill2.lineTo (W, H); hill2.lineTo (0, H); hill2.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5e080d), 0, H * 0.78f, juce::Colour (0xff120102), 0, H, false));
    g.fillPath (hill2);

    // a few static glass bubbles (same spots every time — no animation)
    juce::Random rnd (2003);
    for (int i = 0; i < 16; ++i)
    {
        const float x = rnd.nextFloat() * W, y = rnd.nextFloat() * H, r = 4.0f + rnd.nextFloat() * 14.0f;
        juce::ColourGradient fill (juce::Colours::white.withAlpha (0.04f), x, y, juce::Colours::white.withAlpha (0.3f), x + r, y, true);
        g.setGradientFill (fill);
        g.fillEllipse (x - r, y - r, r * 2, r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawEllipse (x - r, y - r, r * 2, r * 2, 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.5f, r * 0.32f);
    }

    // header glass
    auto header = juce::Rectangle<float> ((float) margin, 6.0f, W - 2.0f * margin, (float) headerH - 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.62f), 0, header.getY(),
                                             juce::Colour (0xffffd6d0).withAlpha (0.38f), 0, header.getBottom(), false));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillRoundedRectangle (header.withHeight (header.getHeight() * 0.45f).reduced (6.0f, 2.0f), 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (header, 18.0f, 1.2f);

    owner.drawLogo (g, header.withWidth (380.0f));

    g.setColour (col::ink.withAlpha (0.7f));
    g.setFont (sans (10.5f, true));
    g.drawText ("P R E S E T", juce::Rectangle<int> (getWidth() / 2 - 150, 22, 300, 14), juce::Justification::centred);

    // retro corner: LED counter + 88x31 buttons
    auto counter = owner.counterArea;
    g.setColour (juce::Colours::black);
    g.fillRect (counter);
    bevel (g, counter, false);
    g.setFont (mono (9.5f, true));
    g.setColour (juce::Colour (0xffff9c9c));
    g.drawText ("FX ONLINE", counter.removeFromTop (18), juce::Justification::centred);
    g.setFont (mono (24.0f, true));
    g.setColour (juce::Colour (0xffff2a2a));
    g.drawText (juce::String (juce::jmax (0, owner.activeModules)).paddedLeft ('0', 2) + "/" + juce::String (Rack::numModules), counter.withTrimmedBottom (2), juce::Justification::centred);

    const int bx = owner.counterArea.getRight() + 12, by = owner.counterArea.getY();
    const int bw = 88, bh = (owner.counterArea.getHeight() - 4) / 2;
    owner.drawBadge (g, { bx,      by,          bw, bh }, "XAINJAYA", ".COM",     col::w98Maroon, juce::Colours::white);
    owner.drawBadge (g, { bx + 92, by,          bw, bh }, "EVIL",     "INSIDE",   juce::Colour (0xff111111), juce::Colour (0xffff3030));
    owner.drawBadge (g, { bx,      by + bh + 4, bw, bh }, "OPEN",     "SOURCE",   juce::Colour (0xff111111), col::ember);
    owner.drawBadge (g, { bx + 92, by + bh + 4, bw, bh }, "56K",      "FRIENDLY", col::w98Brick, juce::Colours::white);

    // Win9x status bar
    auto status = owner.statusArea;
    g.setColour (col::w98Grey);
    g.fillRect (status);
    bevel (g, status, true);
    auto inner = status.reduced (3);
    auto grField = inner.removeFromRight (120); inner.removeFromRight (3);
    auto latField = inner.removeFromRight (150); inner.removeFromRight (3);
    for (auto f : { grField, latField, inner }) bevel (g, f, false);

    g.setFont (mono (11.5f));
    g.setColour (juce::Colours::black);
    g.drawText ("LATENCY: " + juce::String (juce::jmax (0, owner.latency)) + " smp", latField.reduced (6, 0), juce::Justification::centredLeft);
    g.drawText ("LIMIT: " + juce::String (juce::jmin (0.0f, owner.shownGr), 1) + " dB", grField.reduced (6, 0), juce::Justification::centredLeft);

    const juce::String text (kMarquee);
    const float tw = juce::GlyphArrangement::getStringWidth (mono (11.5f), text);
    if (owner.marqueeX < -tw) owner.marqueeX += tw;
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (inner.reduced (2));
        for (float x = (float) inner.getX() + owner.marqueeX; x < (float) inner.getRight(); x += tw)
            g.drawText (text, juce::Rectangle<float> (x, (float) inner.getY(), tw + 4.0f, (float) inner.getHeight()), juce::Justification::centredLeft, false);
    }
}

void EvilFXEditor::drawLogo (juce::Graphics& g, juce::Rectangle<float> area)
{
    // evil smiley orb with horns
    const juce::Point<float> c (area.getX() + 46.0f, area.getCentreY() + 3.0f);
    const float r = 28.0f;
    juce::Path horns;
    horns.addTriangle (c.x - 21.0f, c.y - 16.0f, c.x - 9.0f, c.y - 25.0f, c.x - 25.0f, c.y - 36.0f);
    horns.addTriangle (c.x + 21.0f, c.y - 16.0f, c.x + 9.0f, c.y - 25.0f, c.x + 25.0f, c.y - 36.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a0208), c.x, c.y - 36.0f, col::red, c.x, c.y - 16.0f, false));
    g.fillPath (horns);

    g.setColour (col::ember.withAlpha (0.35f));
    g.fillEllipse (c.x - r - 4, c.y - r - 4, (r + 4) * 2, (r + 4) * 2);
    drawOrb (g, c, r, col::redPale, juce::Colour (0xffff3644), col::redDeep);

    g.setColour (col::ink);
    juce::Path brows;   // angry eyebrows
    brows.startNewSubPath (c.x - 14.0f, c.y - 12.0f); brows.lineTo (c.x - 4.0f, c.y - 7.0f);
    brows.startNewSubPath (c.x + 14.0f, c.y - 12.0f); brows.lineTo (c.x + 4.0f, c.y - 7.0f);
    g.strokePath (brows, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.fillEllipse (c.x - 11.0f, c.y - 6.0f, 6.0f, 8.0f);
    g.fillEllipse (c.x + 5.0f, c.y - 6.0f, 6.0f, 8.0f);
    juce::Path grin;
    grin.addCentredArc (c.x, c.y + 1.0f, 15.0f, 13.0f, 0.0f, 2.0f, 4.28f, true);
    grin.closeSubPath();
    g.fillPath (grin);
    g.setColour (juce::Colours::white);   // fangs
    juce::Path fangs;
    fangs.addTriangle (c.x - 9.0f, c.y + 8.0f, c.x - 4.0f, c.y + 8.0f, c.x - 6.5f, c.y + 13.0f);
    fangs.addTriangle (c.x + 9.0f, c.y + 8.0f, c.x + 4.0f, c.y + 8.0f, c.x + 6.5f, c.y + 13.0f);
    g.fillPath (fangs);
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.5f), c.x, c.y - r * 0.92f, juce::Colours::white.withAlpha (0.0f), c.x, c.y - r * 0.1f, false);
    g.setGradientFill (spec);
    g.fillEllipse (c.x - r * 0.64f, c.y - r * 0.9f, r * 1.28f, r * 0.8f);

    // wordmark
    const float tx = c.x + r + 16.0f;
    juce::GlyphArrangement ga;
    ga.addLineOfText (sans (42.0f, true), "EVIL", tx, area.getCentreY() + 4.0f);
    juce::Path evil; ga.createPath (evil);
    const auto eb = evil.getBounds();
    juce::GlyphArrangement gb;
    gb.addLineOfText (sans (42.0f), "FX", eb.getRight() + 8.0f, area.getCentreY() + 4.0f);
    juce::Path fx; gb.createPath (fx);
    const auto fb = fx.getBounds();

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillPath (evil, juce::AffineTransform::translation (0.0f, 2.0f));
    g.fillPath (fx, juce::AffineTransform::translation (0.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a0208), 0, eb.getY(), col::red, 0, eb.getBottom(), false));
    g.fillPath (evil);
    g.setGradientFill (juce::ColourGradient (col::ember, 0, fb.getY(), juce::Colour (0xffc0101c), 0, fb.getBottom(), false));
    g.fillPath (fx);

    g.setFont (sans (11.5f, true));
    g.setColour (col::ink.withAlpha (0.75f));
    g.drawText ("open-source fx  -  by xain jaya", juce::Rectangle<float> (tx, area.getCentreY() + 10.0f, 300.0f, 16.0f), juce::Justification::centredLeft);
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
EvilFXEditor::LibraryRow::LibraryRow (int n, juce::String name, bool isRetro)
    : juce::Button (name), number (n), retro (isRetro)
{
    setTooltip ("Add / remove " + name);
}

void EvilFXEditor::LibraryRow::paintButton (juce::Graphics& g, bool hover, bool down)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (1.0f, 1.0f);
    const float corner = r.getHeight() * 0.5f;

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff6060), 0, r.getY(), juce::Colour (0xffb00c18), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (corner * 0.6f, 1.0f), corner * 0.4f);
    }
    else if (hover || down)
    {
        g.setColour (juce::Colours::white.withAlpha (down ? 0.7f : 0.45f));
        g.fillRoundedRectangle (r, corner);
    }

    // number badge
    auto badge = juce::Rectangle<float> (r.getX() + 3.0f, r.getCentreY() - 8.0f, 16.0f, 16.0f);
    g.setColour (on ? juce::Colours::white : col::ink.withAlpha (0.12f));
    g.fillEllipse (badge);
    g.setColour (on ? col::redDeep : col::ink);
    g.setFont (sans (10.0f, true));
    g.drawText (juce::String (number), badge, juce::Justification::centred);

    // name
    g.setColour (on ? juce::Colours::white : col::ink.withAlpha (0.9f));
    g.setFont (retro ? mono (12.0f, true) : sans (13.0f, true));
    g.drawText (getName(), r.withTrimmedLeft (26.0f).withTrimmedRight (60.0f), juce::Justification::centredLeft);

    // add / added indicator
    auto ind = juce::Rectangle<float> (r.getRight() - 20.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
    g.setColour (on ? juce::Colours::white : col::ink.withAlpha (0.55f));
    juce::Path p;
    if (on) { p.startNewSubPath (ind.getX(), ind.getCentreY()); p.lineTo (ind.getX() + 4.5f, ind.getBottom() - 1.5f); p.lineTo (ind.getRight(), ind.getY() + 1.0f); }
    else    { p.startNewSubPath (ind.getCentreX(), ind.getY()); p.lineTo (ind.getCentreX(), ind.getBottom());
              p.startNewSubPath (ind.getX(), ind.getCentreY()); p.lineTo (ind.getRight(), ind.getCentreY()); }
    g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Web 1.0 tag
    if (retro)
    {
        auto tag = juce::Rectangle<int> ((int) r.getRight() - 64, (int) r.getCentreY() - 7, 38, 14);
        g.setColour (col::w98Grey);
        g.fillRect (tag);
        bevel (g, tag, true);
        g.setColour (juce::Colours::black);
        g.setFont (mono (8.5f, true));
        g.drawText ("WEB1.0", tag, juce::Justification::centred);
    }
}

void EvilFXEditor::Library::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (3.0f);
    constexpr float corner = 14.0f;
    juce::Path shape; shape.addRoundedRectangle (r, corner);
    juce::DropShadow (juce::Colour (0x55400008), 12, { 0, 4 }).drawForPath (g, shape);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.74f), 0, r.getY(),
                                             juce::Colour (0xffffd8d6).withAlpha (0.58f), 0, r.getBottom(), false));
    g.fillPath (shape);

    auto bar = r.withHeight ((float) libHeaderH - 4.0f);
    juce::Path barPath;
    barPath.addRoundedRectangle (bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), corner, corner, true, true, false, false);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a0408), 0, bar.getY(), juce::Colour (0xff8a0a14), 0, bar.getBottom(), false));
    g.fillPath (barPath);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.4f), 0, bar.getY(), juce::Colours::white.withAlpha (0.04f), 0, bar.getCentreY(), false));
    g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * 0.5f).reduced (3.0f, 1.0f), corner - 3.0f);

    g.setColour (juce::Colours::white);
    g.setFont (sans (15.0f, true));
    g.drawText ("FX Library", bar.reduced (14.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (sans (11.0f));
    g.drawText ("click to add / remove", bar.reduced (14.0f, 0.0f), juce::Justification::centredRight);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (shape, juce::PathStrokeType (1.3f));
}

void EvilFXEditor::Library::resized()
{
    auto r = getLocalBounds().reduced (10, 0).withTrimmedTop (libHeaderH + 2);
    for (auto* row : rows) row->setBounds (r.removeFromTop (libRowH));
}

void EvilFXEditor::RackArea::paint (juce::Graphics& g)
{
    if (! empty) return;
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (440.0f, 150.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), 0, r.getY(),
                                             juce::Colour (0xffffd8d6).withAlpha (0.35f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (r, 18.0f, 1.2f);
    g.setColour (col::ink);
    g.setFont (sans (24.0f, true));
    g.drawText ("Your rack is empty", r.withTrimmedBottom (50.0f), juce::Justification::centred);
    g.setFont (sans (14.0f));
    g.setColour (col::ink.withAlpha (0.75f));
    g.drawText ("Pick some evil from the FX Library on the left", r.withTrimmedTop (60.0f).withTrimmedBottom (30.0f), juce::Justification::centred);
    g.drawText ("or load a preset up top.", r.withTrimmedTop (86.0f).withTrimmedBottom (8.0f), juce::Justification::centred);
}

void EvilFXEditor::Meter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float barH = (area.getHeight() - 3.0f) * 0.5f;
    auto drawBar = [&] (juce::Rectangle<float> b, float level)
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
