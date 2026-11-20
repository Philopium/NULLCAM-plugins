#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MemoryGranulatorEngine.h"

#include <array>

namespace param
{
    enum class Index
    {
        memoryLength = 0,
        captureEnable,
        freeze,
        captureArmed,
        clearTrigger,
        releaseTrigger,
        memoryMode,
        recLevel,
        stereoCapture,
        feedbackSource,
        feedbackAmount,
        feedbackStability,
        feedbackFilter,
        feedbackDamping,
        feedbackSaturation,
        feedbackDiffusion,
        feedbackPitch,

        grainMode,
        grainSize,
        density,
        overlap,
        bufferPosition,
        scanSpeed,
        spray,
        reverseProbability,
        grainEnvelope,
        grainGain,
        stereoSpread,
        stereoRandom,
        maxGrains,
        tempoSync,
        hostRate,

        pitchSource,
        midiChannel,
        releaseCC,
        notePriority,
        followMode,
        noteLow,
        noteHigh,
        trackRangeLow,
        trackRangeHigh,
        transposition,
        pitchSensitivity,
        confidenceThreshold,
        pitchAttack,
        pitchRelease,
        pitchSmoothing,
        hybridBlend,

        mapTranspose,
        mapSpeed,
        mapPosition,
        mapDensity,
        mapSize,
        mapRate,
        mapHarmonic,
        mapResonant,
        mapReverb,
        mapStereo,
        mapSmoothing,
        mapCurve,

        fundamental,
        octaveMix,
        harmonicLayers,
        intervalA,
        intervalB,
        harmonicSpread,
        quantize,
        scale,
        rootNote,
        tuning,
        detune,
        harmLevel1,
        harmLevel2,
        harmLevel3,
        harmLevel4,

        reverbMix,
        reverbDecay,
        roomSize,
        diffusion,
        damping,
        preDelay,
        width,
        spaceModulation,
        filterFreq,
        resonance,
        reverbFeedback,
        reverbFreeze,

        modSource,
        modDestination,
        modDepth,
        modSource2,
        modDestination2,
        modDepth2,
        modRate,

        dryWet,
        grainLevel,
        reverbLevel,
        inputGain,
        outputGain,

        count
    };

    inline constexpr int kNumParameters = (int) Index::count;
} // namespace param

namespace ids
{
    inline constexpr std::array<const char*, param::kNumParameters> names = {
        "memoryLength", "captureEnable", "freeze", "captureArmed", "clearTrigger",
        "releaseTrigger", "memoryMode", "recLevel", "stereoCapture", "feedbackSource",
        "feedbackAmount", "feedbackStability", "feedbackFilter", "feedbackDamping",
        "feedbackSaturation", "feedbackDiffusion", "feedbackPitch",

        "grainMode", "grainSize", "density", "overlap", "bufferPosition", "scanSpeed",
        "spray", "reverseProbability", "grainEnvelope", "grainGain", "stereoSpread",
        "stereoRandom", "maxGrains", "tempoSync", "hostRate",

        "pitchSource", "midiChannel", "releaseCC", "notePriority", "followMode",
        "noteLow", "noteHigh", "trackRangeLow", "trackRangeHigh", "transposition",
        "pitchSensitivity", "confidenceThreshold", "pitchAttack", "pitchRelease",
        "pitchSmoothing", "hybridBlend",

        "mapTranspose", "mapSpeed", "mapPosition", "mapDensity", "mapSize", "mapRate",
        "mapHarmonic", "mapResonant", "mapReverb", "mapStereo", "mapSmoothing", "mapCurve",

        "fundamental", "octaveMix", "harmonicLayers", "intervalA", "intervalB",
        "harmonicSpread", "quantize", "scale", "rootNote", "tuning", "detune",
        "harmLevel1", "harmLevel2", "harmLevel3", "harmLevel4",

        "reverbMix", "reverbDecay", "roomSize", "diffusion", "damping", "preDelay",
        "width", "spaceModulation", "filterFreq", "resonance", "reverbFeedback",
        "reverbFreeze",

        "modSource", "modDestination", "modDepth", "modSource2", "modDestination2",
        "modDepth2", "modRate",

        "dryWet", "grainLevel", "reverbLevel", "inputGain", "outputGain"
    };

