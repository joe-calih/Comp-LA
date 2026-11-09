#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class LALevelerAudioProcessor : public juce::AudioProcessor
{
public:
    struct Preset
    {
        const char* name;
        float gain, peak;      // 0..100
        bool  limit, tube;
        float inLevel, output; // dB
        float parallel;        // 0..100 (100 = fully compressed)
        bool  midSide;
        float harm;            // 0..1  (Lin -> Dist)
        float lf;              // 0..10 dB sidechain low-cut
    };

    LALevelerAudioProcessor();
    ~LALevelerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Comp LA"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- presets / A-B ----
    static const std::vector<Preset>& factoryPresets();
    void loadFactoryPreset (int index);
    juce::File getUserPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool exportPresetTo (const juce::File& f);
    bool importPresetFrom (const juce::File& f, bool applyIt);
    juce::String stateToString();
    bool applyStateString (const juce::String& xmlText);
    juce::String presetNote;
    std::atomic<bool> testTone { false };
    void toggleAB();
    int  getActiveSlot() const { return activeSlot; }

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    juce::String currentPresetName { "Initial Preset" };

    // meters (written by audio thread, read by GUI)
    std::atomic<float> meterIn { -100.f }, meterOut { -100.f }, meterHar { -100.f }, meterGR { 0.f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void setParam (const char* id, float realValue);

    double fs = 44100.0;
    std::array<juce::dsp::IIR::Filter<float>, 2> scShelf;
    float lastLf = -1.f;
    double tonePhase = 0.0, toneLfo = 0.0; bool toneInit = false;

    float env[2] { 0, 0 }, gr[2] { 0, 0 }, mem[2] { 0, 0 };
    float dcX[2] { 0, 0 }, dcY[2] { 0, 0 };

    juce::SmoothedValue<float> smInGain, smMakeup, smOutGain, smMix, smHarm;

    std::array<juce::ValueTree, 2> slots;
    int activeSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LALevelerAudioProcessor)
};
