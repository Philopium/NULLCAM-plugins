#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: matte black card, acid screen print, knurled metal knobs. It carries
// the two display fonts that travel inside the binary (Source/Fonts), so the panel is
// identical on every machine and in every host.
class PierLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PierLookAndFeel();

    juce::Font displayFont (float height) const;   // PIER-MODE, the shout
    juce::Font shadeFont (float height) const;     // the same, as a printed ghost
    juce::Font monoFont (float height, bool bold = false) const;

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

    // The four inks of the panel.
    static juce::Colour paper();
    static juce::Colour panel();
    static juce::Colour line();
    static juce::Colour ink();
    static juce::Colour cream();
    static juce::Colour dim();
    static juce::Colour acid();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PierLookAndFeel)
};

class NullcamChainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::AudioProcessorValueTreeState::Listener,
                                         private juce::AsyncUpdater
{
public:
    explicit NullcamChainAudioProcessorEditor (NullcamChainAudioProcessor&);
    ~NullcamChainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Unit { hz, hzTenth, khz, ms, percent };

    struct KnobSpec
    {
        const char* id;
        const char* label;
        Unit unit;
    };

    struct Knob
    {
        juce::String label;
        juce::String paramId;
        Unit unit = Unit::percent;
        juce::Rectangle<int> cell;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Stage
    {
        juce::String number;
        juce::String name;
        juce::String caption;
        bool showsBitDepth = false;
        juce::Rectangle<int> area;
        std::vector<int> knobIndices;
    };

    void addStage (const char* number, const char* name, const char* caption,
                   std::initializer_list<KnobSpec> specs, bool showsBitDepth = false);

    juce::String valueText (double value, Unit unit) const;
    double valueFromText (const juce::String& text, Unit unit) const;

    void makePaperTexture();
    void paintWordmarkGhost (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintStage (juce::Graphics&, const Stage&);
    void paintBitReadout (juce::Graphics&, const Stage&);
    void paintBaseplate (juce::Graphics&);
    void paintScrews (juce::Graphics&);

    void refreshPresetList();
    void choosePreset (int itemId);
    void promptSavePreset();
    void promptDeletePreset();
    void setPresetName (const juce::String& name);
    void updateEditedState();

    float currentValue (std::size_t index) const;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    NullcamChainAudioProcessor& processor;
    PierLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Stage> stages;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> presetPlateArea;
    juce::Rectangle<int> bitReadoutArea;

    juce::Image paperTexture;

    juce::StringArray userPresets;
    juce::String presetName { "MOTION" };
    std::array<float, 16> referenceValues {};
    bool edited = false;

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NullcamChainAudioProcessorEditor)
};
