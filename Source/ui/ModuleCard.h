#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "EvilLookAndFeel.h"

namespace aero
{
struct CardSpec
{
    int number = 0;                     // position in the signal chain (0 = master, no badge)
    juce::String title, tagline;
    bool retro = false;
    const char* powerId = nullptr;      // nullptr = always on (master)
    std::vector<std::pair<const char*, juce::String>> knobs;
    std::vector<const char*> combos;    // choice parameters shown as drop-downs
    std::vector<std::pair<const char*, juce::String>> toggles;
};

// "Remove from rack" button: Win9x close box on retro cards, a little glass orb on the others
class RemoveButton : public juce::Button
{
public:
    explicit RemoveButton (bool isRetro) : juce::Button ("remove"), retro (isRetro) { setTooltip ("Remove from rack"); }

    void paintButton (juce::Graphics& g, bool hover, bool down) override
    {
        auto r = getLocalBounds();
        if (retro)
        {
            auto box = r.withSizeKeepingCentre (16, 14);
            g.setColour (col::w98Grey);
            g.fillRect (box);
            bevel (g, box, ! down);
            drawX (g, box.toFloat().reduced (4.0f, 3.5f).translated (down ? 1.0f : 0.0f, down ? 1.0f : 0.0f), juce::Colours::black, 1.6f);
            return;
        }
        const auto c = r.toFloat().getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f - 3.0f;
        drawOrb (g, c, rad, juce::Colour (0xffffc0c0), hover ? col::red : juce::Colour (0xffb02030), col::redDeep.darker (0.4f));
        drawX (g, juce::Rectangle<float> (rad * 0.9f, rad * 0.9f).withCentre (c), juce::Colours::white, 1.8f);
    }

private:
    static void drawX (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float thick)
    {
        g.setColour (c);
        g.drawLine (r.getX(), r.getY(), r.getRight(), r.getBottom(), thick);
        g.drawLine (r.getRight(), r.getY(), r.getX(), r.getBottom(), thick);
    }
    bool retro;
};

// One effect module: title bar with power switch, an optional row of
// combo/toggles, and a row of knobs. Aero cards are glass, retro cards are Win9x windows.
class ModuleCard : public juce::Component
{
public:
    static constexpr int titleH = 30, optionH = 26;

    std::function<void()> onRemove;   // called when the X is clicked

    ModuleCard (juce::AudioProcessorValueTreeState& state, CardSpec s) : spec (std::move (s)), removeBtn (spec.retro)
    {
        using APVTS = juce::AudioProcessorValueTreeState;
        if (spec.powerId != nullptr)
        {
            addAndMakeVisible (removeBtn);
            removeBtn.onClick = [this] { if (onRemove) onRemove(); };
            power.getProperties().set ("power", true);
            power.getProperties().set ("retro", spec.retro);
            power.setButtonText (spec.retro ? "ON" : "");
            power.setTooltip ("Turn " + spec.title + " on/off");
            addAndMakeVisible (power);
            powerAtt = std::make_unique<APVTS::ButtonAttachment> (state, spec.powerId, power);
            power.onStateChange = [this] { refreshEnabledLook(); };
        }

        for (auto& [id, label] : spec.knobs)
        {
            auto* sl = sliders.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
            sl->getProperties().set ("retro", spec.retro);
            sl->setName (id);
            sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
            sl->setPopupDisplayEnabled (false, false, nullptr);
            sl->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            sl->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            if (auto* p = state.getParameter (id))
            {
                sl->setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
                sl->setTooltip (p->getName (64) + "  (double-click to reset)");
            }
            addAndMakeVisible (sl);
            sliderAtts.add (new APVTS::SliderAttachment (state, id, *sl));
            labels.add (label);
        }

        for (auto* id : spec.combos)
        {
            auto* cb = combos.add (new juce::ComboBox());
            cb->getProperties().set ("retro", spec.retro);
            if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
            {
                cb->addItemList (p->choices, 1);
                cb->setTooltip (p->getName (64));
            }
            addAndMakeVisible (cb);
            comboAtts.add (new APVTS::ComboBoxAttachment (state, id, *cb));
        }

        for (auto& [id, label] : spec.toggles)
        {
            auto* t = toggles.add (new juce::ToggleButton (label));
            t->getProperties().set ("retro", spec.retro);
            addAndMakeVisible (t);
            toggleAtts.add (new APVTS::ButtonAttachment (state, id, *t));
        }

        setBufferedToImage (true);
        refreshEnabledLook();
    }

