#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: near-black card, amber and teal accents, three stage cards
// across the top, the live partial display under them, the meters at the foot.
class HarmonicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HarmonicLookAndFeel();

    juce::Font titleFont (float height) const;   // Harmonic Resonator, the shout
    juce::Font labelFont (float height) const;   // small caps
    juce::Font valueFont (float height) const;   // readouts

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

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
    static juce::Colour amber();
    static juce::Colour teal();

    static juce::ColourGradient accentGradient (juce::Rectangle<float> area);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicLookAndFeel)
};

class HarmonicResonatorModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit HarmonicResonatorModeAudioProcessorEditor (HarmonicResonatorModeAudioProcessor&);
    ~HarmonicResonatorModeAudioProcessorEditor() override;

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
        int group = 0;          // which card the knob belongs to
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
        int slot = 0;           // 0 full width, 1 left half, 2 right half
        std::unique_ptr<juce::ComboBox> box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void addKnob (const char* paramId, const char* label, int group, int column, int row);
    void addChoice (const char* paramId, int group, int slot);

    void layoutKnobGroup (int group, juce::Rectangle<int> content, int columns, int rows,
                          int knobSize);

    void timerCallback() override;

    void paintKnobs (juce::Graphics&, int group);
    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintHarmonics (juce::Graphics&);
    void paintResonators (juce::Graphics&);
    void paintInteraction (juce::Graphics&);
    void paintPartials (juce::Graphics&);
    void paintModulation (juce::Graphics&);
    void paintOutput (juce::Graphics&);
    void paintBaseplate (juce::Graphics&);

    void refreshPresetList();
    void choosePreset (int itemId);
    void setPresetName (const juce::String& name);
    void promptSavePreset();
    void promptDeletePreset();
    void updatePresetButtons();

    juce::String valueText (const Knob& knob) const;
    float paramValue (const juce::String& id) const;

    HarmonicResonatorModeAudioProcessor& processor;
    HarmonicLookAndFeel lookAndFeel;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> harmonicsArea;
    juce::Rectangle<int> resonatorArea;
    juce::Rectangle<int> interactionArea;
    juce::Rectangle<int> partialsArea;
    juce::Rectangle<int> modArea;
    juce::Rectangle<int> outputArea;

    juce::StringArray userPresets;
    juce::String presetName { "BELL METAL" };
    bool edited = false;
    bool applyingPreset = false;
    int frame = 0;

    // Smoothed readouts.
    float inLevel = 0.0f;
    float wetLevel = 0.0f;
    float trackedHz = 0.0f;
    float confidence = 0.0f;
    int activePartials = 0;
    std::array<float, harmonicresonator::kNumResonators> partialEnergy {};
    std::array<float, harmonicresonator::kNumResonators> partialFreq {};

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicResonatorModeAudioProcessorEditor)
};
