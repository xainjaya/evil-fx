#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Two looks in one LookAndFeel:
//  * Frutiger Aero, evil edition (default): glossy crimson orbs, glass, ember highlights
//  * Retro Web 1.0 (component property "retro" = true): grey bevels, brick, maroon
namespace aero
{
namespace col
{
    const juce::Colour ink       { 0xff3a070c };   // text
    const juce::Colour red       { 0xffe01b2a };
    const juce::Colour redDeep   { 0xff7d0410 };
    const juce::Colour redPale   { 0xffffd9d6 };
    const juce::Colour ember     { 0xffff7a1a };   // hot-orange accent
    const juce::Colour emberDeep { 0xffb83200 };
    const juce::Colour w98Grey   { 0xffc0c0c0 };
    const juce::Colour w98Dark   { 0xff808080 };
    const juce::Colour w98Maroon { 0xff800000 };
    const juce::Colour w98Red    { 0xffd42020 };
    const juce::Colour w98Brick  { 0xff9e1010 };
}

inline bool isRetro (const juce::Component& c) { return (bool) c.getProperties().getWithDefault ("retro", false); }

inline juce::Font sans (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}
inline juce::Font mono (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, bold ? juce::Font::bold : juce::Font::plain));
}

// Classic Win9x two-tone bevel
inline void bevel (juce::Graphics& g, juce::Rectangle<int> r, bool raised = true)
{
    const auto tl = raised ? juce::Colours::white : col::w98Dark;
    const auto br = raised ? juce::Colours::black : juce::Colours::white;
    g.setColour (tl);
    g.fillRect (r.getX(), r.getY(), r.getWidth() - 1, 1);
    g.fillRect (r.getX(), r.getY(), 1, r.getHeight() - 1);
    g.setColour (br);
    g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1);
    g.fillRect (r.getRight() - 1, r.getY(), 1, r.getHeight());
    g.setColour (raised ? col::w98Dark : juce::Colour (0xff404040));
    g.fillRect (r.getX() + 1, r.getBottom() - 2, r.getWidth() - 2, 1);
    g.fillRect (r.getRight() - 2, r.getY() + 1, 1, r.getHeight() - 2);
}

// Glossy orb used for knobs and the logo
inline void drawOrb (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour light, juce::Colour mid, juce::Colour edge)
{
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    g.fillEllipse (c.x - r, c.y - r + 2.5f, r * 2.0f, r * 2.0f);

    juce::ColourGradient body (light, c.x - r * 0.3f, c.y - r * 0.5f, edge, c.x + r * 0.2f, c.y + r * 1.25f, true);
    body.addColour (0.5, mid);
    g.setGradientFill (body);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    // caustic glow at the bottom (light passing through the glass)
    juce::ColourGradient caustic (juce::Colours::white.withAlpha (0.0f), c.x, c.y + r * 0.1f,
                                  juce::Colours::white.withAlpha (0.45f), c.x, c.y + r, false);
    g.setGradientFill (caustic);
    g.fillEllipse (c.x - r * 0.75f, c.y + r * 0.05f, r * 1.5f, r * 0.9f);

    // specular highlight
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.92f), c.x, c.y - r * 0.92f,
                               juce::Colours::white.withAlpha (0.05f), c.x, c.y - r * 0.02f, false);
    g.setGradientFill (spec);
    g.fillEllipse (c.x - r * 0.64f, c.y - r * 0.9f, r * 1.28f, r * 0.86f);

    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
    g.setColour (edge.darker (0.4f).withAlpha (0.5f));
    g.drawEllipse (c.x - r - 0.5f, c.y - r - 0.5f, r * 2.0f + 1.0f, r * 2.0f + 1.0f, 0.8f);
}

