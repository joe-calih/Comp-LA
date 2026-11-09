#pragma once
#include "PluginProcessor.h"
#include "LevelerLookAndFeel.h"

// chrome toggle switch with hex nut and lever
class ToggleSwitch : public juce::Button
{
public:
    explicit ToggleSwitch (bool invertDirection = false) : juce::Button ({}), invert (invertDirection)
    { setClickingTogglesState (true); }
    void paintButton (juce::Graphics& g, bool, bool) override;
private:
    bool invert;
};

// amber VU meter
class VUMeter : public juce::Component
{
public:
    void setValue (float f) { needle = juce::jlimit (0.f, 1.f, f); repaint(); }
    void setGRMode (bool b) { grMode = b; }
    void paint (juce::Graphics&) override;
private:
    float needle = 0.f; bool grMode = false;
};

class HamburgerButton : public juce::Button
{
public:
    HamburgerButton() : juce::Button ({}) {}
    void paintButton (juce::Graphics&, bool, bool) override;
};

class PowerButton : public juce::Button
{
public:
    PowerButton() : juce::Button ({}) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool, bool) override;
};

class LALevelerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LALevelerAudioProcessorEditor (LALevelerAudioProcessor&);
    ~LALevelerAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void buildPresetMenu();
    void stepPreset (int dir);
    void promptSaveAs();
    void setMeterMode (int m);
    void showMainMenu();
    void showAbout();
    void promptNote();
    void setupKnob (juce::Slider&, const char* id, const char* labels, int minor, juce::Rectangle<int> b,
                    const char* suffix, double def);

    LALevelerAudioProcessor& proc;
    LevelerLookAndFeel laf;

    juce::Slider gain, peak, inLevel, output, parallel, harm, lf;
    std::unique_ptr<SA> aGain, aPeak, aIn, aOut, aPar, aHarm, aLf;
    ToggleSwitch limitSw, tubeSw, msSw { true };
    std::unique_ptr<BA> aLimit, aTube, aMs, aPower;
    PowerButton power;
    VUMeter vu;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, saveAsBtn { "SAVE AS" },
                     aBtn { "A" }, bBtn { "B" }, abArrow { ">" }, undoBtn { "UNDO" }, redoBtn { "REDO" },
                     menuBtnUnused { "" };
    HamburgerButton menuBtn;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltip { this, 600 };
    bool lastDark = false;
    float uiScale = 1.0f;
    juce::TextButton meterBtn[4] { juce::TextButton ("IN"), juce::TextButton ("GR"),
                                   juce::TextButton ("OUT"), juce::TextButton ("HAR") };
    juce::StringArray userNames;
    float needleSmooth = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LALevelerAudioProcessorEditor)
};
