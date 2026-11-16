#include "PluginProcessor.h"
#include "PluginEditor.h"

MauriModeAudioProcessor::MauriModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MauriModeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* id, const char* name,
                          juce::NormalisableRange<float> range, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    // The rhythm: a stepped rotary per choice, so the whole panel is knobs.
    add (ids::clock, "CLOCK", juce::NormalisableRange<float> (0.0f, 2.0f, 1.0f), 0.0f);
    add (ids::timeSig, "TIME SIG", juce::NormalisableRange<float> (0.0f, (float) (mauri::kTimeSigs - 1), 1.0f), 0.0f);
    add (ids::rate, "RATE", juce::NormalisableRange<float> (0.0f, 7.0f, 1.0f), 3.0f);
    add (ids::style, "STYLE", juce::NormalisableRange<float> (0.0f, (float) (mauri::kStyles - 1), 1.0f), 0.0f);

    // The slice blade itself.
    add (ids::slice, "SLICE", juce::NormalisableRange<float> (0.03f, 1.0f, 0.001f), 0.65f);
    add (ids::repeat, "RATCHET", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.10f);
    add (ids::reverse, "REVERSE", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f);
    add (ids::scatter, "SCATTER", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.10f);

    // Dynamics -> pattern & rhythm: the playing generates the sequence in real time.
    add (ids::dynPat, "DYN PAT", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.70f);
    add (ids::dynSlice, "DYN SLICE", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.50f);
    add (ids::dynRate, "DYN RATCH", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f);
    add (ids::sens, "SENS", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f);

    add (ids::tone, "TONE", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.80f);
    add (ids::mix, "MIX", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f);
    add (ids::output, "LEVEL", juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.6f), 1.0f);

    return layout;
}

void MauriModeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setSettings ({});
}

bool MauriModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

void MauriModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    mauri::Settings s;
    s.clock = juce::roundToInt (apvts.getRawParameterValue (ids::clock)->load());
    s.timeSig = juce::roundToInt (apvts.getRawParameterValue (ids::timeSig)->load());
    s.rateIndex = juce::roundToInt (apvts.getRawParameterValue (ids::rate)->load());
    s.style = juce::roundToInt (apvts.getRawParameterValue (ids::style)->load());
    s.slice = apvts.getRawParameterValue (ids::slice)->load();
    s.repeat = apvts.getRawParameterValue (ids::repeat)->load();
    s.reverse = apvts.getRawParameterValue (ids::reverse)->load();
    s.scatter = apvts.getRawParameterValue (ids::scatter)->load();
    s.dynPat = apvts.getRawParameterValue (ids::dynPat)->load();
    s.dynSlice = apvts.getRawParameterValue (ids::dynSlice)->load();
    s.dynRate = apvts.getRawParameterValue (ids::dynRate)->load();
    s.sens = apvts.getRawParameterValue (ids::sens)->load();
    s.tone = apvts.getRawParameterValue (ids::tone)->load();
    s.mix = apvts.getRawParameterValue (ids::mix)->load();
    s.output = apvts.getRawParameterValue (ids::output)->load();

    engine.setSettings (s);

    if (auto* ph = getPlayHead())
    {
        if (const auto position = ph->getPosition())
        {
            if (const auto bpm = position->getBpm())
                displayBpm.store (*bpm, std::memory_order_relaxed);

            displayPlaying.store (position->getIsPlaying(), std::memory_order_relaxed);
            engine.setTransport (displayBpm.load (std::memory_order_relaxed),
                                 displayPlaying.load (std::memory_order_relaxed));
        }
    }

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels > 0)
    {
        auto* left = buffer.getWritePointer (0);
        auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

        engine.process (left, right, numSamples);
    }

    for (int ch = 2; ch < numChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    {
        const juce::SpinLock::ScopedTryLockType lock (scopeLock);

        if (lock.isLocked())
            engine.updateScope (scope.data(), (int) scope.size());
    }
}