    // brief glow so you can spot the card you just jumped to
    void flash() { flashAmt = 1.0f; repaint(); }
    bool tickFlash()
    {
        if (flashAmt <= 0.0f) return false;
        flashAmt = juce::jmax (0.0f, flashAmt - 0.06f);
        repaint();
        return true;
    }

    juce::Slider* getSlider (const juce::String& id)
    {
        for (auto* s : sliders) if (s->getName() == id) return s;
        return nullptr;
    }

    // space in the option row left free for extra widgets (e.g. the master meter)
    juce::Rectangle<int> getOptionSpare() const { return optionSpare; }
    bool isRetro() const { return spec.retro; }

    void paint (juce::Graphics& g) override
    {
        spec.retro ? paintRetro (g) : paintAero (g);

        if (flashAmt > 0.0f)
        {
            const auto fr = getLocalBounds().toFloat().reduced (2.0f);
            g.setColour (juce::Colour (0xffffe08a).withAlpha (0.9f * flashAmt));
            if (spec.retro) g.drawRect (fr, 3.0f);
            else            g.drawRoundedRectangle (fr, 15.0f, 3.0f);
        }

        // knob captions
        g.setFont (spec.retro ? mono (11.0f, true) : sans (11.5f, true));
        g.setColour (spec.retro ? juce::Colours::black : col::ink.withAlpha (0.85f));
        for (int i = 0; i < sliders.size(); ++i)
            g.drawText (labels[i], captionBounds[(size_t) i], juce::Justification::centred);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (spec.retro ? 4 : 6);
        auto title = r.removeFromTop (titleH - (spec.retro ? 4 : 0));
        if (spec.powerId != nullptr)
        {
            removeBtn.setBounds (spec.retro ? title.removeFromRight (24).reduced (0, 2) : title.removeFromRight (24).reduced (0, 1));
            title.removeFromRight (spec.retro ? 2 : 0);
            power.setBounds (spec.retro ? title.removeFromRight (52).reduced (2, 3) : title.removeFromRight (30).reduced (2));
        }

        r.removeFromTop (4);
        const bool hasOptions = ! spec.combos.empty() || ! spec.toggles.empty() || spec.number == 0;
        auto opt = hasOptions ? r.removeFromTop (optionH).reduced (6, 2) : juce::Rectangle<int>();
        const int comboW = combos.size() > 1 ? juce::jmin (104, (opt.getWidth() - 6 * (combos.size() - 1)) / combos.size()) : 96;
        for (auto* cb : combos) { cb->setBounds (opt.removeFromLeft (comboW)); opt.removeFromLeft (6); }
        for (auto* t : toggles)
        {
            const int w = spec.retro ? 130 : juce::roundToInt (juce::GlyphArrangement::getStringWidth (sans (12.0f, true), t->getButtonText())) + 46;
            t->setBounds (opt.removeFromLeft (w)); opt.removeFromLeft (4);
        }
        optionSpare = opt;

        r = r.reduced (4, 2);
        const int n = juce::jmax (1, sliders.size());
        const int colW = r.getWidth() / 4;   // same knob size on every card; 3-knob cards are centred
        const int knob = juce::jmin (colW - 4, r.getHeight() - 31, 66);
        auto knobRow = r.withSizeKeepingCentre (colW * n, knob + 31);
        captionBounds.resize ((size_t) sliders.size());
        for (int i = 0; i < sliders.size(); ++i)
        {
            auto col = knobRow.removeFromLeft (colW);
            captionBounds[(size_t) i] = col.removeFromTop (15);
            sliders[i]->setBounds (col);
        }
    }

private:
    void refreshEnabledLook()
    {
        const bool on = spec.powerId == nullptr || power.getToggleState();
        if (on == lastOn) return;
        lastOn = on;
        for (auto* s : sliders) s->setAlpha (on ? 1.0f : 0.42f);
        for (auto* cb : combos) cb->setAlpha (on ? 1.0f : 0.5f);
        for (auto* t : toggles) t->setAlpha (on ? 1.0f : 0.5f);
        repaint();
    }

