#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: a gummy rainbow toy for grown ups. Round corners, thick glossy
// edges, a fat round typeface for the shout and a printed-mono one for the small
// print. The three fonts travel inside the binary (Source/Fonts), so the panel is
// the same on every machine and in every host.
class SpampiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpampiLookAndFeel();

    juce::Font gummyFont (float height) const;    // SPAMPI-MODE, the shout
    juce::Font labelFont (float height) const;    // Rowdies Bold, the names
    juce::Font bodyFont (float height) const;     // Rowdies Regular, the small print
    juce::Font numberFont (float height) const;   // Titan One, the big values

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

    // The inks of the panel.
    static juce::Colour base();
    static juce::Colour card();
    static juce::Colour cardTop();
    static juce::Colour line();
    static juce::Colour ink();
    static juce::Colour dim();
    static juce::Colour accent (int index);   // the rainbow, one colour per knob

    // Six stops, left to right: the whole machine in one gradient.
    static juce::ColourGradient rainbow (juce::Rectangle<float> area, bool vertical = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpampiLookAndFeel)
};

class SpampiModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::AudioProcessorValueTreeState::Listener,
                                       private juce::Timer
{
public:
    explicit SpampiModeAudioProcessorEditor (SpampiModeAudioProcessor&);
    ~SpampiModeAudioProcessorEditor() override;

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
        bool isNote = false;     // ROOT is printed as a note name
        bool isStepped = false;  // TRIGGER prints LOOP / GATE / TRIG
        juce::Rectangle<int> cell;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (const char* paramId, const char* label, int accent,
                  bool isNote, bool isStepped);

    void paintBackground (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintRecording (juce::Graphics&);
    void paintPitch (juce::Graphics&);
    void paintKnobs (juce::Graphics&);
    void paintLegalNotice (juce::Graphics&);

    juce::String valueText (const Knob&) const;
    static juce::String noteName (int midi);

    void refreshPresetList();
    void refreshSampleList();
    void choosePreset (int itemId);
    void chooseSample (int itemId);
    void promptSavePreset();
    void promptDeletePreset();
    void loadSampleFile();
    void setPresetName (const juce::String& name);
    void updateEditedState();

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    SpampiModeAudioProcessor& processor;
    SpampiLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;

    juce::ComboBox presetBox;
    juce::ComboBox sampleBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };
    juce::TextButton loadButton { "LOAD SAMPLE" };

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> recordingArea;
    juce::Rectangle<int> pitchArea;
    juce::Rectangle<int> gridArea;

    // Live state, refreshed from the audio thread's atomics.
    std::array<float, 240> pitchTrace {};
    std::array<float, 240> waveform {};
    float sampleStartFraction = 0.0f;   // where the recording is cut, 0..1
    float noteValue = -1.0f;
    float frequency = 0.0f;
    float confidence = 0.0f;
    float inputLevel = 0.0f;
    float playbackRate = 1.0f;
    float octaveJump = 0.0f;

    juce::String currentSampleName;
    int currentSampleIndex = -1;
    int currentRoot = -1;

    juce::StringArray userPresets;
    juce::String presetName { "MONKEY" };
    bool edited = false;
    int knownSampleCount = 0;
    int frame = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpampiModeAudioProcessorEditor)
};
