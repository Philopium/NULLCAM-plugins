#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

// The faceplate: dark steel, a hot orange blade and a wordmark cut into slices.
// The two display fonts travel inside the binary (Source/Fonts), so the panel is the
// same on every machine and in every host.
class MauriLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MauriLookAndFeel();

    juce::Font displayFont (float height) const;   // MAURI-MODE, the shout
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

    // The inks of the panel.
    static juce::Colour steel();
    static juce::Colour card();
    static juce::Colour line();
    static juce::Colour ink();
    static juce::Colour dim();
    static juce::Colour blade();   // hot orange
    static juce::Colour spark();   // pale cream

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MauriLookAndFeel)
};

class MauriModeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::AudioProcessorValueTreeState::Listener,
                                      private juce::Timer
{
public:
    explicit MauriModeAudioProcessorEditor (MauriModeAudioProcessor&);
    ~MauriModeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Pulls the live data (scope, playhead, dynamics) into the panel. The timer calls
    // it while the editor is open; the offscreen render tool calls it directly.
    void pullLiveData();

private:
    enum class Unit { percent, clock, timeSig, rate, style };

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

    struct Card
    {
        juce::String number;
        juce::String name;
        juce::String caption;
        int columns = 1;
        bool isScope = false;
        juce::Rectangle<int> area;
        std::vector<int> knobIndices;
    };

    void addCard (const char* number, const char* name, const char* caption,
                  int columns, std::initializer_list<KnobSpec> specs);

    juce::String valueText (double value, Unit unit) const;
    double valueFromText (const juce::String& text, Unit unit) const;

    void makeSteelTexture();
    void bakeWordmarks();

    void paintBackground (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPresetBar (juce::Graphics&);
    void paintCard (juce::Graphics&, const Card&);
    void paintScope (juce::Graphics&, const Card&);
    void paintGrid (juce::Graphics&);
    void paintLegalNotice (juce::Graphics&);

    void paintSlicedImage (juce::Graphics&, const juce::Image&, juce::Rectangle<int>,
                           int bands, float opacity, int seed);

    void refreshPresetList();
    void choosePreset (int itemId);
    void promptSavePreset();
    void promptDeletePreset();
    void setPresetName (const juce::String& name);
    void updateEditedState();

    float currentValue (std::size_t index) const;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    MauriModeAudioProcessor& processor;
    MauriLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Card> cards;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DEL" };

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> presetBarArea;
    juce::Rectangle<int> presetPlateArea;
    juce::Rectangle<int> gridArea;
    juce::Rectangle<int> screenArea;
    juce::Rectangle<int> meterArea;
    juce::Rectangle<int> readoutArea;

    juce::Image steelTexture;
    juce::Image wordmarkImage;
    juce::Image ghostImage;

    std::array<float, mauri::kScopePoints> scope {};
    int lastOnsets = 0;
    float onsetFlash = 0.0f;

    juce::StringArray userPresets;
    juce::String presetName { "MAURI" };
    std::array<float, 16> referenceValues {};
    bool edited = false;

    juce::Component::SafePointer<juce::AlertWindow> activeDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MauriModeAudioProcessorEditor)
};
