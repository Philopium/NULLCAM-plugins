#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: near black card, lime and pool inks, and a big nose.
class GiannisLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GiannisLookAndFeel();

    juce::Font gummyFont (float height) const;   // GIANNIS-MODE, the shout
    juce::Font labelFont (float height) const;   // the small caps
    juce::Font bodyFont (float height) const;    // the readouts
    juce::Font numberFont (float height) const;  // the numbers

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
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
    static juce::Colour lime();
    static juce::Colour pool();
    static juce::Colour violet();
    static juce::Colour rose();

    // The two inks the panel mixes for its gradients.
    static juce::ColourGradient inkGradient (juce::Rectangle<float> area, bool vertical = false);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GiannisLookAndFeel)
};

class GiannisModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::AudioProcessorValueTreeState::Listener,
                                        private juce::Timer
{
public:
    explicit GiannisModeAudioProcessorEditor (GiannisModeAudioProcessor&);
    ~GiannisModeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Pulls the live data into the panel. The timer calls it while the editor is
    // open; the offscreen render tool calls it directly.
    void pullLiveData();

private:
    struct Knob
    {
        juce::String label;
        juce::String paramId;
        int accent = 0;
        int column = 0;
        int row = 0;
        juce::Rectangle<int> cell;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (const char* paramId, const char* label, int accent, int column, int row);

    juce::String valueText (const Knob&) const;

    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintNoseCard (juce::Graphics&);
    void paintKnobCard (juce::Graphics&);
    void paintMeterCard (juce::Graphics&);
    void paintBaseplate (juce::Graphics&);

    void paintNose (juce::Graphics&, juce::Rectangle<int> area, float amount);

    void refreshPresetList();
    void choosePreset (int itemId);
    void promptSavePreset();
    void promptDeletePreset();
    void updatePresetButtons();

    void setPresetName (const juce::String& name);

    GiannisModeAudioProcessor& processor;
    GiannisLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> noseCardArea;
    juce::Rectangle<int> knobCardArea;
    juce::Rectangle<int> meterCardArea;
    juce::Rectangle<int> noseArea;

    juce::StringArray userPresets;
    juce::String presetName { "GIANNIS" };
    juce::String selectedUserPreset;
    bool edited = false;
    bool applyingPreset = false;
    int frame = 0;

    // Smoothed readouts, so the nose pumps instead of twitching.
    float subAmount = 0.0f;
    float inputAmount = 0.0f;
    float voiceAmount = 0.0f;
    float trackedHz = 0.0f;
    int trackedNote = -1;

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GiannisModeAudioProcessorEditor)
};
