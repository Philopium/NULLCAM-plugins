#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: near-black card, cyan and magenta accents, a 16-step pattern
// grid at the centre, and a global delay section around it.
class EchoMatrixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EchoMatrixLookAndFeel();

    juce::Font titleFont (float height) const;   // Echo Matrix, the shout
    juce::Font labelFont (float height) const;   // small caps
    juce::Font valueFont (float height) const;   // readouts

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

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
    static juce::Colour cyan();
    static juce::Colour magenta();

    static juce::ColourGradient accentGradient (juce::Rectangle<float> area);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoMatrixLookAndFeel)
};

class EchoMatrixModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit EchoMatrixModeAudioProcessorEditor (EchoMatrixModeAudioProcessor&);
    ~EchoMatrixModeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Pulls live data into the panel. The timer calls it while open; the
    // offscreen render tool calls it directly.
    void pullLiveData();

private:
    struct Knob
    {
        juce::String label;
        juce::String paramId;
        int column = 0;
        int row = 0;
        juce::Rectangle<int> cell;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (const char* paramId, const char* label, int column, int row);

    void timerCallback() override;

    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintGlobalSection (juce::Graphics&);
    void paintPatternGrid (juce::Graphics&);
    void paintStepEditor (juce::Graphics&);
    void paintMeterSection (juce::Graphics&);
    void paintBaseplate (juce::Graphics&);

    void refreshPresetList();
    void choosePreset (int itemId);
    void setPresetName (const juce::String& name);
    void promptSavePreset();
    void promptDeletePreset();
    void updatePresetButtons();

    void selectStep (int step);
    void updateStepSelectionUI();
    void syncStepButtons();

    juce::String valueText (const Knob& knob) const;
    float paramValue (const juce::String& id) const;
    float effectiveDelaySeconds() const;

    EchoMatrixModeAudioProcessor& processor;
    EchoMatrixLookAndFeel lookAndFeel;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };
    juce::ToggleButton syncButton { "SYNC" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;

    std::vector<std::unique_ptr<Knob>> knobs;

    // 16 step toggle buttons (on/off).
    std::array<std::unique_ptr<juce::ToggleButton>, echomatrix::Engine::kNumSteps> stepButtons;

    // Selected step editor controls.
    std::unique_ptr<juce::Slider> stepLevelSlider;
    std::unique_ptr<juce::Slider> stepPanSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepPanAttachment;
    int selectedStep = 0;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> globalArea;
    juce::Rectangle<int> patternArea;
    juce::Rectangle<int> stepArea;
    juce::Rectangle<int> meterArea;

    juce::StringArray userPresets;
    juce::String presetName { "DOTTED EIGHTH" };
    bool edited = false;
    bool applyingPreset = false;
    int frame = 0;

    // Smoothed readouts.
    float activeEvents = 0.0f;
    float wetLevel = 0.0f;
    int currentStep = 0;
    float stepPhase = 0.0f;

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoMatrixModeAudioProcessorEditor)
};
