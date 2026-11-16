#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "NullcamChain.h"

#include <vector>

namespace ids
{
inline constexpr const char* hpf = "hpf";
inline constexpr const char* lpf = "lpf";
inline constexpr const char* crush = "crush";
inline constexpr const char* delayMs = "delay";
inline constexpr const char* echo = "echo";
inline constexpr const char* feedback = "feedback";
inline constexpr const char* ringHz = "ringHz";
inline constexpr const char* ring = "ring";
inline constexpr const char* dry = "dry";
inline constexpr const char* output = "output";
} // namespace ids

class NullcamChainAudioProcessor : public juce::AudioProcessor
{
public:
    NullcamChainAudioProcessor();
    ~NullcamChainAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PIER-MODE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // factory settings lifted from the app's fxSnapshot(), see README
    enum class Preset { clean, motion, sort };
    void applyPreset (Preset preset);

    // The name of the factory preset the current knobs match, or an empty string.
    juce::String matchFactoryPreset() const;

    // User presets are one XML file each in the user application data folder, so
    // they survive a rebuild, sync with a backup and can be sent to someone else.
    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float value);

    std::vector<nullcam::Chain> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NullcamChainAudioProcessor)
};
