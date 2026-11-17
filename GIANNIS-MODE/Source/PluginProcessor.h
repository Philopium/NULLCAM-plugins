#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "GiannisEngine.h"

namespace ids
{
    inline constexpr const char* sub   = "sub";
    inline constexpr const char* doom  = "doom";
    inline constexpr const char* oct   = "oct";
    inline constexpr const char* fuzz  = "fuzz";
    inline constexpr const char* tone  = "tone";
    inline constexpr const char* level = "level";
} // namespace ids

class GiannisModeAudioProcessor : public juce::AudioProcessor
{
public:
    GiannisModeAudioProcessor();
    ~GiannisModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GIANNIS-MODE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The factory voices: from a pure sub to the full fuzz octave stack.
    enum class Preset
    {
        giannis = 0, subZero, dubstep, doom, fuzzOct, synthBass,
        cleanOct, quake, wub, organ, tapeOct, subsonic
    };

    static constexpr int kNumPresets = 12;

    void applyPreset (Preset preset);
    juce::String matchFactoryPreset() const;
    static juce::String getFactoryPresetName (Preset preset);

    // User presets: one XML file each, in the user application data folder.
    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState apvts;

    const giannis::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float value);

    giannis::Engine engine;
    double hostSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GiannisModeAudioProcessor)
};
