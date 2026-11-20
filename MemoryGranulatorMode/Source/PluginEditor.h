#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <memory>
#include <vector>

class MemoryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MemoryLookAndFeel();

    juce::Font titleFont (float height) const;
    juce::Font labelFont (float height) const;
    juce::Font valueFont (float height) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    static juce::Colour base();
    static juce::Colour card();
    static juce::Colour cardTop();
    static juce::Colour line();
    static juce::Colour ink();
    static juce::Colour dim();
    static juce::Colour amber();
    static juce::Colour teal();
    static juce::Colour coral();

    static juce::ColourGradient accentGradient (juce::Rectangle<float> area);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryLookAndFeel)
};

class MemoryGranulatorModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit MemoryGranulatorModeAudioProcessorEditor (MemoryGranulatorModeAudioProcessor&);
    ~MemoryGranulatorModeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void pullLiveData();

private:
    enum Group
    {
        memoryGroup = 0,
        granularGroup,
        pitchCoreGroup,
        pitchMapGroup,
        harmonicsGroup,
        spaceGroup,
        modulationGroup,
        outputGroup,
        groupCount
    };

    struct Knob
    {
        juce::String label;
        juce::String paramId;
        int group = 0;
        int column = 0;
        int row = 0;
        juce::Rectangle<int> cell;
        juce::Rectangle<int> labelBounds;
        juce::Rectangle<int> valueBounds;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::String paramId;
        int group = 0;
        int slot = 0;
        std::unique_ptr<juce::ComboBox> box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct Toggle
    {
        juce::String label;
        juce::String paramId;
        int group = 0;
        int slot = 0;
        std::unique_ptr<juce::ToggleButton> button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void addKnob (const char* paramId, const char* label, int group, int column, int row);
    void addChoice (const char* paramId, int group, int slot);
    void addToggle (const char* paramId, const char* label, int group, int slot);

    void layoutKnobGroup (int group, juce::Rectangle<int> content, int columns, int rows,
                          int knobSize);
    void layoutChoices();
    void layoutToggles();

    void timerCallback() override;

    void paintBaseplate (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintCardTitle (juce::Graphics&, juce::Rectangle<int>, const juce::String&, const juce::String&);
    void paintKnobs (juce::Graphics&, int group);
    void paintMemory (juce::Graphics&);
    void paintGranular (juce::Graphics&);
    void paintPitch (juce::Graphics&);
    void paintHarmonics (juce::Graphics&);
    void paintSpace (juce::Graphics&);
    void paintModulation (juce::Graphics&);
    void paintOutput (juce::Graphics&);

    void refreshPresetList();
    void choosePreset (int itemId);
    void setPresetName (const juce::String& name);
    void promptSavePreset();
    void promptDeletePreset();
    void updatePresetButtons();

    juce::String valueText (const Knob& knob) const;
    float paramValue (const juce::String& id) const;

    MemoryGranulatorModeAudioProcessor& processor;
    MemoryLookAndFeel lookAndFeel;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::vector<std::unique_ptr<Toggle>> toggles;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> memoryArea;
    juce::Rectangle<int> granularArea;
    juce::Rectangle<int> pitchArea;
    juce::Rectangle<int> harmonicsArea;
    juce::Rectangle<int> spaceArea;
    juce::Rectangle<int> modulationArea;
    juce::Rectangle<int> outputArea;

    juce::StringArray userPresets;
    juce::String presetName { "MEMORY BLOOM" };
    bool edited = false;
    bool applyingPreset = false;
    int frame = 0;

    float inputLevel = 0.0f;
    float grainLevel = 0.0f;
    float reverbLevel = 0.0f;
    float outputLevel = 0.0f;
    float feedbackLevel = 0.0f;
    float trackedHz = 0.0f;
    float confidence = 0.0f;
    float playhead = 0.5f;
    float modulationValue = 0.0f;
    int activeGrains = 0;
    int activeNotes = 0;
    int midiNotes[8] {};

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryGranulatorModeAudioProcessorEditor)
};
