#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
// The six knobs, in the order the factories below read them:
// sub, doom, oct, fuzz, tone, level.
struct PresetValues
{
    float sub, doom, oct, fuzz, tone, level;
};

const PresetValues& presetValues (GiannisModeAudioProcessor::Preset preset)
{
    static const PresetValues table[] = {
        // sub,  doom, oct,  fuzz, tone, level
        { 0.80f, 0.30f, 0.20f, 0.35f, 0.45f, 0.90f },  // GIANNIS
        { 0.95f, 0.00f, 0.00f, 0.00f, 0.50f, 0.85f },  // SUB ZERO
        { 0.85f, 0.75f, 0.00f, 0.55f, 0.30f, 0.95f },  // DUBSTEP
        { 0.55f, 0.95f, 0.00f, 0.25f, 0.20f, 0.90f },  // DOOM
        { 0.25f, 0.00f, 0.85f, 0.75f, 0.55f, 0.80f },  // FUZZ OCT
        { 0.70f, 0.15f, 0.45f, 0.50f, 0.45f, 0.85f },  // SYNTH BASS
        { 0.20f, 0.00f, 0.55f, 0.00f, 0.85f, 0.85f },  // CLEAN OCT
        { 0.95f, 0.95f, 0.00f, 0.80f, 0.25f, 1.00f },  // QUAKE
        { 0.70f, 0.45f, 0.25f, 0.65f, 0.35f, 0.90f },  // WUB
        { 0.55f, 0.20f, 0.70f, 0.20f, 0.70f, 0.85f },  // ORGAN
        { 0.40f, 0.00f, 0.45f, 0.15f, 0.60f, 0.90f },  // TAPE OCT
        { 0.90f, 0.40f, 0.00f, 0.00f, 0.35f, 0.95f }   // SUBSONIC
    };

    return table[juce::jlimit (0, (int) std::size (table) - 1, (int) preset)];
}
} // namespace

//==============================================================================
GiannisModeAudioProcessor::GiannisModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    // Open on the signature voice, so the plugin is useful the moment it appears.
    applyPreset (Preset::giannis);
}

juce::AudioProcessorValueTreeState::ParameterLayout GiannisModeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* id, const char* name,
                          juce::NormalisableRange<float> range, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    // The two subs, the octave up, then the dirt.
    add (ids::sub,   "SUB",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.80f);
    add (ids::doom,  "DOOM",  juce::NormalisableRange<float> (0.0f, 1.0f), 0.30f);
    add (ids::oct,   "OCT",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.20f);
    add (ids::fuzz,  "FUZZ",  juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f);
    add (ids::tone,  "TONE",  juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f);
    add (ids::level, "LEVEL", juce::NormalisableRange<float> (0.0f, 1.5f), 0.90f);

    return layout;
}

juce::String GiannisModeAudioProcessor::getFactoryPresetName (Preset preset)
{
    static const char* names[] = {
        "GIANNIS", "SUB ZERO", "DUBSTEP", "DOOM", "FUZZ OCT", "SYNTH BASS",
        "CLEAN OCT", "QUAKE", "WUB", "ORGAN", "TAPE OCT", "SUBSONIC"
    };

    return names[juce::jlimit (0, kNumPresets - 1, (int) preset)];
}

void GiannisModeAudioProcessor::setParam (const char* id, float value)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void GiannisModeAudioProcessor::applyPreset (Preset preset)
{
    const auto& v = presetValues (preset);

    setParam (ids::sub, v.sub);
    setParam (ids::doom, v.doom);
    setParam (ids::oct, v.oct);
    setParam (ids::fuzz, v.fuzz);
    setParam (ids::tone, v.tone);
    setParam (ids::level, v.level);
}

juce::String GiannisModeAudioProcessor::matchFactoryPreset() const
{
    for (int i = 0; i < kNumPresets; ++i)
    {
        const auto& v = presetValues ((Preset) i);

        if (std::abs (apvts.getRawParameterValue (ids::sub)->load() - v.sub) < 0.005f
            && std::abs (apvts.getRawParameterValue (ids::doom)->load() - v.doom) < 0.005f
            && std::abs (apvts.getRawParameterValue (ids::oct)->load() - v.oct) < 0.005f
            && std::abs (apvts.getRawParameterValue (ids::fuzz)->load() - v.fuzz) < 0.005f
            && std::abs (apvts.getRawParameterValue (ids::tone)->load() - v.tone) < 0.005f
            && std::abs (apvts.getRawParameterValue (ids::level)->load() - v.level) < 0.005f)
            return getFactoryPresetName ((Preset) i);
    }

    return {};
}

//==============================================================================
juce::File GiannisModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("GIANNIS-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return GiannisModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray GiannisModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool GiannisModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("GIANNIS-MODE-PRESET");
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

bool GiannisModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("GIANNIS-MODE-PRESET"))
        return false;

    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
    {
        auto* parameter = apvts.getParameter (child->getStringAttribute ("id"));

        if (parameter == nullptr)
            continue;

        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    }

    return true;
}

bool GiannisModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

//==============================================================================
void GiannisModeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate;
    engine.prepare (sampleRate);
}

void GiannisModeAudioProcessor::releaseResources() {}

bool GiannisModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void GiannisModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    giannis::Settings settings;
    settings.sub = apvts.getRawParameterValue (ids::sub)->load();
    settings.doom = apvts.getRawParameterValue (ids::doom)->load();
    settings.oct = apvts.getRawParameterValue (ids::oct)->load();
    settings.fuzz = apvts.getRawParameterValue (ids::fuzz)->load();
    settings.tone = apvts.getRawParameterValue (ids::tone)->load();
    settings.level = apvts.getRawParameterValue (ids::level)->load();

    engine.setSettings (settings);

    // The dry input is already in the buffer; the engine reads it and writes the
    // processed signal in place.
    const auto processed = buffer.getNumChannels() > 1 ? 2 : 1;

    const float* inputs[2] = { buffer.getReadPointer (0),
                               processed > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0) };
    float* outputs[2] = { buffer.getWritePointer (0),
                          processed > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0) };

    engine.process (inputs, outputs, processed, numSamples);

    // Keep any extra channels silent rather than copying the mono signal into
    // them: this plugin is mono in / mono out, stereo in / stereo out.
    for (int channel = processed; channel < numChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* GiannisModeAudioProcessor::createEditor()
{
    return new GiannisModeAudioProcessorEditor (*this);
}

void GiannisModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GiannisModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GiannisModeAudioProcessor();
}
