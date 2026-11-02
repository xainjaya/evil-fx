#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "EvilLookAndFeel.h"
#include "../Presets.h"

namespace aero
{
// Overlay preset browser: type to search, filter by category or favourites,
// click the star to favourite, click a preset to hear it, double-click / Enter to
// load and close. Esc closes.
class PresetBrowser : public juce::Component,
                      private juce::ListBoxModel,
                      private juce::TextEditor::Listener
{
public:
    std::function<void (int factoryIndex)> onLoadFactory;
    std::function<void (const juce::File&)> onLoadUser;
    std::function<void()> onClose;
    std::function<juce::String()> currentName;

    PresetBrowser()
    {
        search.setTextToShowWhenEmpty ("Search presets...  (try \"pad\", \"robot\", \"tempo\")", col::ink.withAlpha (0.45f));
        search.setFont (sans (15.0f));
        search.setColour (juce::TextEditor::backgroundColourId, juce::Colours::white.withAlpha (0.85f));
        search.setColour (juce::TextEditor::textColourId, col::ink);
        search.setColour (juce::TextEditor::outlineColourId, col::redDeep.withAlpha (0.35f));
        search.setColour (juce::TextEditor::focusedOutlineColourId, col::red);
        search.addListener (this);
        search.setEscapeAndReturnKeysConsumed (false);
        addAndMakeVisible (search);

        filter.setTooltip ("Show one category");
        filter.onChange = [this] { refilter(); };
        addAndMakeVisible (filter);

        list.setModel (this);
        list.setRowHeight (28);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::white.withAlpha (0.55f));
        list.setColour (juce::ListBox::outlineColourId, col::redDeep.withAlpha (0.3f));
        list.setOutlineThickness (1);
        addAndMakeVisible (list);

        closeBtn.setTooltip ("Close (Esc)");
        closeBtn.onClick = [this] { if (onClose) onClose(); };
        addAndMakeVisible (closeBtn);
        setWantsKeyboardFocus (true);
    }

    // rebuild the entry list (call when opening, or after saving a preset)
    void refresh()
    {
        entries.clear();
        const auto& all = Presets::all();
        juce::StringArray cats;
        for (int i = 0; i < (int) all.size(); ++i)
        {
            entries.push_back ({ all[(size_t) i].name, all[(size_t) i].category, i, {}, Presets::favKeyFactory (all[(size_t) i].name) });
            cats.addIfNotAlreadyThere (all[(size_t) i].category);
        }
        for (auto& f : Presets::userPresets())
            entries.push_back ({ f.getFileNameWithoutExtension(), "Your Presets", -1, f, Presets::favKeyUser (f.getFileNameWithoutExtension()) });
        favs = Presets::loadFavourites();

        const auto keep = filter.getText();
        filter.clear (juce::dontSendNotification);
        filter.addItem ("All presets", 1);
        filter.addItem ("Favourites", 2);
        filter.addSeparator();
        int id = 3;
        for (auto& c : cats) filter.addItem (c, id++);
        filter.addItem ("Your Presets", id);
        filter.setText (keep.isNotEmpty() ? keep : "All presets", juce::dontSendNotification);
        if (filter.getSelectedId() == 0) filter.setSelectedId (1, juce::dontSendNotification);
        refilter();
    }

    void focusSearch() { search.grabKeyboardFocus(); search.selectAll(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.35f));   // dim the rack behind
        const auto r = panel.toFloat();
        juce::Path shape; shape.addRoundedRectangle (r, 18.0f);
        juce::DropShadow (juce::Colour (0x88400008), 24, { 0, 8 }).drawForPath (g, shape);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.94f), 0, r.getY(),
                                                 juce::Colour (0xffffe0dc).withAlpha (0.94f), 0, r.getBottom(), false));
        g.fillPath (shape);
        auto bar = r.withHeight (44.0f);
        juce::Path barPath; barPath.addRoundedRectangle (bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), 18.0f, 18.0f, true, true, false, false);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7a7a), 0, bar.getY(), juce::Colour (0xffc0101c), 0, bar.getBottom(), false));
        g.fillPath (barPath);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * 0.48f).reduced (6.0f, 2.0f), 12.0f);
        g.setColour (juce::Colours::white);
        g.setFont (sans (18.0f, true));
        g.drawText ("PRESETS.EXE", bar.reduced (18.0f, 0.0f), juce::Justification::centredLeft);
        g.setFont (sans (12.5f));
        g.drawText (juce::String (shown.size()) + " of " + juce::String ((int) entries.size()) + " shown",
                    bar.reduced (60.0f, 0.0f), juce::Justification::centredRight);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (shape, juce::PathStrokeType (1.3f));

        g.setColour (col::ink.withAlpha (0.8f));
        g.setFont (sans (11.5f));
        g.drawText ("Click to hear it  -  double-click or Enter to keep it  -  click the star to favourite  -  Esc to close",
                    panel.withTop (panel.getBottom() - 30).reduced (18, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        panel = getLocalBounds().withSizeKeepingCentre (juce::jmin (620, getWidth() - 40), juce::jmin (560, getHeight() - 30));
        auto r = panel.reduced (18, 0);
        closeBtn.setBounds (juce::Rectangle<int> (panel.getRight() - 46, panel.getY() + 7, 32, 30));
        r.removeFromTop (56);
        auto row = r.removeFromTop (34);
        filter.setBounds (row.removeFromRight (170));
        row.removeFromRight (8);
        search.setBounds (row);
        r.removeFromTop (10);
        r.removeFromBottom (32);
        list.setBounds (r);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        if (k == juce::KeyPress::returnKey) { loadRow (list.getSelectedRow(), true); return true; }
        if (k == juce::KeyPress::downKey || k == juce::KeyPress::upKey)
        {
            const int d = k == juce::KeyPress::downKey ? 1 : -1;
            const int row = juce::jlimit (0, juce::jmax (0, (int) shown.size() - 1), list.getSelectedRow() + d);
            list.selectRow (row);
            return true;
        }
        return false;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! panel.contains (e.getPosition()) && onClose) onClose();   // click outside closes
    }

