#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MauriEngine.h"

#include <array>

namespace ids
{
inline constexpr const char* clock = "clock";       // 0 host, 1 input, 2 free
inline constexpr const char* timeSig = "timeSig";   // index into mauri::kTimeSigsInfo
inline constexpr const char* rate = "rate";         // index into mauri::kDivisionNames
inline constexpr const char* style = "style";       // index into mauri::kStyleNames
inline constexpr const char* slice = "slice";
inline constexpr const char* repeat = "repeat";
inline constexpr const char* reverse = "reverse";
inline constexpr const char* scatter = "scatter";
inline constexpr const char* dynPat = "dynPat";
inline constexpr const char* dynSlice = "dynSlice";
inline constexpr const char* dynRate = "dynRate";
inline constexpr const char* sens = "sens";
inline constexpr const char* tone = "tone";
inline constexpr const char* mix = "mix";
inline constexpr const char* output = "output";
} // namespace ids

class MauriModeAudioProcessor : public juce::AudioProcessor
{
public:
    MauriModeAudioProcessor();
    ~MauriModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MAURI-MODE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Factory rhythms: the same table that applies a preset also recognises it again
    // when a project is reopened.
    enum class Preset { mauri, odd78, ghost54, shred68 };
    void applyPreset (Preset preset);
    juce::String matchFactoryPreset() const;

    // User presets are one XML file each in the user application data folder.
    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    // Display only: a copy of the last captured second, and the transport the host
    // reports. Both are guarded so the audio thread never waits for the editor.
    bool copyScope (float* dest, int count) const;

    juce::AudioProcessorValueTreeState apvts;

    // Read straight from the audio thread's atomics: the editor polls these.
    const mauri::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }

    std::atomic<double> displayBpm { 120.0 };
    std::atomic<bool> displayPlaying { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float value);

    mauri::Slicer engine;

    mutable juce::SpinLock scopeLock;
    std::array<float, mauri::kScopePoints> scope {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MauriModeAudioProcessor)
};
