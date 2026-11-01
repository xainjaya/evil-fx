#pragma once
#include "PluginProcessor.h"
#include "Rack.h"
#include "ui/EvilLookAndFeel.h"
#include "ui/ModuleCard.h"
#include "ui/Visualizer.h"

// Layout: header on top, the FX library on the left, the rack on the right showing
// only the effects you picked (scrolls if you pick lots), and master + visualizer
// along the bottom.
// Everything is laid out at a fixed base size inside `Desktop`, which is scaled
// to the window size (aspect-locked) so it never reflows into a mess.
class EvilFXEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseW = 1160, baseH = 680;

    explicit EvilFXEditor (EvilFXProcessor&);
    ~EvilFXEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    void timerCallback() override;

    //--------------------------------------------------------------------------
    class Desktop : public juce::Component
    {
    public:
        explicit Desktop (EvilFXEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        EvilFXEditor& owner;
    };

    class Meter : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        float l = 0.0f, r = 0.0f;
    };

    // one row in the library list
    class LibraryRow : public juce::Button
    {
    public:
        LibraryRow (int number, juce::String name, bool retro);
        void paintButton (juce::Graphics&, bool hover, bool down) override;
    private:
        int number; bool retro;
    };

    class Library : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        void resized() override;
        juce::OwnedArray<LibraryRow> rows;
    };

    class RackArea : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        bool empty = true;
    };

    //--------------------------------------------------------------------------
    void toggleInRack (int module);
    void refreshRack();
    void drawLogo (juce::Graphics&, juce::Rectangle<float>);
    void drawBadge (juce::Graphics&, juce::Rectangle<int>, const juce::String& top, const juce::String& bottom, juce::Colour, juce::Colour);

    EvilFXProcessor& proc;
    aero::LookAndFeel lnf;
    Desktop desktop { *this };
    Library library;
    juce::Viewport viewport;
    RackArea rackArea;
    juce::OwnedArray<aero::ModuleCard> cards;   // every effect, in chain order
    std::unique_ptr<aero::ModuleCard> master;
    Meter meter;
    aero::Visualizer viz;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::TooltipWindow tooltips { this, 600 };

    juce::Rectangle<int> counterArea, statusArea, marqueeArea;
    juce::String lastRack;
    float marqueeX = 0.0f;
    int activeModules = -1, latency = -1;
    float shownGr = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EvilFXEditor)
};