private:
    struct Entry { juce::String name, category; int factoryIndex; juce::File file; juce::String favKey; };

    class CloseButton : public juce::Button
    {
    public:
        CloseButton() : juce::Button ("close") {}
        void paintButton (juce::Graphics& g, bool hover, bool) override
        {
            const auto c = getLocalBounds().toFloat().getCentre();
            drawOrb (g, c, 11.0f, juce::Colour (0xffffc0c0), hover ? col::red : juce::Colour (0xffb02030), col::redDeep.darker (0.4f));
            g.setColour (juce::Colours::white);
            g.drawLine (c.x - 4.5f, c.y - 4.5f, c.x + 4.5f, c.y + 4.5f, 1.8f);
            g.drawLine (c.x + 4.5f, c.y - 4.5f, c.x - 4.5f, c.y + 4.5f, 1.8f);
        }
    };

    void textEditorTextChanged (juce::TextEditor&) override { refilter(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override { loadRow (juce::jmax (0, list.getSelectedRow()), true); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { if (onClose) onClose(); }

    void refilter()
    {
        shown.clear();
        const auto tokens = juce::StringArray::fromTokens (search.getText().toLowerCase(), " ", "");
        const int fid = filter.getSelectedId();
        const auto cat = filter.getText();
        for (int i = 0; i < (int) entries.size(); ++i)
        {
            const auto& e = entries[(size_t) i];
            if (fid == 2 && ! favs.contains (e.favKey)) continue;
            if (fid >= 3 && e.category != cat) continue;
            const auto hay = (e.name + " " + e.category).toLowerCase();
            bool ok = true;
            for (auto& t : tokens) if (t.isNotEmpty() && ! hay.contains (t)) { ok = false; break; }
            if (ok) shown.add (i);
        }
        list.updateContent();
        const auto cur = currentName ? currentName() : juce::String();
        int sel = -1;
        for (int k = 0; k < shown.size(); ++k) if (entries[(size_t) shown[k]].name == cur) { sel = k; break; }
        if (sel >= 0) { list.selectRow (sel, false, true); list.scrollToEnsureRowIsOnscreen (sel); }
        else list.deselectAllRows();
        repaint();
    }

    void loadRow (int row, bool close)
    {
        if (! juce::isPositiveAndBelow (row, shown.size())) return;
        const auto& e = entries[(size_t) shown[row]];
        if (e.factoryIndex >= 0) { if (onLoadFactory) onLoadFactory (e.factoryIndex); }
        else if (onLoadUser) onLoadUser (e.file);
        list.repaint();
        if (close && onClose) onClose();
    }

    // ---- ListBoxModel
    int getNumRows() override { return shown.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (! juce::isPositiveAndBelow (row, shown.size())) return;
        const auto& e = entries[(size_t) shown[row]];
        if (selected)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc8202c), 0, 0, juce::Colour (0xff9a0c18), 0, (float) h, false));
            g.fillRect (0, 0, w, h);
        }
        else if (row % 2) { g.setColour (juce::Colours::white.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }

        const bool fav = favs.contains (e.favKey);
        juce::Path star;
        star.addStar ({ 19.0f, (float) h * 0.5f }, 5, 4.0f, 9.0f, 0.0f);
        if (fav)
        {
            g.setColour (juce::Colour (0xffffb000));
            g.fillPath (star);
            g.setColour (juce::Colour (0xffa86400));
            g.strokePath (star, juce::PathStrokeType (1.0f));
        }
        else
        {
            g.setColour (selected ? juce::Colours::white.withAlpha (0.7f) : col::ink.withAlpha (0.3f));
            g.strokePath (star, juce::PathStrokeType (1.3f));
        }

        g.setColour (selected ? juce::Colours::white : col::ink);
        g.setFont (sans (14.5f, true));
        g.drawText (e.name, juce::Rectangle<int> (38, 0, w - 200, h), juce::Justification::centredLeft);
        g.setFont (sans (12.0f));
        g.setColour (selected ? juce::Colours::white.withAlpha (0.9f) : col::ink.withAlpha (0.7f));
        g.drawText (e.category, juce::Rectangle<int> (w - 170, 0, 160, h), juce::Justification::centredRight);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (! juce::isPositiveAndBelow (row, shown.size())) return;
        if (e.x < 34)   // the star
        {
            Presets::toggleFavourite (entries[(size_t) shown[row]].favKey);
            favs = Presets::loadFavourites();
            if (filter.getSelectedId() == 2) refilter(); else list.repaintRow (row);
            return;
        }
        loadRow (row, false);
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent& e) override { if (e.x >= 34) loadRow (row, true); }
    void returnKeyPressed (int row) override { loadRow (row, true); }

    std::vector<Entry> entries;
    juce::Array<int> shown;
    juce::StringArray favs;
    juce::TextEditor search;
    juce::ComboBox filter;
    juce::ListBox list;
    CloseButton closeBtn;
    juce::Rectangle<int> panel;
};
} // namespace aero