void MauriModeAudioProcessor::setParam (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

namespace
{
struct FactoryValue
{
    const char* id;
    float value;
};

const std::vector<FactoryValue>& factoryValues (MauriModeAudioProcessor::Preset preset)
{
    static const std::vector<FactoryValue> mauri {
        { ids::clock, 0 }, { ids::timeSig, 0 }, { ids::rate, 3 }, { ids::style, 0 },
        { ids::slice, 0.65f }, { ids::repeat, 0.10f }, { ids::reverse, 0.0f },
        { ids::scatter, 0.10f }, { ids::dynPat, 0.70f }, { ids::dynSlice, 0.50f },
        { ids::dynRate, 0.45f }, { ids::sens, 0.55f }, { ids::tone, 0.80f },
        { ids::mix, 1.0f }, { ids::output, 1.0f }
    };

    static const std::vector<FactoryValue> odd78 {
        { ids::clock, 0 }, { ids::timeSig, 6 }, { ids::rate, 3 }, { ids::style, 1 },
        { ids::slice, 0.60f }, { ids::repeat, 0.15f }, { ids::reverse, 0.10f },
        { ids::scatter, 0.15f }, { ids::dynPat, 0.85f }, { ids::dynSlice, 0.60f },
        { ids::dynRate, 0.55f }, { ids::sens, 0.55f }, { ids::tone, 0.82f },
        { ids::mix, 1.0f }, { ids::output, 0.98f }
    };

    static const std::vector<FactoryValue> ghost54 {
        { ids::clock, 1 }, { ids::timeSig, 2 }, { ids::rate, 3 }, { ids::style, 4 },
        { ids::slice, 0.72f }, { ids::repeat, 0.08f }, { ids::reverse, 0.20f },
        { ids::scatter, 0.25f }, { ids::dynPat, 0.90f }, { ids::dynSlice, 0.70f },
        { ids::dynRate, 0.65f }, { ids::sens, 0.60f }, { ids::tone, 0.68f },
        { ids::mix, 1.0f }, { ids::output, 1.0f }
    };

    static const std::vector<FactoryValue> shred68 {
        { ids::clock, 2 }, { ids::timeSig, 4 }, { ids::rate, 4 }, { ids::style, 3 },
        { ids::slice, 0.45f }, { ids::repeat, 0.25f }, { ids::reverse, 0.10f },
        { ids::scatter, 0.30f }, { ids::dynPat, 0.80f }, { ids::dynSlice, 0.65f },
        { ids::dynRate, 0.75f }, { ids::sens, 0.55f }, { ids::tone, 0.85f },
        { ids::mix, 1.0f }, { ids::output, 0.96f }
    };

    switch (preset)
    {
        case MauriModeAudioProcessor::Preset::mauri:   return mauri;
        case MauriModeAudioProcessor::Preset::odd78:   return odd78;
        case MauriModeAudioProcessor::Preset::ghost54: return ghost54;
        case MauriModeAudioProcessor::Preset::shred68: return shred68;
    }

    return mauri;
}
} // namespace

void MauriModeAudioProcessor::applyPreset (Preset preset)
{
    for (const auto& entry : factoryValues (preset))
        setParam (entry.id, entry.value);
}

juce::String MauriModeAudioProcessor::matchFactoryPreset() const
{
    const std::pair<Preset, const char*> candidates[] = {
        { Preset::mauri, "MAURI" },
        { Preset::odd78, "ODD 7/8" },
        { Preset::ghost54, "GHOST 5/4" },
        { Preset::shred68, "SHRED 6/8" }
    };

    for (const auto& candidate : candidates)
    {
        bool matches = true;

        for (const auto& entry : factoryValues (candidate.first))
        {
            const auto* parameter = apvts.getParameter (entry.id);

            if (parameter == nullptr)
                continue;

            const auto actual = parameter->convertFrom0to1 (parameter->getValue());
            const auto tolerance = 0.002f * juce::jmax (1.0f, std::abs (entry.value));

            if (std::abs (actual - entry.value) > tolerance)
            {
                matches = false;
                break;
            }
        }

        if (matches)
            return candidate.second;
    }

    return {};
}

juce::File MauriModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("MAURI-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return MauriModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray MauriModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool MauriModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("MAURI-MODE-PRESET");
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

bool MauriModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("MAURI-MODE-PRESET"))
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

bool MauriModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

bool MauriModeAudioProcessor::copyScope (float* dest, int count) const
{
    if (dest == nullptr || count <= 0)
        return false;

    const juce::SpinLock::ScopedTryLockType lock (scopeLock);

    if (! lock.isLocked())
        return false;

    const auto n = juce::jmin (count, (int) scope.size());

    for (int i = 0; i < n; ++i)
        dest[i] = scope[(std::size_t) i];

    return true;
}

void MauriModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void MauriModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MauriModeAudioProcessor::createEditor()
{
    return new MauriModeAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MauriModeAudioProcessor();
}

void MauriModeAudioProcessor::releaseResources()
{
}