    void paintAero (juce::Graphics& g)
    {
        const auto r = getLocalBounds().toFloat().reduced (4.0f);
        constexpr float corner = 14.0f;
        juce::Path shape; shape.addRoundedRectangle (r, corner);

        juce::DropShadow (juce::Colour (0x55400008), 12, { 0, 4 }).drawForPath (g, shape);

        const bool on = lastOn;
        juce::ColourGradient glass (juce::Colours::white.withAlpha (on ? 0.72f : 0.5f), 0, r.getY(),
                                    juce::Colour (0xffffd8d6).withAlpha (on ? 0.55f : 0.35f), 0, r.getBottom(), false);
        g.setGradientFill (glass);
        g.fillPath (shape);

        // title bar
        auto bar = r.withHeight ((float) titleH + 2.0f);
        juce::Path barPath; barPath.addRoundedRectangle (bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), corner, corner, true, true, false, false);
        const auto top = spec.number == 0 ? juce::Colour (0xffffb36a) : juce::Colour (0xffff7a7a);
        const auto bot = spec.number == 0 ? col::emberDeep : juce::Colour (0xffc0101c);
        juce::ColourGradient barGrad (top.withAlpha (on ? 0.95f : 0.55f), 0, bar.getY(), bot.withAlpha (on ? 0.95f : 0.55f), 0, bar.getBottom(), false);
        g.setGradientFill (barGrad);
        g.fillPath (barPath);
        juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.65f), 0, bar.getY(), juce::Colours::white.withAlpha (0.08f), 0, bar.getCentreY(), false);
        g.setGradientFill (gloss);
        g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * 0.5f).reduced (3.0f, 1.0f), corner - 3.0f);

        float textX = r.getX() + 12.0f;
        if (spec.number > 0)
        {
            const juce::Point<float> c (r.getX() + 20.0f, bar.getCentreY());
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f);
            g.setColour (col::redDeep);
            g.setFont (sans (11.5f, true));
            g.drawText (juce::String (spec.number), juce::Rectangle<float> (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f), juce::Justification::centred);
            textX = c.x + 15.0f;
        }
        g.setFont (sans (15.5f, true));
        const auto titleArea = juce::Rectangle<float> (textX, bar.getY(), r.getRight() - textX - 60.0f, bar.getHeight());
        g.setColour (col::ink.withAlpha (0.35f));
        g.drawText (spec.title, titleArea.translated (0.0f, 1.0f), juce::Justification::centredLeft);
        g.setColour (juce::Colours::white);
        g.drawText (spec.title, titleArea, juce::Justification::centredLeft);
        // the little tagline only shows when it fits beside the title
        const float titleW = juce::GlyphArrangement::getStringWidth (sans (15.5f, true), spec.title);
        const float tagW = juce::GlyphArrangement::getStringWidth (sans (11.0f), spec.tagline);
        if (titleW + tagW + 10.0f < titleArea.getWidth())
        {
            g.setFont (sans (11.0f));
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawText (spec.tagline, titleArea, juce::Justification::centredRight);
        }

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (shape, juce::PathStrokeType (1.3f));
        g.setColour (col::redDeep.withAlpha (0.25f));
        g.drawRoundedRectangle (r.expanded (0.8f), corner + 0.8f, 0.8f);
    }

    void paintRetro (juce::Graphics& g)
    {
        auto r = getLocalBounds().reduced (3);
        g.setColour (juce::Colour (0x30000000));
        g.fillRect (r.translated (3, 3));
        g.setColour (col::w98Grey);
        g.fillRect (r);
        bevel (g, r, true);

        auto bar = r.reduced (3).withHeight (titleH - 6);
        const bool on = lastOn;
        juce::ColourGradient grad (on ? col::w98Maroon : col::w98Dark, (float) bar.getX(), 0,
                                   on ? col::w98Red : juce::Colour (0xffb5b5b5), (float) bar.getRight(), 0, false);
        g.setGradientFill (grad);
        g.fillRect (bar);

        // tiny "program icon"
        auto icon = bar.removeFromLeft (bar.getHeight()).reduced (4);
        g.setColour (juce::Colours::white);
        g.fillRect (icon);
        g.setColour (col::w98Brick);
        g.fillRect (icon.reduced (2));
        g.setColour (juce::Colours::white);
        g.setFont (mono (9.0f, true));
        g.drawText (juce::String (spec.number), icon, juce::Justification::centred);

        g.setFont (mono (12.5f, true));
        g.setColour (juce::Colours::white);
        g.drawText (spec.title, bar.withTrimmedLeft (2).withTrimmedRight (82), juce::Justification::centredLeft);

        // sunken panel behind knobs
        auto panel = r.reduced (6).withTrimmedTop (titleH - 2);
        bevel (g, panel, false);
    }

    CardSpec spec;
    RemoveButton removeBtn;
    juce::ToggleButton power;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::ToggleButton> toggles;
    juce::StringArray labels;
    std::vector<juce::Rectangle<int>> captionBounds;
    juce::Rectangle<int> optionSpare;
    bool lastOn = true;
    float flashAmt = 0.0f;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAtts;

    JUCE_DECLARE_NON_COPYABLE (ModuleCard)
};
} // namespace aero
