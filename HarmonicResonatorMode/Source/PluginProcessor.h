#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "HarmonicResonatorEngine.h"

namespace ids
{
    // output
    inline constexpr const char* inputGain   = "inputGain";
    inline constexpr const char* outputGain  = "outputGain";
    inline constexpr const char* mix         = "mix";
    inline constexpr const char* fundamental = "fundamental";

    // harmonics
    inline constexpr const char* drive     = "drive";
    inline constexpr const char* shape     = "shape";
    inline constexpr const char* evenOdd   = "evenOdd";
    inline constexpr const char* symmetry  = "symmetry";
    inline constexpr const char* density   = "density";
    inline constexpr const char* brightness = "brightness";

    // resonators
    inline constexpr const char* tune      = "tune";
    inline constexpr const char* tuning    = "tuning";
    inline constexpr const char* resonance = "resonance";
    inline constexpr const char* decay     = "decay";
    inline constexpr const char* spread    = "spread";
    inline constexpr const char* resGain   = "resGain";
    inline constexpr const char* align     = "align";

    // interaction
    inline constexpr const char* routing     = "routing";
    inline constexpr const char* interaction = "interaction";
    inline constexpr const char* feedback    = "feedback";
    inline constexpr const char* fbDrive     = "fbDrive";
    inline constexpr const char* lowCut      = "lowCut";
    inline constexpr const char* highCut     = "highCut";
    inline constexpr const char* source      = "source";
    inline constexpr const char* destination = "destination";

    // modulation
    inline constexpr const char* modMode  = "modMode";
    inline constexpr const char* modDest  = "modDest";
    inline constexpr const char* modRate  = "modRate";
    inline constexpr const char* modDepth = "modDepth";
    inline constexpr const char* envDepth = "envDepth";
} // namespace ids

class HarmonicResonatorModeAudioProcessor : public juce::AudioProcessor
{
public:
    HarmonicResonatorModeAudioProcessor();
    ~HarmonicResonatorModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HarmonicResonatorMode"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

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
        bellMetal = 0, tubeChoir, ironGong, glassHarmonics,
        subAnchor, wreckedRadio, resonantFold, pitchLock
    };

    static constexpr int kNumPresets = 8;

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

    const harmonicresonator::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float value);
    void resolveValuePointers();

    // The raw values of every parameter, resolved once. The audio thread only
    // reads them with an atomic load: no String, no allocation, no lock.
    struct ValuePointers
    {
        std::atomic<float>* inputGain   = nullptr;
        std::atomic<float>* outputGain  = nullptr;
        std::atomic<float>* mix         = nullptr;
        std::atomic<float>* fundamental = nullptr;

        std::atomic<float>* drive      = nullptr;
        std::atomic<float>* shape      = nullptr;
        std::atomic<float>* evenOdd    = nullptr;
        std::atomic<float>* symmetry   = nullptr;
        std::atomic<float>* density    = nullptr;
        std::atomic<float>* brightness = nullptr;

        std::atomic<float>* tune      = nullptr;
        std::atomic<float>* tuning    = nullptr;
        std::atomic<float>* resonance = nullptr;
        std::atomic<float>* decay     = nullptr;
        std::atomic<float>* spread    = nullptr;
        std::atomic<float>* resGain   = nullptr;
        std::atomic<float>* align     = nullptr;

        std::atomic<float>* routing     = nullptr;
        std::atomic<float>* interaction = nullptr;
        std::atomic<float>* feedback    = nullptr;
        std::atomic<float>* fbDrive     = nullptr;
        std::atomic<float>* lowCut      = nullptr;
        std::atomic<float>* highCut     = nullptr;
        std::atomic<float>* source      = nullptr;
        std::atomic<float>* destination = nullptr;

        std::atomic<float>* modMode  = nullptr;
        std::atomic<float>* modDest  = nullptr;
        std::atomic<float>* modRate  = nullptr;
        std::atomic<float>* modDepth = nullptr;
        std::atomic<float>* envDepth = nullptr;
    };

    harmonicresonator::Engine engine;
    ValuePointers values;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicResonatorModeAudioProcessor)
};
