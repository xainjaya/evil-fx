#pragma once
#include <juce_dsp/juce_dsp.h>
#include "EvilLookAndFeel.h"

// VISUALIZER.EXE — a WinAmp-style spectrum / oscilloscope.
// Off by default. While off it does no work at all: the processor stops pushing
// samples, the editor timer drops back to 15 Hz, and this component never repaints.
namespace aero
{
class Visualizer : public juce::Component
{
public:
    std::function<void (bool on, int mode)> onChange;

    Visualizer()
    {
        power.getProperties().set ("retro", true);
        power.getProperties().set ("power", true);
        power.setButtonText ("ON");
        power.setTooltip ("Visualizer on/off (uses a little CPU while open)");
        power.onClick = [this] { notify(); };
        addAndMakeVisible (power);

        for (auto* b : { &bars, &scope })
        {
            b->getProperties().set ("retro", true);
            b->setRadioGroupId (8008);
            b->onClick = [this] { notify(); repaint(); };
            addAndMakeVisible (b);
        }
        bars.setToggleState (true, juce::dontSendNotification);

        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize);
    }

    void setState (bool on, int mode)
    {
        power.setToggleState (on, juce::dontSendNotification);
        (mode == 1 ? scope : bars).setToggleState (true, juce::dontSendNotification);
        repaint();
    }
    bool isOn() const { return power.getToggleState(); }
    int mode() const { return scope.getToggleState() ? 1 : 0; }

    // call from the editor timer while on
    template <typename Pull>
    void tick (Pull&& pull, double sampleRate)
    {
        float tmp[4096];
        const int got = pull (tmp, 4096);
        for (int i = 0; i < got; ++i) { history[(size_t) histPos] = tmp[i]; histPos = (histPos + 1) % histLen; }
        if (mode() == 0) computeBars (sampleRate);
        repaint (screen);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().reduced (3);
        g.setColour (juce::Colour (0x30000000));
        g.fillRect (r.translated (3, 3));
        g.setColour (col::w98Grey);
        g.fillRect (r);
        bevel (g, r, true);

        auto bar = r.reduced (3).withHeight (24);
        g.setGradientFill (juce::ColourGradient (col::w98Maroon, (float) bar.getX(), 0, col::w98Red, (float) bar.getRight(), 0, false));
        g.fillRect (bar);
        g.setColour (juce::Colours::white);
        g.setFont (mono (12.5f, true));
        g.drawText ("VISUALIZER.EXE", bar.withTrimmedLeft (8), juce::Justification::centredLeft);

        g.setColour (juce::Colours::black);
        g.fillRect (screen);
        bevel (g, screen.expanded (2), false);

        if (! isOn())
        {
            g.setFont (mono (13.0f, true));
            g.setColour (juce::Colour (0xffff3030));
            g.drawText ("[ visualizer off ]", screen.withTrimmedBottom (18), juce::Justification::centred);
            g.setFont (mono (11.0f));
            g.setColour (juce::Colour (0xff905050));
            g.drawText ("tick ON to watch the evil", screen.withTrimmedTop (18), juce::Justification::centred);
            return;
        }
        mode() == 0 ? paintBars (g) : paintScope (g);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        auto title = r.removeFromTop (24);
        power.setBounds (title.removeFromRight (56).reduced (2, 3));
        r.removeFromTop (6);
        auto opts = r.removeFromBottom (22);
        bars.setBounds (opts.removeFromLeft (80));
        scope.setBounds (opts.removeFromLeft (80));
        r.removeFromBottom (4);
        screen = r.reduced (2);
    }

private:
    static constexpr int fftOrder = 10, fftSize = 1 << fftOrder, numBars = 32, histLen = 4096;

    void notify() { if (onChange) onChange (isOn(), mode()); repaint(); }

    void computeBars (double sr)
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = history[(size_t) ((histPos - fftSize + i + histLen) % histLen)] * window[(size_t) i];
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