// The Evil FX mascot: glossy blue smiley wearing a sideways orange cap with a
// purple/black checker patch. `c` is the centre of the head, `r` its radius;
// the cap reaches ~1.35r above the centre and the brim ~1.55r to the right.
// blink: 0 = eyes open, 1 = shut. grin: 0 = normal smile, 1 = big open-mouth grin.
inline void drawSmiley (juce::Graphics& g, juce::Point<float> c, float r, float blink = 0.0f, float grin = 0.0f)
{
    blink = juce::jlimit (0.0f, 1.0f, blink);
    grin = juce::jlimit (0.0f, 1.0f, grin);
    auto P = [c, r] (float x, float y) { return juce::Point<float> (c.x + x * r, c.y + y * r); };

    // soft blue glow behind everything
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6f8dff).withAlpha (0.38f), c.x, c.y - r * 0.2f,
                                             juce::Colour (0xff6f8dff).withAlpha (0.0f), c.x + r * 1.7f, c.y, true));
    g.fillEllipse (c.x - r * 1.7f, c.y - r * 1.9f, r * 3.4f, r * 3.4f);

    // head
    drawOrb (g, c, r, juce::Colour (0xffdce8ff), juce::Colour (0xff86a4ff), juce::Colour (0xff3d4fd6));

    // face — sits under the cap
    const auto ink = juce::Colour (0xff12142e);
    for (float ex : { -0.37f, 0.37f })
    {
        const auto e = P (ex, -0.05f);
        const float eh = r * 0.4f * juce::jmax (0.12f, 1.0f - blink);
        g.setColour (ink);
        g.fillEllipse (e.x - r * 0.12f, e.y - eh * 0.5f, r * 0.24f, eh);
        if (blink < 0.5f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.6f * (1.0f - blink * 2.0f)));
            g.fillEllipse (e.x + r * 0.01f, e.y - eh * 0.35f, r * 0.07f, eh * 0.3f);
        }
    }
    const float depth = 0.66f + 0.22f * grin;
    juce::Path smile;
    smile.startNewSubPath (P (-0.68f, 0.12f));
    smile.cubicTo (P (-0.42f, depth), P (0.42f, depth + 0.02f), P (0.66f, 0.1f));
    if (grin > 0.3f)   // mouth opens into a grin
    {
        const float open = (grin - 0.3f) / 0.7f;
        juce::Path mouth (smile);
        mouth.cubicTo (P (0.4f, 0.1f + 0.2f * (1.0f - open)), P (-0.4f, 0.1f + 0.2f * (1.0f - open)), P (-0.68f, 0.12f));
        mouth.closeSubPath();
        g.setColour (juce::Colour (0xff3a0f2a).withAlpha (open));
        g.fillPath (mouth);
        g.setColour (juce::Colour (0xffff6f9a).withAlpha (open * 0.85f));   // tongue
        g.fillEllipse (c.x - r * 0.2f, c.y + r * (depth - 0.2f), r * 0.4f, r * 0.16f);
    }
    g.setColour (ink);
    g.strokePath (smile, juce::PathStrokeType (r * 0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // brim — sticks out sideways to the right, drawn first so the crown overlaps it
    juce::Path brim;
    brim.startNewSubPath (P (0.38f, -0.68f));
    brim.quadraticTo (P (1.2f, -0.7f), P (1.62f, -0.26f));
    brim.lineTo (P (1.6f, 0.0f));
    brim.quadraticTo (P (1.05f, -0.26f), P (0.5f, -0.28f));
    brim.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillPath (brim, juce::AffineTransform::translation (0.0f, r * 0.06f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff9a2e), c.x + r * 0.6f, c.y - r * 0.6f,
                                             juce::Colour (0xffc2470f), c.x + r * 1.3f, c.y - r * 0.05f, false));
    g.fillPath (brim);
    g.setColour (juce::Colour (0xff9a3408).withAlpha (0.7f));
    g.strokePath (brim, juce::PathStrokeType (juce::jmax (0.8f, r * 0.03f)));

    // crown — a dome over the top of the head, rim lower on the left (it's on sideways)
    juce::Path crown;
    crown.startNewSubPath (P (-1.03f, -0.28f));
    crown.cubicTo (P (-1.08f, -1.02f), P (-0.5f, -1.38f), P (0.06f, -1.36f));
    crown.cubicTo (P (0.62f, -1.34f), P (1.08f, -0.98f), P (1.02f, -0.42f));
    crown.quadraticTo (P (0.0f, -0.36f), P (-1.03f, -0.28f));
    crown.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffe066), c.x + r * 0.5f, c.y - r * 1.25f,
                                             juce::Colour (0xffff8c1a), c.x - r * 0.95f, c.y - r * 0.35f, true));
    g.fillPath (crown);
    // gloss on the crown
    juce::Path shine;
    shine.addEllipse (c.x - r * 0.55f, c.y - r * 1.3f, r * 1.25f, r * 0.42f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), c.x, c.y - r * 1.3f,
                                             juce::Colours::white.withAlpha (0.0f), c.x, c.y - r * 0.88f, false));
    g.fillPath (shine);
    // shadow the cap throws onto the forehead
    juce::Path rimShadow;
    rimShadow.startNewSubPath (P (-0.98f, -0.28f));
    rimShadow.quadraticTo (P (0.0f, -0.36f), P (1.0f, -0.42f));
    g.setColour (juce::Colour (0xff1c2a8a).withAlpha (0.35f));
    g.strokePath (rimShadow, juce::PathStrokeType (r * 0.1f), juce::AffineTransform::translation (0.0f, r * 0.06f));
    g.setColour (juce::Colour (0xffb8560c).withAlpha (0.8f));
    g.strokePath (crown, juce::PathStrokeType (juce::jmax (0.8f, r * 0.035f)));

    // purple / black checkerboard patch on the front of the cap, tilted with the cap
    {
        juce::Graphics::ScopedSaveState s (g);
        const auto pc = P (-0.08f, -0.94f);
        g.addTransform (juce::AffineTransform::rotation (0.07f, pc.x, pc.y));
        const int rows = 3, cols = 8;
        const float pw = r * 0.92f, ph = r * 0.34f, cw = pw / cols, ch = ph / rows;
        const float x0 = pc.x - pw * 0.5f, y0 = pc.y - ph * 0.5f;
        g.setColour (juce::Colours::black);
        g.fillRect (x0, y0, pw, ph);
        g.setColour (juce::Colour (0xffe21cf0));
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x)
                if (((x + y) & 1) == 0) g.fillRect (x0 + x * cw, y0 + y * ch, cw + 0.05f, ch + 0.05f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (x0, y0, pw, ph, juce::jmax (0.6f, r * 0.02f));
    }
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, col::ink);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, col::red.withAlpha (0.4f));
        setColour (juce::TextEditor::textColourId, col::ink);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
        setColour (juce::TextEditor::highlightColourId, col::red.withAlpha (0.35f));
        setColour (juce::CaretComponent::caretColourId, col::ink);
        setColour (juce::ComboBox::textColourId, col::ink);
        setColour (juce::ComboBox::arrowColourId, col::ink);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfffff0f0));
        setColour (juce::PopupMenu::textColourId, col::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::red);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::Label::textColourId, col::ink);
        setColour (juce::ScrollBar::thumbColourId, col::red.withAlpha (0.7f));
        setColour (juce::ScrollBar::trackColourId, juce::Colours::white.withAlpha (0.25f));
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xffffffe1));
        setColour (juce::TooltipWindow::textColourId, juce::Colours::black);
        setColour (juce::TooltipWindow::outlineColourId, juce::Colours::black);
    }

    //==========================================================================
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcR = r - 2.0f;

        if (isRetro (s))
        {
            // tick ring
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            for (int i = 0; i <= 10; ++i)
            {
                const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
                g.drawLine (juce::Line<float> (c.getPointOnCircumference (arcR - 3.5f, a), c.getPointOnCircumference (arcR, a)), 1.0f);
            }
            juce::Path value;
            value.addCentredArc (c.x, c.y, arcR - 5.5f, arcR - 5.5f, 0.0f, startAngle, angle, true);
            g.setColour (col::w98Brick);
            g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

            const float kr = r - 10.0f;
            g.setColour (col::w98Grey);
            g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
            juce::Path hiArc, loArc;
            hiArc.addCentredArc (c.x, c.y, kr, kr, 0.0f, -2.36f, 0.78f, true);
            loArc.addCentredArc (c.x, c.y, kr, kr, 0.0f, 0.78f, 3.93f, true);
            g.setColour (juce::Colours::white);  g.strokePath (hiArc, juce::PathStrokeType (2.0f));
            g.setColour (juce::Colour (0xff404040)); g.strokePath (loArc, juce::PathStrokeType (2.0f));
            g.setColour (juce::Colours::black);
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (kr * 0.25f, angle), c.getPointOnCircumference (kr * 0.85f, angle)), 2.5f);
            return;
        }

        // aero: track + glowing value arc
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (col::ink.withAlpha (0.14f));
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        juce::ColourGradient arcGrad (col::red, c.x - r, c.y + r, col::ember, c.x + r, c.y - r, false);
        g.setGradientFill (arcGrad);
        g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = r - 8.0f;
        drawOrb (g, c, kr, col::redPale, juce::Colour (0xffff3644), col::redDeep);

        const auto dot = c.getPointOnCircumference (kr * 0.62f, angle);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (dot.x - 4.5f, dot.y - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        const bool retro = isRetro (s);
        l->setFont (retro ? mono (11.0f) : sans (11.5f));
        l->setColour (juce::Label::textColourId, retro ? juce::Colours::black : col::ink);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        return l;
    }

    //==========================================================================
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        const bool on = b.getToggleState(), retro = isRetro (b);
        const bool isPower = (bool) b.getProperties().getWithDefault ("power", false);
        auto bounds = b.getLocalBounds().toFloat();

        if (isPower && ! retro)
        {
            const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
            const auto c = bounds.getCentre();
            if (on)
            {
                g.setColour (col::ember.withAlpha (0.45f));
                g.fillEllipse (c.x - r - 2.5f, c.y - r - 2.5f, (r + 2.5f) * 2, (r + 2.5f) * 2);
                drawOrb (g, c, r, juce::Colour (0xfffff0c8), col::ember, col::emberDeep.darker (0.3f));
            }
            else
                drawOrb (g, c, r, juce::Colours::white, juce::Colour (0xffe6cccc), juce::Colour (0xffa67b7b));

            juce::Path sym;   // power symbol
            const float sr = r * 0.45f;
            sym.addCentredArc (c.x, c.y, sr, sr, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
            sym.startNewSubPath (c.x, c.y - sr * 1.25f);
            sym.lineTo (c.x, c.y - sr * 0.15f);
            g.setColour (on ? juce::Colours::white : col::ink.withAlpha (0.55f));
            g.strokePath (sym, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }

        if (retro)   // Win9x checkbox + label
        {
            const int box = 13;
            auto br = juce::Rectangle<int> (2, (b.getHeight() - box) / 2, box, box);
            g.setColour (juce::Colours::white);
            g.fillRect (br);
            bevel (g, br, false);
            if (on)
            {
                juce::Path tick;
                tick.startNewSubPath ((float) br.getX() + 3.0f, (float) br.getCentreY());
                tick.lineTo ((float) br.getX() + 5.5f, (float) br.getBottom() - 3.5f);
                tick.lineTo ((float) br.getRight() - 3.0f, (float) br.getY() + 3.0f);
                g.setColour (juce::Colours::black);
                g.strokePath (tick, juce::PathStrokeType (2.0f));
            }
            if (b.getButtonText().isNotEmpty())
            {
                g.setColour (isPower ? juce::Colours::white : juce::Colours::black);
                g.setFont (mono (11.5f, isPower));
                g.drawText (b.getButtonText(), box + 7, 0, b.getWidth() - box - 7, b.getHeight(), juce::Justification::centredLeft);
            }
            return;
        }

        // aero pill switch + label
        auto pill = juce::Rectangle<float> (2.0f, (bounds.getHeight() - 16.0f) * 0.5f, 32.0f, 16.0f);
        juce::ColourGradient fill (on ? juce::Colour (0xffffd38a) : juce::Colours::white, pill.getX(), pill.getY(),
                                   on ? col::emberDeep : juce::Colour (0xffd9b8b8), pill.getX(), pill.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (pill, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillRoundedRectangle (pill.withHeight (7.0f).reduced (3.0f, 1.0f), 3.5f);
        g.setColour (col::ink.withAlpha (hover ? 0.6f : 0.35f));
        g.drawRoundedRectangle (pill, 8.0f, 1.0f);
        const float kx = on ? pill.getRight() - 15.0f : pill.getX() + 1.0f;
        drawOrb (g, { kx + 7.0f, pill.getCentreY() }, 6.5f, juce::Colours::white, juce::Colour (0xfffaeeee), juce::Colour (0xffbb8a8a));

        g.setColour (col::ink);
        g.setFont (sans (12.0f, true));
        g.drawText (b.getButtonText(), 40, 0, b.getWidth() - 40, b.getHeight(), juce::Justification::centredLeft);
    }

    //==========================================================================
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<int> (0, 0, w, h);
        if (isRetro (box))
        {
            g.setColour (juce::Colours::white);
            g.fillRect (r);
            bevel (g, r, false);
            auto btn = r.removeFromRight (h).reduced (2);
            g.setColour (col::w98Grey);
            g.fillRect (btn);
            bevel (g, btn, true);
            drawArrow (g, btn.toFloat(), juce::Colours::black);
            return;
        }
        auto rf = r.toFloat().reduced (0.5f);
        const float corner = rf.getHeight() * 0.5f;
        juce::ColourGradient fill (juce::Colours::white, 0, 0, juce::Colour (0xffffcfcf), 0, (float) h, false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (rf, corner);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.fillRoundedRectangle (rf.withHeight (rf.getHeight() * 0.45f).reduced (corner * 0.5f, 1.0f), corner * 0.4f);
        g.setColour (col::redDeep.withAlpha (0.55f));
        g.drawRoundedRectangle (rf, corner, 1.0f);
        drawArrow (g, rf.removeFromRight ((float) h).reduced (2.0f), col::ink);
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        return isRetro (box) ? mono (12.0f) : sans (13.0f, true);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (isRetro (box) ? 4 : 10, 1, box.getWidth() - box.getHeight() - (isRetro (box) ? 4 : 10), box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, isRetro (box) ? juce::Colours::black : col::ink);
    }

    juce::Font getPopupMenuFont() override { return sans (14.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        juce::ColourGradient bg (juce::Colours::white, 0, 0, juce::Colour (0xffffe0e0), 0, (float) h, false);
        g.setGradientFill (bg);
        g.fillAll();
        g.setColour (col::redDeep.withAlpha (0.5f));
        g.drawRect (0, 0, w, h);
    }

    //==========================================================================
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        if (b.getWidth() > b.getHeight() * 3 / 2)   // wide buttons: glossy pill
        {
            auto r = b.getLocalBounds().toFloat().reduced (1.0f);
            const float corner = r.getHeight() * 0.5f;
            if ((bool) b.getProperties().getWithDefault ("accent", false))   // the main call-to-action: glossy red
            {
                g.setGradientFill (juce::ColourGradient (down ? col::red : juce::Colour (0xffff6a5a), 0, r.getY(),
                                                         hover ? col::red : juce::Colour (0xffb00c18), 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, corner);
                g.setColour (juce::Colours::white.withAlpha (0.45f));
                g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (corner * 0.5f, 1.0f), corner * 0.4f);
                g.setColour (col::redDeep);
                g.drawRoundedRectangle (r, corner, 1.0f);
                return;
            }
            g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xffffb0a0) : juce::Colours::white, 0, r.getY(),
                                                     hover ? juce::Colour (0xffff8a7a) : juce::Colour (0xffffc8c0), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, corner);
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f).reduced (corner * 0.5f, 1.0f), corner * 0.4f);
            g.setColour (col::redDeep.withAlpha (0.55f));
            g.drawRoundedRectangle (r, corner, 1.0f);
            return;
        }
        const auto c = b.getLocalBounds().toFloat().getCentre();
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f;
        drawOrb (g, c, r, down ? juce::Colour (0xffffb0b0) : juce::Colours::white,
                 hover ? juce::Colour (0xffff6f6f) : juce::Colour (0xffff9f9f), col::redDeep);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour ((bool) b.getProperties().getWithDefault ("accent", false) ? juce::Colours::white : col::ink);
        g.setFont (sans (b.getWidth() > b.getHeight() * 3 / 2 ? 12.0f : 14.0f, true));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }

private:
    static void drawArrow (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        juce::Path p;
        const auto ce = r.getCentre();
        p.addTriangle (ce.x - 4.0f, ce.y - 2.0f, ce.x + 4.0f, ce.y - 2.0f, ce.x, ce.y + 3.0f);
        g.setColour (c);
        g.fillPath (p);
    }
};
} // namespace aero
