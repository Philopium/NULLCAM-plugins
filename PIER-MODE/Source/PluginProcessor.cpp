#include "PluginProcessor.h"
#include "PluginEditor.h"

NullcamChainAudioProcessor::NullcamChainAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NullcamChainAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* id, const char* name,
                          juce::NormalisableRange<float> range, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    // the band pass comes first and decides most of the character
    add (ids::hpf, "HPF", juce::NormalisableRange<float> (70.0f, 1200.0f, 1.0f, 0.35f), 330.0f);
    add (ids::lpf, "LPF", juce::NormalisableRange<float> (350.0f, 16000.0f, 1.0f, 0.35f), 16000.0f);
    add (ids::crush, "CRUSH", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f);
    add (ids::delayMs, "DELAY", juce::NormalisableRange<float> (80.0f, 1600.0f, 1.0f, 0.5f), 344.0f);
    add (ids::echo, "ECHO", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f);
    add (ids::feedback, "FEEDBACK", juce::NormalisableRange<float> (0.0f, 0.98f, 0.001f), 0.978f);
    add (ids::ringHz, "RING Hz", juce::NormalisableRange<float> (38.0f, 400.0f, 0.1f, 0.6f), 117.2f);
    add (ids::ring, "RING", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f);
    add (ids::dry, "DRY", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f);
    add (ids::output, "OUTPUT", juce::NormalisableRange<float> (0.1f, 2.0f, 0.001f, 0.6f), 0.625f);

    return layout;
}

void NullcamChainAudioProcessor::prepareToPlay (double sampleRate, int)
{
    const int numChannels = juce::jmax (2, getTotalNumOutputChannels());
    channels.assign ((std::size_t) numChannels, nullcam::Chain {});

    for (auto& chain : channels)
    {
        chain.prepare (sampleRate);
        chain.setSettings ({});   // defaults, real values arrive in processBlock
    }
}

bool NullcamChainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

void NullcamChainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    nullcam::Settings s;
    s.hpfHz = apvts.getRawParameterValue (ids::hpf)->load();
    s.lpfHz = apvts.getRawParameterValue (ids::lpf)->load();
    s.crush = apvts.getRawParameterValue (ids::crush)->load();
    s.delayMs = apvts.getRawParameterValue (ids::delayMs)->load();
    s.echo = apvts.getRawParameterValue (ids::echo)->load();
    s.feedback = apvts.getRawParameterValue (ids::feedback)->load();
    s.ringHz = apvts.getRawParameterValue (ids::ringHz)->load();
    s.ring = apvts.getRawParameterValue (ids::ring)->load();
    s.dry = apvts.getRawParameterValue (ids::dry)->load();
    s.outputTrim = apvts.getRawParameterValue (ids::output)->load();

    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) channels.size());
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[(std::size_t) ch].setSettings (s);
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
            data[i] = channels[(std::size_t) ch].processSample (data[i]);
    }

    // anything above the configured layout stays silent instead of leaking dry audio
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

void NullcamChainAudioProcessor::setParam (const char* id, float value)
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

// One table per factory sound, used both to apply it and to recognise it again when
// a project is reopened.
const std::vector<FactoryValue>& factoryValues (NullcamChainAudioProcessor::Preset preset)
{
    static const std::vector<FactoryValue> clean {
        { ids::hpf, 70.0f }, { ids::lpf, 16000.0f }, { ids::crush, 0.0f },
        { ids::delayMs, 344.0f }, { ids::echo, 0.0f }, { ids::feedback, 0.0f },
        { ids::ringHz, 117.2f }, { ids::ring, 0.0f }, { ids::dry, 1.0f },
        { ids::output, 1.0f }
    };

    // fxSnapshot() with delay 2, gain 3, aud 0.4
    static const std::vector<FactoryValue> motion {
        { ids::hpf, 330.0f }, { ids::lpf, 16000.0f }, { ids::crush, 0.0f },
        { ids::delayMs, 344.0f }, { ids::echo, 1.0f }, { ids::feedback, 0.978f },
        { ids::ringHz, 117.2f }, { ids::ring, 1.0f }, { ids::dry, 1.0f },
        { ids::output, 0.625f }
    };

    // fxSnapshot() with SORT default params (mix 1, low 0.32)
    static const std::vector<FactoryValue> sort {
        { ids::hpf, 436.0f }, { ids::lpf, 13400.0f }, { ids::crush, 0.65f },
        { ids::delayMs, 100.0f }, { ids::echo, 0.0f }, { ids::feedback, 0.0f },
        { ids::ringHz, 60.0f }, { ids::ring, 0.35f }, { ids::dry, 1.0f },
        { ids::output, 0.82f }
    };

    switch (preset)
    {
        case NullcamChainAudioProcessor::Preset::clean:  return clean;
        case NullcamChainAudioProcessor::Preset::motion: return motion;
        case NullcamChainAudioProcessor::Preset::sort:   return sort;
    }

    return clean;
}
} // namespace

void NullcamChainAudioProcessor::applyPreset (Preset preset)
{
    for (const auto& entry : factoryValues (preset))
        setParam (entry.id, entry.value);
}

juce::String NullcamChainAudioProcessor::matchFactoryPreset() const
{
    const std::pair<Preset, const char*> candidates[] = {
        { Preset::motion, "MOTION" },
        { Preset::sort, "SORT" },
        { Preset::clean, "CLEAN" }
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

juce::File NullcamChainAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("PIER-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return NullcamChainAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray NullcamChainAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool NullcamChainAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("PIER-MODE-PRESET");
    xml.setAttribute ("name", trimmed);
    xml.setAttribute ("version", 1);

    // Every parameter is written generically, so a knob added later is saved without
    // touching this function. The value stored is the real value, not the normalised
    // one: a preset stays meaningful if a range is widened in a future version.
    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
        {
            auto* child = xml.createNewChildElement ("PARAM");
            child->setAttribute ("id", ranged->paramID);
            child->setAttribute ("value", (double) ranged->convertFrom0to1 (ranged->getValue()));
        }

    return presetFile (trimmed).replaceWithText (xml.toString());
}

bool NullcamChainAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("PIER-MODE-PRESET"))
        return false;

    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
    {
        auto* parameter = apvts.getParameter (child->getStringAttribute ("id"));

        if (parameter == nullptr)
            continue;   // a knob this build does not have (yet)

        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    }

    return true;
}

bool NullcamChainAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

void NullcamChainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void NullcamChainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* NullcamChainAudioProcessor::createEditor()
{
    return new NullcamChainAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NullcamChainAudioProcessor();
}
void NullcamChainAudioProcessor::releaseResources()
{
}