    inline constexpr const char* memoryLength = names[(int) param::Index::memoryLength];
    inline constexpr const char* captureEnable = names[(int) param::Index::captureEnable];
    inline constexpr const char* freeze = names[(int) param::Index::freeze];
    inline constexpr const char* captureArmed = names[(int) param::Index::captureArmed];
    inline constexpr const char* clearTrigger = names[(int) param::Index::clearTrigger];
    inline constexpr const char* releaseTrigger = names[(int) param::Index::releaseTrigger];
    inline constexpr const char* memoryMode = names[(int) param::Index::memoryMode];
    inline constexpr const char* recLevel = names[(int) param::Index::recLevel];
    inline constexpr const char* stereoCapture = names[(int) param::Index::stereoCapture];
    inline constexpr const char* feedbackSource = names[(int) param::Index::feedbackSource];
    inline constexpr const char* feedbackAmount = names[(int) param::Index::feedbackAmount];
    inline constexpr const char* feedbackStability = names[(int) param::Index::feedbackStability];
    inline constexpr const char* feedbackFilter = names[(int) param::Index::feedbackFilter];
    inline constexpr const char* feedbackDamping = names[(int) param::Index::feedbackDamping];
    inline constexpr const char* feedbackSaturation = names[(int) param::Index::feedbackSaturation];
    inline constexpr const char* feedbackDiffusion = names[(int) param::Index::feedbackDiffusion];
    inline constexpr const char* feedbackPitch = names[(int) param::Index::feedbackPitch];

    inline constexpr const char* grainMode = names[(int) param::Index::grainMode];
    inline constexpr const char* grainSize = names[(int) param::Index::grainSize];
    inline constexpr const char* density = names[(int) param::Index::density];
    inline constexpr const char* overlap = names[(int) param::Index::overlap];
    inline constexpr const char* bufferPosition = names[(int) param::Index::bufferPosition];
    inline constexpr const char* scanSpeed = names[(int) param::Index::scanSpeed];
    inline constexpr const char* spray = names[(int) param::Index::spray];
    inline constexpr const char* reverseProbability = names[(int) param::Index::reverseProbability];
    inline constexpr const char* grainEnvelope = names[(int) param::Index::grainEnvelope];
    inline constexpr const char* grainGain = names[(int) param::Index::grainGain];
    inline constexpr const char* stereoSpread = names[(int) param::Index::stereoSpread];
    inline constexpr const char* stereoRandom = names[(int) param::Index::stereoRandom];
    inline constexpr const char* maxGrains = names[(int) param::Index::maxGrains];
    inline constexpr const char* tempoSync = names[(int) param::Index::tempoSync];
    inline constexpr const char* hostRate = names[(int) param::Index::hostRate];

    inline constexpr const char* pitchSource = names[(int) param::Index::pitchSource];
    inline constexpr const char* midiChannel = names[(int) param::Index::midiChannel];
    inline constexpr const char* releaseCC = names[(int) param::Index::releaseCC];
    inline constexpr const char* notePriority = names[(int) param::Index::notePriority];
    inline constexpr const char* followMode = names[(int) param::Index::followMode];
    inline constexpr const char* noteLow = names[(int) param::Index::noteLow];
    inline constexpr const char* noteHigh = names[(int) param::Index::noteHigh];
    inline constexpr const char* trackRangeLow = names[(int) param::Index::trackRangeLow];
    inline constexpr const char* trackRangeHigh = names[(int) param::Index::trackRangeHigh];
    inline constexpr const char* transposition = names[(int) param::Index::transposition];
    inline constexpr const char* pitchSensitivity = names[(int) param::Index::pitchSensitivity];
    inline constexpr const char* confidenceThreshold = names[(int) param::Index::confidenceThreshold];
    inline constexpr const char* pitchAttack = names[(int) param::Index::pitchAttack];
    inline constexpr const char* pitchRelease = names[(int) param::Index::pitchRelease];
    inline constexpr const char* pitchSmoothing = names[(int) param::Index::pitchSmoothing];
    inline constexpr const char* hybridBlend = names[(int) param::Index::hybridBlend];

    inline constexpr const char* mapTranspose = names[(int) param::Index::mapTranspose];
    inline constexpr const char* mapSpeed = names[(int) param::Index::mapSpeed];
    inline constexpr const char* mapPosition = names[(int) param::Index::mapPosition];
    inline constexpr const char* mapDensity = names[(int) param::Index::mapDensity];
    inline constexpr const char* mapSize = names[(int) param::Index::mapSize];
    inline constexpr const char* mapRate = names[(int) param::Index::mapRate];
    inline constexpr const char* mapHarmonic = names[(int) param::Index::mapHarmonic];
    inline constexpr const char* mapResonant = names[(int) param::Index::mapResonant];
    inline constexpr const char* mapReverb = names[(int) param::Index::mapReverb];
    inline constexpr const char* mapStereo = names[(int) param::Index::mapStereo];
    inline constexpr const char* mapSmoothing = names[(int) param::Index::mapSmoothing];
    inline constexpr const char* mapCurve = names[(int) param::Index::mapCurve];

