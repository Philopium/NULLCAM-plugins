#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "EchoMatrixEngine.h"

namespace ids
{
    inline constexpr const char* delayTime = "delayTime";
    inline constexpr const char* sync      = "sync";
    inline constexpr const char* feedback  = "feedback";
    inline constexpr const char* mix       = "mix";
    inline constexpr const char* inputGain = "inputGain";
    inline constexpr const char* outputGain = "outputGain";
    inline constexpr const char* hpfCutoff = "hpfCutoff";
    inline constexpr const char* lpfCutoff = "lpfCutoff";
    inline constexpr const char* drive     = "drive";

    inline constexpr const char* stepEnabled = "stepEnabled";
    inline constexpr const char* stepLevel   = "stepLevel";
    inline constexpr const char* stepPan     = "stepPan";
} // namespace ids

class EchoMatrixModeAudioProcessor : public juce::AudioProcessor
{
public:
    EchoMatrixModeAudioProcessor();
    ~EchoMatrixModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EchoMatrixMode"; }
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

    // Factory presets.
    enum class Preset
    {
        dottedEighth = 0, pingPong, dub, reverseWash, glitchTap,
        ambientWash, slapback, chaotic
    };

    static constexpr int kNumPresets = 8;
    static constexpr int kNumSteps = 16;

    // The APVTS id of a per-step parameter, for example "stepLevel7".
    static juce::String stepParamId (const char* kind, int step);

    void applyPreset (Preset preset);
    juce::String matchFactoryPreset() const;
    static juce::String getFactoryPresetName (Preset preset);

    // User presets: one XML file each.
    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState apvts;

    const echomatrix::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }

    // Called by the editor when the host provides a new BPM (tempo sync).
    void setHostBpm (double bpm) noexcept { engine.setBpm (bpm); }
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float value);

    echomatrix::Engine engine;
    double hostSampleRate = 48000.0;
    double hostBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoMatrixModeAudioProcessor)
};
