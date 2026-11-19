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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float value);

    // The raw values of every parameter, resolved once. The audio thread only
    // reads them with an atomic load: no String, no allocation, no lock.
    struct ValuePointers
    {
        std::atomic<float>* delayTime  = nullptr;
        std::atomic<float>* sync       = nullptr;
        std::atomic<float>* feedback   = nullptr;
        std::atomic<float>* mix        = nullptr;
        std::atomic<float>* inputGain  = nullptr;
        std::atomic<float>* outputGain = nullptr;
        std::atomic<float>* hpfCutoff  = nullptr;
        std::atomic<float>* lpfCutoff  = nullptr;
        std::atomic<float>* drive      = nullptr;

        std::atomic<float>* stepEnabled[echomatrix::Engine::kNumSteps] = {};
        std::atomic<float>* stepLevel  [echomatrix::Engine::kNumSteps] = {};
        std::atomic<float>* stepPan    [echomatrix::Engine::kNumSteps] = {};
    };

    void resolveValuePointers();

    echomatrix::Engine engine;
    ValuePointers values;
    double hostSampleRate = 48000.0;
    double hostBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoMatrixModeAudioProcessor)
};