        const double binHz = sr / fftSize;
        for (int b = 0; b < numBars; ++b)
        {
            const double f0 = 40.0 * std::pow (16000.0 / 40.0, (double) b / numBars);
            const double f1 = 40.0 * std::pow (16000.0 / 40.0, (double) (b + 1) / numBars);
            const int k0 = juce::jlimit (1, fftSize / 2 - 1, (int) (f0 / binHz));
            const int k1 = juce::jlimit (k0, fftSize / 2 - 1, (int) (f1 / binHz));
            float mag = 0.0f;
            for (int k = k0; k <= k1; ++k) mag = juce::jmax (mag, fftData[(size_t) k]);
            const float db = juce::Decibels::gainToDecibels (mag / (fftSize * 0.25f), -72.0f);
            const float level = juce::jlimit (0.0f, 1.0f, (db + 72.0f) / 72.0f);
            levels[(size_t) b] = juce::jmax (level, levels[(size_t) b] - 0.035f);
            if (levels[(size_t) b] >= peaks[(size_t) b]) { peaks[(size_t) b] = levels[(size_t) b]; peakHold[(size_t) b] = 18; }
            else if (--peakHold[(size_t) b] < 0) peaks[(size_t) b] = juce::jmax (0.0f, peaks[(size_t) b] - 0.02f);
        }
    }

    void paintBars (juce::Graphics& g)
    {
        const auto s = screen.toFloat().reduced (4.0f);
        const float bw = s.getWidth() / numBars;
        constexpr int segments = 16;
        const float segH = s.getHeight() / segments;
        for (int b = 0; b < numBars; ++b)
        {
            const int lit = juce::roundToInt (levels[(size_t) b] * segments);
            for (int k = 0; k < lit; ++k)
            {
                const float t = (float) k / (segments - 1);
                g.setColour (t < 0.6f ? col::red.interpolatedWith (col::ember, t / 0.6f)
                                      : col::ember.interpolatedWith (juce::Colour (0xffffe040), (t - 0.6f) / 0.4f));
                g.fillRect (s.getX() + b * bw + 1.0f, s.getBottom() - (k + 1) * segH + 1.0f, bw - 2.0f, segH - 2.0f);
            }
            const float py = s.getBottom() - peaks[(size_t) b] * s.getHeight();
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (s.getX() + b * bw + 1.0f, py - 2.0f, bw - 2.0f, 2.0f);
        }
    }

    void paintScope (juce::Graphics& g)
    {
        const auto s = screen.toFloat().reduced (4.0f);
        g.setColour (juce::Colour (0xff3a0808));
        g.drawHorizontalLine ((int) s.getCentreY(), s.getX(), s.getRight());

        constexpr int span = 768;
        int start = (histPos - span - 512 + histLen) % histLen;   // look for a rising zero-crossing so it holds still
        for (int k = 0; k < 512; ++k)
        {
            const int i0 = (start + k) % histLen, i1 = (i0 + 1) % histLen;
            if (history[(size_t) i0] <= 0.0f && history[(size_t) i1] > 0.0f) { start = i1; break; }
        }
        juce::Path p;
        for (int k = 0; k < span; ++k)
        {
            const float v = juce::jlimit (-1.0f, 1.0f, history[(size_t) ((start + k) % histLen)]);
            const float x = s.getX() + s.getWidth() * (float) k / (span - 1), y = s.getCentreY() - v * s.getHeight() * 0.48f;
            k == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (col::red.withAlpha (0.35f));
        g.strokePath (p, juce::PathStrokeType (4.0f));
        g.setColour (juce::Colour (0xffff7a50));
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    juce::ToggleButton power, bars { "BARS" }, scope { "SCOPE" };
    juce::Rectangle<int> screen;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> window {};
    std::array<float, fftSize * 2> fftData {};
    std::array<float, histLen> history {};
    std::array<float, numBars> levels {}, peaks {};
    std::array<int, numBars> peakHold {};
    int histPos = 0;
};
} // namespace aero
