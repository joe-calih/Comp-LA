#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

namespace col
{
    const juce::Colour topBar    { 0xff101010 };
    const juce::Colour panelHi   { 0xffe9e7e2 }, panelLo { 0xffd6d4cf };
    const juce::Colour silverHi  { 0xffd2d4d6 }, silverLo { 0xffb4b7ba };
    const juce::Colour ink       { 0xff2b2b2b };
    const juce::Colour accent    { 0xffe8573c };   // orange (power / A / active)
    const juce::Colour logoRed   { 0xffb8322a };
    const juce::Colour screwDark { 0xff1b1b1b };
}

inline juce::Font uiFont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class LevelerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LevelerLookAndFeel()
    {
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff222222));
        setColour (juce::TextButton::buttonOnColourId, col::accent);
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd8d8d8));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1c1c));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (19.f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (11.f, true); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        g.setColour (juce::Colour (0xff181818));
        g.fillRect (0, 0, w, h);
        juce::Path p;                                     // chevron
        const float cx = (float) w - 22.f, cy = (float) h * 0.5f;
        p.startNewSubPath (cx - 6, cy - 3); p.lineTo (cx, cy + 3); p.lineTo (cx + 6, cy - 3);
        g.setColour (juce::Colours::white);
        g.strokePath (p, juce::PathStrokeType (1.8f));
    }

    void positionComboBoxText (juce::ComboBox& b, juce::Label& l) override
    {
        l.setBounds (14, 1, b.getWidth() - 50, b.getHeight() - 2);
        l.setFont (getComboBoxFont (b));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider& s) override
    {
        using namespace juce;
        const auto c = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
        const float R = (float) jmin (w, h) * 0.5f;
        const float ang = a0 + pos * (a1 - a0);
        auto pt = [&] (float a, float r) { return Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };

        if (s.getProperties().contains ("arrowKnob"))
        {
            const float r = R * 0.72f;
            ColourGradient gr (Colour (0xff4a4a4a), c.x - r, c.y - r, Colour (0xff0e0e0e), c.x + r, c.y + r, false);
            g.setGradientFill (gr); g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
            g.setColour (Colour (0xff8a8a8a)); g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.5f);
            const float L = r * 0.78f, w = r * 0.30f;      // modern solid arrow: triangular head + tapered shaft
            Path arrow;
            arrow.startNewSubPath (0.f, -L);
            arrow.lineTo (w * 1.45f, -L * 0.18f);
            arrow.lineTo (w * 0.42f, -L * 0.18f);
            arrow.lineTo (w * 0.42f, L * 0.72f);
            arrow.lineTo (-w * 0.42f, L * 0.72f);
            arrow.lineTo (-w * 0.42f, -L * 0.18f);
            arrow.lineTo (-w * 1.45f, -L * 0.18f);
            arrow.closeSubPath();
            arrow = arrow.createPathWithRoundedCorners (1.6f);
            const auto xf = AffineTransform::rotation (ang).translated (c.x, c.y);
            g.setColour (Colour (0x66000000)); g.fillPath (arrow, xf.translated (0.8f, 1.2f));
            g.setGradientFill (ColourGradient (Colours::white, c.x, c.y - L, Colour (0xffc9ccd0), c.x, c.y + L, false));
            g.fillPath (arrow, xf);
            return;
        }

        const float bodyR = R * 0.50f;

        // scale ticks + numbers
        const auto labels = StringArray::fromTokens (s.getProperties().getWithDefault ("labels", "").toString(), "|", "");
        const int nMajor = labels.size();
        const int minorPerMajor = (int) s.getProperties().getWithDefault ("minor", 1);
        g.setColour (col::ink);
        if (nMajor > 1)
        {
            const int total = (nMajor - 1) * minorPerMajor;
            for (int i = 0; i <= total; ++i)
            {
                const bool major = i % minorPerMajor == 0;
                const float a = a0 + (a1 - a0) * (float) i / (float) total;
                g.drawLine ({ pt (a, R * (major ? 0.60f : 0.64f)), pt (a, R * 0.72f) }, major ? 1.6f : 1.0f);
            }
            g.setFont (uiFont (R * 0.17f, true));
            for (int i = 0; i < nMajor; ++i)
            {
                const float a = a0 + (a1 - a0) * (float) i / (float) (nMajor - 1);
                auto p = pt (a, R * 0.86f);
                if (labels[i] != "~")
                    g.drawText (labels[i], Rectangle<float> (30, 13).withCentre (p), Justification::centred, false);
            }
        }

        // knob shadow + fluted body
        g.setColour (Colour (0x55000000)); g.fillEllipse (c.x - bodyR + 2, c.y - bodyR + 3, bodyR * 2, bodyR * 2);
        for (int i = 0; i < 36; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / 36.f;
            g.setColour (Colour (0xff050505));
            g.drawLine ({ pt (a, bodyR * 0.93f), pt (a, bodyR * 1.04f) }, 2.4f);
        }
        ColourGradient body (Colour (0xff4b4b4b), c.x - bodyR * 0.6f, c.y - bodyR * 0.8f,
                             Colour (0xff0a0a0a), c.x + bodyR * 0.7f, c.y + bodyR * 0.9f, false);
        g.setGradientFill (body); g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
        const float capR = bodyR * 0.70f;
        ColourGradient cap (Colour (0xff3a3a3a), c.x, c.y - capR, Colour (0xff111111), c.x, c.y + capR, false);
        g.setGradientFill (cap); g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
        g.setColour (Colour (0xff6a6a6a)); g.drawEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2, 1.0f);

        // pointer: white wedge from rim towards the centre
        Path ptr; ptr.addRoundedRectangle (-2.2f, -bodyR * 1.02f, 4.4f, bodyR * 0.95f, 1.6f);
        g.setColour (Colours::white);
        g.fillPath (ptr, AffineTransform::rotation (ang).translated (c.x, c.y));
    }
};
