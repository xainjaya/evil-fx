#pragma once
#include "PluginProcessor.h"
#include "Rack.h"
#include "ui/EvilLookAndFeel.h"
#include "ui/ModuleCard.h"
#include "ui/Visualizer.h"

// Layout: header (logo, presets, save/random), a toolbar (+ ADD FX menu, clear,
// theme, help), the rack showing only the effects you added (5 across, scrolls),
// and master + visualizer along the bottom.
// Everything is laid out at a fixed base size inside `Desktop`, which is scaled
// to the window size (aspect-locked) so it never reflows into a mess.
class EvilFXEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseW = 1280, baseH = 780;
    static const juce::StringArray& themeNames();

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
        juce::Image background;   // the scenery is rendered once per theme and reused
    };

    class Meter : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        float l = 0.0f, r = 0.0f;
    };

    class RackArea : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        bool empty = true;
    };

    class SiteLink : public juce::Button
    {
    public:
        SiteLink() : juce::Button ("xainjaya.com") {}
        void paintButton (juce::Graphics&, bool hover, bool down) override;
    };

    class MoreHint : public juce::Button
    {
    public:
        MoreHint() : juce::Button ("more") {}
        void paintButton (juce::Graphics&, bool hover, bool down) override;
        int count = 0;
    };

    //--------------------------------------------------------------------------
    void showAddMenu();
    void focusModule (int module);
    void removeModule (int module);
    void clearRack();
    void refreshRack();
    void updateMoreHint();
    void rebuildPresetMenu();
    void syncPresetBox();
    void savePresetDialog();
    void showHelp();
    void showNote (const juce::String&);
    void renderBackground();
    void drawLogo (juce::Graphics&, juce::Rectangle<float>);

    EvilFXProcessor& proc;
    aero::LookAndFeel lnf;
    Desktop desktop { *this };
    juce::Viewport viewport;
    RackArea rackArea;
    MoreHint moreHint;
    SiteLink siteBtn;
    juce::OwnedArray<aero::ModuleCard> cards;   // every effect, in chain order
    std::vector<aero::CardSpec> specs;
    std::unique_ptr<aero::ModuleCard> master;
    Meter meter;
    aero::Visualizer viz;
    juce::ComboBox presetBox, themeBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, randomBtn { "RANDOM" },
                     addBtn { "+ ADD FX" }, clearBtn { "CLEAR" }, helpBtn { "?" };
    juce::TooltipWindow tooltips { this, 600 };
    juce::Array<juce::File> userFiles;

    juce::Rectangle<int> toolbarArea, statusArea, marqueeArea, bpmField, latField, grField;
    juce::String lastRack, note;
    int noteTicks = 0, inRack = 0;
    float marqueeX = 0.0f;
    int activeModules = -1, latency = -1;
    float shownGr = 1.0f, shownBpm = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EvilFXEditor)
};