    inline constexpr const char* fundamental = names[(int) param::Index::fundamental];
    inline constexpr const char* octaveMix = names[(int) param::Index::octaveMix];
    inline constexpr const char* harmonicLayers = names[(int) param::Index::harmonicLayers];
    inline constexpr const char* intervalA = names[(int) param::Index::intervalA];
    inline constexpr const char* intervalB = names[(int) param::Index::intervalB];
    inline constexpr const char* harmonicSpread = names[(int) param::Index::harmonicSpread];
    inline constexpr const char* quantize = names[(int) param::Index::quantize];
    inline constexpr const char* scale = names[(int) param::Index::scale];
    inline constexpr const char* rootNote = names[(int) param::Index::rootNote];
    inline constexpr const char* tuning = names[(int) param::Index::tuning];
    inline constexpr const char* detune = names[(int) param::Index::detune];
    inline constexpr const char* harmLevel1 = names[(int) param::Index::harmLevel1];
    inline constexpr const char* harmLevel2 = names[(int) param::Index::harmLevel2];
    inline constexpr const char* harmLevel3 = names[(int) param::Index::harmLevel3];
    inline constexpr const char* harmLevel4 = names[(int) param::Index::harmLevel4];

    inline constexpr const char* reverbMix = names[(int) param::Index::reverbMix];
    inline constexpr const char* reverbDecay = names[(int) param::Index::reverbDecay];
    inline constexpr const char* roomSize = names[(int) param::Index::roomSize];
    inline constexpr const char* diffusion = names[(int) param::Index::diffusion];
    inline constexpr const char* damping = names[(int) param::Index::damping];
    inline constexpr const char* preDelay = names[(int) param::Index::preDelay];
    inline constexpr const char* width = names[(int) param::Index::width];
    inline constexpr const char* spaceModulation = names[(int) param::Index::spaceModulation];
    inline constexpr const char* filterFreq = names[(int) param::Index::filterFreq];
    inline constexpr const char* resonance = names[(int) param::Index::resonance];
    inline constexpr const char* reverbFeedback = names[(int) param::Index::reverbFeedback];
    inline constexpr const char* reverbFreeze = names[(int) param::Index::reverbFreeze];

    inline constexpr const char* modSource = names[(int) param::Index::modSource];
    inline constexpr const char* modDestination = names[(int) param::Index::modDestination];
    inline constexpr const char* modDepth = names[(int) param::Index::modDepth];
    inline constexpr const char* modSource2 = names[(int) param::Index::modSource2];
    inline constexpr const char* modDestination2 = names[(int) param::Index::modDestination2];
    inline constexpr const char* modDepth2 = names[(int) param::Index::modDepth2];
    inline constexpr const char* modRate = names[(int) param::Index::modRate];

    inline constexpr const char* dryWet = names[(int) param::Index::dryWet];
    inline constexpr const char* grainLevel = names[(int) param::Index::grainLevel];
    inline constexpr const char* reverbLevel = names[(int) param::Index::reverbLevel];
    inline constexpr const char* inputGain = names[(int) param::Index::inputGain];
    inline constexpr const char* outputGain = names[(int) param::Index::outputGain];
} // namespace ids

class MemoryGranulatorModeAudioProcessor : public juce::AudioProcessor
{
public:
    MemoryGranulatorModeAudioProcessor();
    ~MemoryGranulatorModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MemoryGranulatorMode"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    enum class Preset
    {
        memoryBloom = 0, chordEcho, frozenVerb, overdubCloud,
        harmonicWeather, polyRhythm, resonantCircuit, darkArchive
    };

    static constexpr int kNumPresets = 8;

    void applyPreset (Preset preset);
    juce::String matchFactoryPreset() const;
    static juce::String getFactoryPresetName (Preset preset);

    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState apvts;

    const memorygranulator::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }
    float getParameterValue (param::Index index) const noexcept;
    void setParameterValue (param::Index index, float realValue);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void resolveValuePointers();

    std::array<std::atomic<float>*, param::kNumParameters> values {};
    memorygranulator::Engine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryGranulatorModeAudioProcessor)
};
