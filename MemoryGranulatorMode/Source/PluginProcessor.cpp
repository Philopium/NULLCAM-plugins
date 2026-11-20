#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace
{
enum class SpecKind { floatValue, boolean, choice };

struct ParameterSpec
{
    param::Index index = param::Index::count;
    const char* name = "";
    SpecKind kind = SpecKind::floatValue;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float defaultValue = 0.0f;
    juce::StringArray choices;
};

const juce::StringArray& memoryModeNames()
{
    static const juce::StringArray names { "LIVE", "FREEZE", "CAPTURE+RELEASE", "OVERDUB", "HOLD" };
    return names;
}

const juce::StringArray& feedbackSourceNames()
{
    static const juce::StringArray names { "INPUT", "GRANULAR", "REVERB", "COMBINED", "FILTERED" };
    return names;
}

const juce::StringArray& grainModeNames()
{
    static const juce::StringArray names { "UNIFORM", "RANDOM", "CLUSTER", "RHYTHMIC",
                                            "CLOUD", "SPARSE", "SCAN", "PITCH-AWARE" };
    return names;
}

const juce::StringArray& envelopeNames()
{
    static const juce::StringArray names { "HANN", "EXP", "TRIANGLE", "PLATEAU" };
    return names;
}

const juce::StringArray& pitchSourceNames()
{
    static const juce::StringArray names { "MIDI", "AUDIO", "HYBRID" };
    return names;
}

const juce::StringArray& midiChannelNames()
{
    static const juce::StringArray names {
        "OMNI", "1", "2", "3", "4", "5", "6", "7", "8",
        "9", "10", "11", "12", "13", "14", "15", "16"
    };
    return names;
}

const juce::StringArray& notePriorityNames()
{
    static const juce::StringArray names { "LATEST", "LOWEST", "HIGHEST", "VELOCITY" };
    return names;
}

const juce::StringArray& followModeNames()
{
    static const juce::StringArray names { "FOLLOW", "HARMONIC", "INTERVAL", "RESONANT", "LOOSE", "FREE" };
    return names;
}

const juce::StringArray& mapCurveNames()
{
    static const juce::StringArray names { "LINEAR", "EXP", "LOG" };
    return names;
}

const juce::StringArray& scaleNames()
{
    static const juce::StringArray names { "CHROMATIC", "MAJOR", "MINOR", "HARM MINOR",
                                            "PENTA MIN", "PENTA MAJ", "DORIAN", "MIXOLYDIAN",
                                            "LYDIAN", "WHOLE TONE", "PHRYGIAN", "BLUES" };
    return names;
}

const juce::StringArray& modSourceNames()
{
    static const juce::StringArray names { "OFF", "LFO", "ENVELOPE", "INPUT", "VELOCITY",
                                            "AFTERTOUCH", "PITCH", "RANDOM", "S&H", "TEMPO",
                                            "FEEDBACK" };
    return names;
}

const juce::StringArray& modDestinationNames()
{
    static const juce::StringArray names { "SIZE", "DENSITY", "POSITION", "RATE", "SPEED",
                                            "PITCH", "SPREAD", "DECAY", "DIFFUSION",
                                            "FEEDBACK", "FILTER", "RESONANCE" };
    return names;
}

std::vector<ParameterSpec> makeParameterSpecs()
{
    std::vector<ParameterSpec> specs;
    specs.reserve ((std::size_t) param::kNumParameters);

    auto addFloat = [&specs] (param::Index index, const char* name,
                               float minimum, float maximum, float defaultValue)
    {
        specs.push_back ({ index, name, SpecKind::floatValue, minimum, maximum, defaultValue, {} });
    };

    auto addBool = [&specs] (param::Index index, const char* name, bool defaultValue)
    {
        specs.push_back ({ index, name, SpecKind::boolean, 0.0f, 1.0f,
                           defaultValue ? 1.0f : 0.0f, {} });
    };

    auto addChoice = [&specs] (param::Index index, const char* name,
                                const juce::StringArray& choices, int defaultValue)
    {
        specs.push_back ({ index, name, SpecKind::choice, 0.0f,
                           (float) (choices.size() - 1), (float) defaultValue, choices });
    };

    addFloat (param::Index::memoryLength, "MEMORY LENGTH", 0.25f, 30.0f, 4.0f);
    addBool  (param::Index::captureEnable, "CAPTURE", true);
    addBool  (param::Index::freeze, "FREEZE", false);
    addBool  (param::Index::captureArmed, "ARM CAPTURE", false);
    addBool  (param::Index::clearTrigger, "CLEAR", false);
    addBool  (param::Index::releaseTrigger, "RELEASE", false);
    addChoice (param::Index::memoryMode, "MEMORY MODE", memoryModeNames(), 0);
    addFloat (param::Index::recLevel, "REC LEVEL", 0.0f, 2.0f, 1.0f);
    addBool  (param::Index::stereoCapture, "STEREO CAPTURE", true);
    addChoice (param::Index::feedbackSource, "FEEDBACK SOURCE", feedbackSourceNames(), 1);
    addFloat (param::Index::feedbackAmount, "MEMORY FEEDBACK", 0.0f, 0.95f, 0.0f);
    addFloat (param::Index::feedbackStability, "FB STABILITY", 0.0f, 1.0f, 0.65f);
    addFloat (param::Index::feedbackFilter, "FB FILTER", 100.0f, 18000.0f, 5000.0f);
    addFloat (param::Index::feedbackDamping, "FB DAMPING", 0.0f, 1.0f, 0.35f);
    addFloat (param::Index::feedbackSaturation, "FB SATURATION", 0.0f, 1.0f, 0.20f);
    addFloat (param::Index::feedbackDiffusion, "FB DIFFUSION", 0.0f, 1.0f, 0.25f);
    addFloat (param::Index::feedbackPitch, "FB PITCH", -24.0f, 24.0f, 0.0f);

    addChoice (param::Index::grainMode, "GRAIN MODE", grainModeNames(), 1);
    addFloat (param::Index::grainSize, "GRAIN SIZE", 0.005f, 2.0f, 0.12f);
    addFloat (param::Index::density, "DENSITY", 0.1f, 80.0f, 8.0f);
    addFloat (param::Index::overlap, "OVERLAP", 0.1f, 4.0f, 1.20f);
    addFloat (param::Index::bufferPosition, "POSITION", 0.0f, 1.0f, 0.50f);
    addFloat (param::Index::scanSpeed, "SCAN SPEED", -2.0f, 2.0f, 0.25f);
    addFloat (param::Index::spray, "SPRAY", 0.0f, 1.0f, 0.25f);
    addFloat (param::Index::reverseProbability, "REVERSE", 0.0f, 1.0f, 0.15f);
    addChoice (param::Index::grainEnvelope, "ENVELOPE", envelopeNames(), 0);
    addFloat (param::Index::grainGain, "GRAIN GAIN", 0.0f, 1.5f, 0.55f);
    addFloat (param::Index::stereoSpread, "STEREO SPREAD", 0.0f, 1.0f, 0.70f);
    addFloat (param::Index::stereoRandom, "PAN RANDOM", 0.0f, 1.0f, 0.35f);
    addFloat (param::Index::maxGrains, "MAX GRAINS", 1.0f, 48.0f, 24.0f);
    addBool  (param::Index::tempoSync, "TEMPO SYNC", false);
    addFloat (param::Index::hostRate, "SCAN MULTIPLIER", 0.25f, 4.0f, 1.0f);

    addChoice (param::Index::pitchSource, "PITCH SOURCE", pitchSourceNames(), 2);
    addChoice (param::Index::midiChannel, "MIDI CHANNEL", midiChannelNames(), 0);
    addFloat (param::Index::releaseCC, "RELEASE CC", 0.0f, 127.0f, 64.0f);
    addChoice (param::Index::notePriority, "NOTE PRIORITY", notePriorityNames(), 0);
    addChoice (param::Index::followMode, "FOLLOW MODE", followModeNames(), 0);
    addFloat (param::Index::noteLow, "NOTE LOW", 12.0f, 96.0f, 36.0f);
    addFloat (param::Index::noteHigh, "NOTE HIGH", 24.0f, 127.0f, 96.0f);
    addFloat (param::Index::trackRangeLow, "TRACK LOW", 20.0f, 500.0f, 40.0f);
    addFloat (param::Index::trackRangeHigh, "TRACK HIGH", 100.0f, 5000.0f, 1600.0f);
    addFloat (param::Index::transposition, "TRANSPOSE", -48.0f, 48.0f, 0.0f);
    addFloat (param::Index::pitchSensitivity, "SENSITIVITY", 0.0f, 2.0f, 1.0f);
    addFloat (param::Index::confidenceThreshold, "CONFIDENCE", 0.0f, 1.0f, 0.55f);
    addFloat (param::Index::pitchAttack, "PITCH ATTACK", 1.0f, 200.0f, 20.0f);
    addFloat (param::Index::pitchRelease, "PITCH RELEASE", 10.0f, 1000.0f, 180.0f);
    addFloat (param::Index::pitchSmoothing, "PITCH SMOOTH", 0.0f, 500.0f, 40.0f);
    addFloat (param::Index::hybridBlend, "HYBRID BLEND", 0.0f, 1.0f, 0.50f);

    addFloat (param::Index::mapTranspose, "MAP TRANSPOSE", -1.0f, 1.0f, 0.35f);
    addFloat (param::Index::mapSpeed, "MAP SPEED", -1.0f, 1.0f, 0.15f);
    addFloat (param::Index::mapPosition, "MAP POSITION", -1.0f, 1.0f, 0.20f);
    addFloat (param::Index::mapDensity, "MAP DENSITY", -1.0f, 1.0f, 0.15f);
    addFloat (param::Index::mapSize, "MAP SIZE", -1.0f, 1.0f, 0.25f);
    addFloat (param::Index::mapRate, "MAP RATE", -1.0f, 1.0f, 0.0f);
    addFloat (param::Index::mapHarmonic, "MAP HARMONIC", -1.0f, 1.0f, 0.0f);
    addFloat (param::Index::mapResonant, "MAP RESONANT", -1.0f, 1.0f, 0.0f);
    addFloat (param::Index::mapReverb, "MAP REVERB", -1.0f, 1.0f, 0.0f);
    addFloat (param::Index::mapStereo, "MAP STEREO", -1.0f, 1.0f, 0.15f);
    addFloat (param::Index::mapSmoothing, "MAP SMOOTH", 0.0f, 500.0f, 80.0f);
    addChoice (param::Index::mapCurve, "MAP CURVE", mapCurveNames(), 0);

    addFloat (param::Index::fundamental, "FUNDAMENTAL", 0.0f, 1.0f, 0.45f);
    addFloat (param::Index::octaveMix, "OCTAVE MIX", -1.0f, 1.0f, 0.25f);
    addFloat (param::Index::harmonicLayers, "HARMONIC LAYERS", 1.0f, 4.0f, 2.0f);
    addFloat (param::Index::intervalA, "INTERVAL A", -24.0f, 24.0f, 7.0f);
    addFloat (param::Index::intervalB, "INTERVAL B", -24.0f, 24.0f, 12.0f);
    addFloat (param::Index::harmonicSpread, "HARMONIC SPREAD", 0.0f, 1.0f, 0.50f);
    addBool  (param::Index::quantize, "QUANTIZE", false);
    addChoice (param::Index::scale, "SCALE", scaleNames(), 0);
    addFloat (param::Index::rootNote, "ROOT", 0.0f, 11.0f, 0.0f);
    addFloat (param::Index::tuning, "TUNING", -100.0f, 100.0f, 0.0f);
    addFloat (param::Index::detune, "DETUNE", -50.0f, 50.0f, 4.0f);
    addFloat (param::Index::harmLevel1, "LEVEL 1", 0.0f, 1.0f, 1.00f);
    addFloat (param::Index::harmLevel2, "LEVEL 2", 0.0f, 1.0f, 0.55f);
    addFloat (param::Index::harmLevel3, "LEVEL 3", 0.0f, 1.0f, 0.30f);
    addFloat (param::Index::harmLevel4, "LEVEL 4", 0.0f, 1.0f, 0.15f);

    addFloat (param::Index::reverbMix, "REVERB MIX", 0.0f, 1.0f, 0.35f);
    addFloat (param::Index::reverbDecay, "DECAY", 0.1f, 12.0f, 2.80f);
    addFloat (param::Index::roomSize, "ROOM SIZE", 0.0f, 1.0f, 0.55f);
    addFloat (param::Index::diffusion, "DIFFUSION", 0.1f, 1.0f, 0.65f);
    addFloat (param::Index::damping, "DAMPING", 0.0f, 1.0f, 0.30f);
    addFloat (param::Index::preDelay, "PRE-DELAY", 0.0f, 0.25f, 0.015f);
    addFloat (param::Index::width, "WIDTH", 0.0f, 1.0f, 0.80f);
    addFloat (param::Index::spaceModulation, "MODULATION", 0.0f, 1.0f, 0.35f);
    addFloat (param::Index::filterFreq, "FILTER", 100.0f, 18000.0f, 8000.0f);
    addFloat (param::Index::resonance, "RESONANCE", 0.0f, 1.0f, 0.20f);
    addFloat (param::Index::reverbFeedback, "REVERB FEEDBACK", 0.0f, 0.95f, 0.20f);
    addBool  (param::Index::reverbFreeze, "FREEZE SPACE", false);

    addChoice (param::Index::modSource, "MOD SOURCE", modSourceNames(), 1);
    addChoice (param::Index::modDestination, "MOD DEST", modDestinationNames(), 1);
    addFloat (param::Index::modDepth, "MOD DEPTH", -1.0f, 1.0f, 0.0f);
    addChoice (param::Index::modSource2, "MOD SOURCE 2", modSourceNames(), 0);
    addChoice (param::Index::modDestination2, "MOD DEST 2", modDestinationNames(), 0);
    addFloat (param::Index::modDepth2, "MOD DEPTH 2", -1.0f, 1.0f, 0.0f);
    addFloat (param::Index::modRate, "MOD RATE", 0.02f, 20.0f, 0.40f);

    addFloat (param::Index::dryWet, "DRY / WET", 0.0f, 1.0f, 0.35f);
    addFloat (param::Index::grainLevel, "GRAIN LEVEL", 0.0f, 1.5f, 0.80f);
    addFloat (param::Index::reverbLevel, "REVERB LEVEL", 0.0f, 1.5f, 0.70f);
    addFloat (param::Index::inputGain, "INPUT", 0.0f, 2.0f, 1.0f);
    addFloat (param::Index::outputGain, "OUTPUT", 0.0f, 2.0f, 0.80f);

    jassert (specs.size() == (std::size_t) param::kNumParameters);

    for (int i = 0; i < param::kNumParameters; ++i)
        jassert ((int) specs[(std::size_t) i].index == i);

    return specs;
}

std::vector<float> defaultValues (const std::vector<ParameterSpec>& specs)
{
    std::vector<float> values;
    values.reserve (specs.size());

    for (const auto& spec : specs)
        values.push_back (spec.defaultValue);

    return values;
}

using Override = std::pair<param::Index, float>;

std::vector<Override> presetOverrides (MemoryGranulatorModeAudioProcessor::Preset preset)
{
    switch (preset)
    {
        case MemoryGranulatorModeAudioProcessor::Preset::memoryBloom:
            break;

        case MemoryGranulatorModeAudioProcessor::Preset::chordEcho:
            return {
                { param::Index::pitchSource, 0.0f },
                { param::Index::grainMode, 1.0f },
                { param::Index::grainSize, 0.18f },
                { param::Index::density, 12.0f },
                { param::Index::overlap, 1.80f },
                { param::Index::feedbackAmount, 0.18f },
                { param::Index::harmonicLayers, 3.0f },
                { param::Index::reverbMix, 0.48f },
                { param::Index::reverbDecay, 3.40f },
                { param::Index::mapTranspose, 0.55f },
                { param::Index::mapStereo, 0.35f },
                { param::Index::dryWet, 0.28f },
                { param::Index::grainLevel, 0.90f },
                { param::Index::reverbLevel, 0.85f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::frozenVerb:
            return {
                { param::Index::memoryMode, 1.0f },
                { param::Index::captureEnable, 0.0f },
                { param::Index::grainMode, 6.0f },
                { param::Index::grainSize, 0.35f },
                { param::Index::density, 5.0f },
                { param::Index::scanSpeed, 0.08f },
                { param::Index::spray, 0.08f },
                { param::Index::reverseProbability, 0.30f },
                { param::Index::feedbackAmount, 0.35f },
                { param::Index::feedbackSource, 2.0f },
                { param::Index::reverbMix, 0.65f },
                { param::Index::reverbDecay, 6.00f },
                { param::Index::damping, 0.45f },
                { param::Index::width, 1.00f },
                { param::Index::dryWet, 0.15f },
                { param::Index::grainLevel, 0.85f },
                { param::Index::reverbLevel, 1.00f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::overdubCloud:
            return {
                { param::Index::memoryMode, 3.0f },
                { param::Index::grainMode, 4.0f },
                { param::Index::grainSize, 0.50f },
                { param::Index::density, 18.0f },
                { param::Index::overlap, 2.50f },
                { param::Index::spray, 0.65f },
                { param::Index::grainEnvelope, 1.0f },
                { param::Index::feedbackAmount, 0.48f },
                { param::Index::feedbackSource, 3.0f },
                { param::Index::feedbackSaturation, 0.55f },
                { param::Index::feedbackDiffusion, 0.60f },
                { param::Index::reverbDecay, 5.50f },
                { param::Index::reverbMix, 0.60f },
                { param::Index::mapDensity, 0.40f },
                { param::Index::dryWet, 0.20f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::harmonicWeather:
            return {
                { param::Index::pitchSource, 2.0f },
                { param::Index::followMode, 1.0f },
                { param::Index::grainMode, 7.0f },
                { param::Index::grainSize, 0.22f },
                { param::Index::density, 9.0f },
                { param::Index::fundamental, 0.70f },
                { param::Index::octaveMix, 0.55f },
                { param::Index::harmonicLayers, 4.0f },
                { param::Index::intervalA, 5.0f },
                { param::Index::harmonicSpread, 0.80f },
                { param::Index::mapHarmonic, 0.65f },
                { param::Index::mapReverb, 0.45f },
                { param::Index::reverbDecay, 4.50f },
                { param::Index::width, 0.90f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::polyRhythm:
            return {
                { param::Index::pitchSource, 0.0f },
                { param::Index::notePriority, 2.0f },
                { param::Index::grainMode, 3.0f },
                { param::Index::tempoSync, 1.0f },
                { param::Index::density, 16.0f },
                { param::Index::hostRate, 2.0f },
                { param::Index::grainSize, 0.08f },
                { param::Index::spray, 0.12f },
                { param::Index::reverseProbability, 0.20f },
                { param::Index::mapPosition, 0.60f },
                { param::Index::mapDensity, 0.40f },
                { param::Index::mapRate, 0.30f },
                { param::Index::reverbMix, 0.25f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::resonantCircuit:
            return {
                { param::Index::pitchSource, 2.0f },
                { param::Index::followMode, 3.0f },
                { param::Index::grainMode, 0.0f },
                { param::Index::density, 14.0f },
                { param::Index::scanSpeed, 0.55f },
                { param::Index::feedbackAmount, 0.62f },
                { param::Index::feedbackSource, 2.0f },
                { param::Index::feedbackFilter, 3800.0f },
                { param::Index::feedbackDamping, 0.20f },
                { param::Index::feedbackSaturation, 0.60f },
                { param::Index::filterFreq, 2200.0f },
                { param::Index::resonance, 0.78f },
                { param::Index::reverbFeedback, 0.72f },
                { param::Index::reverbMix, 0.55f },
                { param::Index::mapResonant, 0.80f },
                { param::Index::mapReverb, 0.65f }
            };

        case MemoryGranulatorModeAudioProcessor::Preset::darkArchive:
            return {
                { param::Index::memoryMode, 4.0f },
                { param::Index::grainMode, 5.0f },
                { param::Index::grainSize, 0.75f },
                { param::Index::density, 2.20f },
                { param::Index::overlap, 3.20f },
                { param::Index::scanSpeed, -0.08f },
                { param::Index::spray, 0.80f },
                { param::Index::reverseProbability, 0.75f },
                { param::Index::grainEnvelope, 1.0f },
                { param::Index::grainGain, 0.32f },
                { param::Index::stereoSpread, 1.00f },
                { param::Index::stereoRandom, 0.80f },
                { param::Index::maxGrains, 36.0f },
                { param::Index::feedbackAmount, 0.28f },
                { param::Index::feedbackSource, 3.0f },
                { param::Index::feedbackSaturation, 0.75f },
                { param::Index::feedbackDamping, 0.75f },
                { param::Index::reverbDecay, 7.50f },
                { param::Index::reverbMix, 0.70f },
                { param::Index::mapPosition, 0.50f },
                { param::Index::mapSpeed, 0.40f },
                { param::Index::dryWet, 0.12f },
                { param::Index::grainLevel, 0.65f },
                { param::Index::reverbLevel, 0.90f }
            };
    }

    return {};
}

juce::String formatParameter (param::Index index, float value)
{
    switch (index)
    {
        case param::Index::memoryLength:
        case param::Index::grainSize:
            return juce::String (value, 3) + " S";

        case param::Index::reverbDecay:
            return juce::String (value, 2) + " S";

        case param::Index::preDelay:
            return juce::String ((int) (value * 1000.0f + 0.5f)) + " MS";

        case param::Index::pitchAttack:
        case param::Index::pitchRelease:
        case param::Index::pitchSmoothing:
        case param::Index::mapSmoothing:
            return juce::String ((int) (value + 0.5f)) + " MS";

        case param::Index::feedbackFilter:
        case param::Index::filterFreq:
        case param::Index::trackRangeLow:
        case param::Index::trackRangeHigh:
            return juce::String ((int) (value + 0.5f)) + " HZ";

        case param::Index::noteLow:
        case param::Index::noteHigh:
            return juce::MidiMessage::getMidiNoteName ((int) (value + 0.5f), true, true, 2);

        case param::Index::rootNote:
            return juce::MidiMessage::getMidiNoteName ((int) (value + 0.5f), true, true, 2);

        case param::Index::tuning:
        case param::Index::detune:
        case param::Index::transposition:
        case param::Index::intervalA:
        case param::Index::intervalB:
        case param::Index::feedbackPitch:
            return juce::String ((int) (value >= 0.0f ? value + 0.5f : value - 0.5f)) + " ST";

        case param::Index::releaseCC:
        case param::Index::maxGrains:
        case param::Index::harmonicLayers:
            return juce::String ((int) (value + 0.5f));

        default:
            if (value >= 0.0f && value <= 1.0f)
                return juce::String ((int) (value * 100.0f + 0.5f)) + " %";

            return juce::String (value, 2);
    }
}
} // namespace

//==============================================================================
MemoryGranulatorModeAudioProcessor::MemoryGranulatorModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    resolveValuePointers();
    applyPreset (Preset::memoryBloom);
}

void MemoryGranulatorModeAudioProcessor::resolveValuePointers()
{
    for (int i = 0; i < param::kNumParameters; ++i)
        values[(std::size_t) i] = apvts.getRawParameterValue (ids::names[(std::size_t) i]);
}

float MemoryGranulatorModeAudioProcessor::getParameterValue (param::Index index) const noexcept
{
    const auto i = (std::size_t) index;
    return values[i] != nullptr ? values[i]->load (std::memory_order_relaxed) : 0.0f;
}

void MemoryGranulatorModeAudioProcessor::setParameterValue (param::Index index, float realValue)
{
    const auto id = juce::String (ids::names[(std::size_t) index]);

    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

juce::AudioProcessorValueTreeState::ParameterLayout MemoryGranulatorModeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto specs = makeParameterSpecs();

    for (const auto& spec : specs)
    {
        const auto id = juce::ParameterID { juce::String (ids::names[(std::size_t) spec.index]), 1 };
        const auto name = juce::String (spec.name);

        if (spec.kind == SpecKind::boolean)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (
                id, name, spec.defaultValue > 0.5f));
        }
        else if (spec.kind == SpecKind::choice)
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                id, name, spec.choices, (int) spec.defaultValue));
        }
        else
        {
            auto range = juce::NormalisableRange<float> (spec.minimum, spec.maximum, 0.0001f);

            if (spec.index == param::Index::density
                || spec.index == param::Index::grainSize
                || spec.index == param::Index::reverbDecay
                || spec.index == param::Index::modRate)
                range.setSkewForCentre (1.0f);

            juce::AudioParameterFloatAttributes attributes;
            attributes = attributes.withStringFromValueFunction (
                [index = spec.index] (float value, int)
                {
                    return formatParameter (index, value);
                });

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                id, name, range, spec.defaultValue, attributes));
        }
    }

    return layout;
}

juce::String MemoryGranulatorModeAudioProcessor::getFactoryPresetName (Preset preset)
{
    static const char* names[] = {
        "MEMORY BLOOM", "CHORD ECHO", "FROZEN VERB", "OVERDUB CLOUD",
        "HARMONIC WEATHER", "POLY RHYTHM", "RESONANT CIRCUIT", "DARK ARCHIVE"
    };

    return names[juce::jlimit (0, kNumPresets - 1, (int) preset)];
}

void MemoryGranulatorModeAudioProcessor::applyPreset (Preset preset)
{
    const auto specs = makeParameterSpecs();
    auto expected = defaultValues (specs);

    for (const auto& overrideValue : presetOverrides (preset))
        expected[(std::size_t) overrideValue.first] = overrideValue.second;

    for (int i = 0; i < param::kNumParameters; ++i)
        setParameterValue ((param::Index) i, expected[(std::size_t) i]);
}

juce::String MemoryGranulatorModeAudioProcessor::matchFactoryPreset() const
{
    const auto specs = makeParameterSpecs();

    for (int presetIndex = 0; presetIndex < kNumPresets; ++presetIndex)
    {
        const auto preset = (Preset) presetIndex;
        auto expected = defaultValues (specs);

        for (const auto& overrideValue : presetOverrides (preset))
            expected[(std::size_t) overrideValue.first] = overrideValue.second;

        auto matches = true;

        for (int i = 0; i < param::kNumParameters && matches; ++i)
        {
            const auto actual = getParameterValue ((param::Index) i);
            const auto wanted = expected[(std::size_t) i];
            const auto tolerance = specs[(std::size_t) i].kind == SpecKind::floatValue
                                 ? std::max (0.005f, std::abs (wanted) * 0.002f)
                                 : 0.01f;
            matches = std::abs (actual - wanted) <= tolerance;
        }

        if (matches)
            return getFactoryPresetName (preset);
    }

    return {};
}

//==============================================================================
juce::File MemoryGranulatorModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("MEMORY-GRANULATOR-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return MemoryGranulatorModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray MemoryGranulatorModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool MemoryGranulatorModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("MEMORY-GRANULATOR-MODE-PRESET");
    xml.setAttribute ("name", trimmed);
    xml.setAttribute ("version", 1);

    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
        {
            auto* child = xml.createNewChildElement ("PARAM");
            child->setAttribute ("id", ranged->paramID);
            child->setAttribute ("value", (double) ranged->convertFrom0to1 (ranged->getValue()));
        }

    return presetFile (trimmed).replaceWithText (xml.toString());
}

bool MemoryGranulatorModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("MEMORY-GRANULATOR-MODE-PRESET"))
        return false;

    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
    {
        auto* parameter = apvts.getParameter (child->getStringAttribute ("id"));

        if (parameter != nullptr)
            parameter->setValueNotifyingHost (
                parameter->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    }

    return true;
}

bool MemoryGranulatorModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

//==============================================================================
void MemoryGranulatorModeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

void MemoryGranulatorModeAudioProcessor::releaseResources()
{
    engine.reset();
}

bool MemoryGranulatorModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void MemoryGranulatorModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                        juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    memorygranulator::Settings settings;

    auto value = [this] (param::Index index)
    {
        return getParameterValue (index);
    };

    settings.memoryLength = value (param::Index::memoryLength);
    settings.captureEnable = value (param::Index::captureEnable) > 0.5f;
    settings.freeze = value (param::Index::freeze) > 0.5f;
    settings.captureArmed = value (param::Index::captureArmed) > 0.5f;
    settings.clearTrigger = value (param::Index::clearTrigger) > 0.5f;
    settings.releaseTrigger = value (param::Index::releaseTrigger) > 0.5f;
    settings.memoryMode = (int) value (param::Index::memoryMode);
    settings.recLevel = value (param::Index::recLevel);
    settings.stereoCapture = value (param::Index::stereoCapture) > 0.5f;
    settings.feedbackSource = (int) value (param::Index::feedbackSource);
    settings.feedbackAmount = value (param::Index::feedbackAmount);
    settings.feedbackStability = value (param::Index::feedbackStability);
    settings.feedbackFilter = value (param::Index::feedbackFilter);
    settings.feedbackDamping = value (param::Index::feedbackDamping);
    settings.feedbackSaturation = value (param::Index::feedbackSaturation);
    settings.feedbackDiffusion = value (param::Index::feedbackDiffusion);
    settings.feedbackPitch = value (param::Index::feedbackPitch);

    settings.grainMode = (int) value (param::Index::grainMode);
    settings.grainSize = value (param::Index::grainSize);
    settings.density = value (param::Index::density);
    settings.overlap = value (param::Index::overlap);
    settings.bufferPosition = value (param::Index::bufferPosition);
    settings.scanSpeed = value (param::Index::scanSpeed);
    settings.spray = value (param::Index::spray);
    settings.reverseProbability = value (param::Index::reverseProbability);
    settings.grainEnvelope = (int) value (param::Index::grainEnvelope);
    settings.grainGain = value (param::Index::grainGain);
    settings.stereoSpread = value (param::Index::stereoSpread);
    settings.stereoRandom = value (param::Index::stereoRandom);
    settings.maxGrains = (int) value (param::Index::maxGrains);
    settings.tempoSync = value (param::Index::tempoSync) > 0.5f;
    settings.hostRate = value (param::Index::hostRate);

    settings.pitchSource = (int) value (param::Index::pitchSource);
    settings.midiChannel = (int) value (param::Index::midiChannel);
    settings.releaseCC = (int) value (param::Index::releaseCC);
    settings.notePriority = (int) value (param::Index::notePriority);
    settings.followMode = (int) value (param::Index::followMode);
    settings.noteLow = value (param::Index::noteLow);
    settings.noteHigh = value (param::Index::noteHigh);
    settings.trackRangeLow = value (param::Index::trackRangeLow);
    settings.trackRangeHigh = value (param::Index::trackRangeHigh);
    settings.transposition = value (param::Index::transposition);
    settings.pitchSensitivity = value (param::Index::pitchSensitivity);
    settings.confidenceThreshold = value (param::Index::confidenceThreshold);
    settings.pitchAttack = value (param::Index::pitchAttack);
    settings.pitchRelease = value (param::Index::pitchRelease);
    settings.pitchSmoothing = value (param::Index::pitchSmoothing);
    settings.hybridBlend = value (param::Index::hybridBlend);

    settings.mapTranspose = value (param::Index::mapTranspose);
    settings.mapSpeed = value (param::Index::mapSpeed);
    settings.mapPosition = value (param::Index::mapPosition);
    settings.mapDensity = value (param::Index::mapDensity);
    settings.mapSize = value (param::Index::mapSize);
    settings.mapRate = value (param::Index::mapRate);
    settings.mapHarmonic = value (param::Index::mapHarmonic);
    settings.mapResonant = value (param::Index::mapResonant);
    settings.mapReverb = value (param::Index::mapReverb);
    settings.mapStereo = value (param::Index::mapStereo);
    settings.mapSmoothing = value (param::Index::mapSmoothing);
    settings.mapCurve = (int) value (param::Index::mapCurve);

    settings.fundamental = value (param::Index::fundamental);
    settings.octaveMix = value (param::Index::octaveMix);
    settings.harmonicLayers = (int) value (param::Index::harmonicLayers);
    settings.intervalA = value (param::Index::intervalA);
    settings.intervalB = value (param::Index::intervalB);
    settings.harmonicSpread = value (param::Index::harmonicSpread);
    settings.quantize = value (param::Index::quantize) > 0.5f;
    settings.scale = (int) value (param::Index::scale);
    settings.rootNote = value (param::Index::rootNote);
    settings.tuning = value (param::Index::tuning);
    settings.detune = value (param::Index::detune);
    settings.harmLevel1 = value (param::Index::harmLevel1);
    settings.harmLevel2 = value (param::Index::harmLevel2);
    settings.harmLevel3 = value (param::Index::harmLevel3);
    settings.harmLevel4 = value (param::Index::harmLevel4);

    settings.reverbMix = value (param::Index::reverbMix);
    settings.reverbDecay = value (param::Index::reverbDecay);
    settings.roomSize = value (param::Index::roomSize);
    settings.diffusion = value (param::Index::diffusion);
    settings.damping = value (param::Index::damping);
    settings.preDelay = value (param::Index::preDelay);
    settings.width = value (param::Index::width);
    settings.spaceModulation = value (param::Index::spaceModulation);
    settings.filterFreq = value (param::Index::filterFreq);
    settings.resonance = value (param::Index::resonance);
    settings.reverbFeedback = value (param::Index::reverbFeedback);
    settings.reverbFreeze = value (param::Index::reverbFreeze) > 0.5f;

    settings.modSource = (int) value (param::Index::modSource) - 1;
    settings.modDestination = (int) value (param::Index::modDestination);
    settings.modDepth = value (param::Index::modDepth);
    settings.modSource2 = (int) value (param::Index::modSource2) - 1;
    settings.modDestination2 = (int) value (param::Index::modDestination2);
    settings.modDepth2 = value (param::Index::modDepth2);
    settings.modRate = value (param::Index::modRate);

    settings.dryWet = value (param::Index::dryWet);
    settings.grainLevel = value (param::Index::grainLevel);
    settings.reverbLevel = value (param::Index::reverbLevel);
    settings.inputGain = value (param::Index::inputGain);
    settings.outputGain = value (param::Index::outputGain);

    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
        {
            const auto bpm = position->getBpm();

            if (bpm.hasValue())
                settings.hostBpm = (float) *bpm;
        }

    for (const auto& metadata : midi)
    {
        const auto message = metadata.getMessage();
        memorygranulator::MidiEvent event;
        event.channel = juce::jmax (0, message.getChannel() - 1);

        if (message.isNoteOn())
        {
            event.kind = memorygranulator::MidiKind::noteOn;
            event.note = message.getNoteNumber();
            event.value = message.getFloatVelocity();
        }
        else if (message.isNoteOff())
        {
            event.kind = memorygranulator::MidiKind::noteOff;
            event.note = message.getNoteNumber();
            event.value = 0.0f;
        }
        else if (message.isController())
        {
            event.kind = memorygranulator::MidiKind::controller;
            event.controller = message.getControllerNumber();
            event.value = (float) message.getControllerValue() / 127.0f;
        }
        else if (message.isAftertouch())
        {
            event.kind = memorygranulator::MidiKind::aftertouch;
            event.value = (float) message.getAfterTouchValue() / 127.0f;
        }
        else if (message.isChannelPressure())
        {
            event.kind = memorygranulator::MidiKind::aftertouch;
            event.value = (float) message.getChannelPressureValue() / 127.0f;
        }
        else if (message.isPitchWheel())
        {
            event.kind = memorygranulator::MidiKind::pitchWheel;
            event.value = (float) message.getPitchWheelValue() / 8191.5f - 1.0f;
        }
        else
        {
            continue;
        }

        engine.handleMidi (event);
    }

    engine.setSettings (settings);

    const auto processedChannels = numChannels > 1 ? 2 : 1;
    const float* inputs[2] = { buffer.getReadPointer (0),
                               processedChannels > 1 ? buffer.getReadPointer (1)
                                                    : buffer.getReadPointer (0) };
    float* outputs[2] = { buffer.getWritePointer (0),
                          processedChannels > 1 ? buffer.getWritePointer (1)
                                               : buffer.getWritePointer (0) };

    engine.process (inputs, outputs, processedChannels, numSamples);

    for (int channel = processedChannels; channel < numChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* MemoryGranulatorModeAudioProcessor::createEditor()
{
    return new MemoryGranulatorModeAudioProcessorEditor (*this);
}

void MemoryGranulatorModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MemoryGranulatorModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MemoryGranulatorModeAudioProcessor();
}
