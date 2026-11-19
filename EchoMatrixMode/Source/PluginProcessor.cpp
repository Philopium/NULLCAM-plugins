#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
struct PresetValues
{
    float delayTime, feedback, mix, inputGain, outputGain;
    float hpfCutoff, lpfCutoff, drive;
    // 16 steps: enabled, level, pan
    float stepEnabled[echomatrix::Engine::kNumSteps];
    float stepLevel[echomatrix::Engine::kNumSteps];
    float stepPan[echomatrix::Engine::kNumSteps];
};

const PresetValues& presetValues (EchoMatrixModeAudioProcessor::Preset preset)
{
    static PresetValues table[] = {
        // delayTime, feedback, mix, in, out, hpf, lpf, drive
        { 0.375f, 0.55f, 0.60f, 1.0f, 1.0f, 120.0f, 9000.0f, 0.10f }, // dottedEighth
        { 0.500f, 0.40f, 0.55f, 1.0f, 1.0f, 100.0f, 11000.0f, 0.05f }, // pingPong
        { 0.500f, 0.75f, 0.50f, 1.0f, 1.0f, 80.0f,  6000.0f, 0.30f }, // dub
        { 0.250f, 0.60f, 0.70f, 1.0f, 1.0f, 200.0f, 8000.0f, 0.20f }, // reverseWash
        { 0.125f, 0.35f, 0.80f, 1.0f, 1.0f, 150.0f, 10000.0f, 0.00f }, // glitchTap
        { 0.750f, 0.30f, 0.45f, 1.0f, 1.0f, 60.0f,  5000.0f, 0.00f }, // ambientWash
        { 0.110f, 0.20f, 0.50f, 1.0f, 1.0f, 100.0f, 12000.0f, 0.00f }, // slapback
        { 0.333f, 0.85f, 0.65f, 1.0f, 1.0f, 40.0f,  7000.0f, 0.40f }, // chaotic
    };

    auto& v = table[juce::jlimit (0, (int) std::size (table) - 1, (int) preset)];

    // Phase 1: fixed 16-step pattern, alternating on/off for variety.
    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        v.stepEnabled[i] = (i % 2 == 0) ? 1.0f : 0.0f;
        v.stepLevel[i]   = 0.7f + 0.3f * float (i % 3) / 2.0f;
        v.stepPan[i]     = (i % 4 == 1) ? -0.6f : ((i % 4 == 3) ? 0.6f : 0.0f);
    }

    // Preset-specific twists on top of the alternating pattern.
    switch (preset)
    {
        case EchoMatrixModeAudioProcessor::Preset::pingPong:
            for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
                v.stepPan[i] = (i % 2 == 0) ? -0.8f : 0.8f;
            break;
        case EchoMatrixModeAudioProcessor::Preset::dub:
            for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
                v.stepEnabled[i] = 1.0f;
            break;
        case EchoMatrixModeAudioProcessor::Preset::glitchTap:
            for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
                v.stepEnabled[i] = (i % 3 == 0) ? 1.0f : 0.0f;
            break;
        case EchoMatrixModeAudioProcessor::Preset::ambientWash:
            for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
                v.stepEnabled[i] = 1.0f;
            break;
        case EchoMatrixModeAudioProcessor::Preset::slapback:
            for (int i = 1; i < echomatrix::Engine::kNumSteps; ++i)
                v.stepEnabled[i] = 0.0f;
            break;
        case EchoMatrixModeAudioProcessor::Preset::chaotic:
            for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
            {
                v.stepEnabled[i] = 1.0f;
                v.stepPan[i]     = float (i % 5 - 2) * 0.4f;
            }
            break;
        default:
            break;
    }

    return v;
}
} // namespace

//==============================================================================
EchoMatrixModeAudioProcessor::EchoMatrixModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    applyPreset (Preset::dottedEighth);
}

juce::AudioProcessorValueTreeState::ParameterLayout EchoMatrixModeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // The read-outs are formatted here, on the parameter itself, so the host
    // shows the same text that the panel shows.
    auto add = [&layout] (const juce::String& id, const juce::String& name,
                          juce::NormalisableRange<float> range, float def,
                          std::function<juce::String (float)> text = nullptr)
    {
        juce::AudioParameterFloatAttributes attributes;

        if (text != nullptr)
        {
            // The string function is handed the real value (not the
            // normalised one), so it can print what the knob means.
            attributes = attributes.withStringFromValueFunction (
                [text] (float value, int) { return text (value); });
        }

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def, attributes));
    };

    const auto percent = [] (float v)
    {
        return juce::String ((int) (v * 100.0f + 0.5f)) + " %";
    };

    const auto decibels = [] (float v)
    {
        if (v <= 0.0001f)
            return juce::String ("-INF DB");

        return juce::String (20.0f * std::log10 (v), 1) + " DB";
    };

    // Global delay controls.
    add (ids::delayTime,  "DELAY TIME", juce::NormalisableRange<float> (0.05f, 2.0f, 0.001f), 0.375f,
         [] (float v) { return juce::String ((int) (v * 1000.0f + 0.5f)) + " MS"; });
    add (ids::feedback,   "FEEDBACK",   juce::NormalisableRange<float> (0.0f, 0.99f), 0.55f, percent);
    add (ids::mix,        "MIX",        juce::NormalisableRange<float> (0.0f, 1.0f), 0.60f, percent);
    add (ids::inputGain,  "INPUT GAIN", juce::NormalisableRange<float> (0.0f, 2.0f), 1.0f, decibels);
    add (ids::outputGain, "OUTPUT GAIN",juce::NormalisableRange<float> (0.0f, 2.0f), 1.0f, decibels);
    add (ids::hpfCutoff,  "HPF",        juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f), 120.0f,
         [] (float v) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " K"
                                           : juce::String ((int) (v + 0.5f)) + " HZ"; });
    add (ids::lpfCutoff,  "LPF",        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f), 9000.0f,
         [] (float v) { return juce::String (v / 1000.0f, 1) + " K"; });
    add (ids::drive,      "DRIVE",      juce::NormalisableRange<float> (0.0f, 1.0f), 0.10f, percent);

    // Tempo sync is a switch: when it is on the delay follows the host BPM.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::sync, 1 }, "SYNC", false,
        juce::AudioParameterBoolAttributes().withStringFromValueFunction (
            [] (float v, int) { return v > 0.5f ? juce::String ("ON") : juce::String ("OFF"); })));

    // Per-step controls (16 steps x 3 params).
    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        add (stepParamId (ids::stepEnabled, i),
             juce::String ("STEP ") + juce::String (i + 1) + " ON",
             juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f,
             [] (float v) { return v > 0.5f ? juce::String ("ON") : juce::String ("OFF"); });
        add (stepParamId (ids::stepLevel, i),
             juce::String ("STEP ") + juce::String (i + 1) + " LVL",
             juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f, percent);
        add (stepParamId (ids::stepPan, i),
             juce::String ("STEP ") + juce::String (i + 1) + " PAN",
             juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
             [] (float v)
             {
                 if (v < -0.005f)
                     return "L " + juce::String ((int) (-v * 100.0f + 0.5f));

                 if (v > 0.005f)
                     return "R " + juce::String ((int) (v * 100.0f + 0.5f));

                 return juce::String ("C");
             });
    }

    return layout;
}

juce::String EchoMatrixModeAudioProcessor::stepParamId (const char* kind, int step)
{
    return juce::String (kind) + juce::String (step);
}

juce::String EchoMatrixModeAudioProcessor::getFactoryPresetName (Preset preset)
{
    static const char* names[] = {
        "DOTTED EIGHTH", "PING PONG", "DUB", "TAPE ECHO",
        "GLITCH TAP", "AMBIENT WASH", "SLAPBACK", "CHAOTIC"
    };

    return names[juce::jlimit (0, kNumPresets - 1, (int) preset)];
}

void EchoMatrixModeAudioProcessor::setParam (const juce::String& id, float value)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void EchoMatrixModeAudioProcessor::applyPreset (Preset preset)
{
    const auto& v = presetValues (preset);

    setParam (ids::delayTime,  v.delayTime);
    setParam (ids::feedback,   v.feedback);
    setParam (ids::mix,        v.mix);
    setParam (ids::inputGain,  v.inputGain);
    setParam (ids::outputGain, v.outputGain);
    setParam (ids::hpfCutoff,  v.hpfCutoff);
    setParam (ids::lpfCutoff,  v.lpfCutoff);
    setParam (ids::drive,      v.drive);
    setParam (ids::sync,       0.0f);

    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        setParam (stepParamId (ids::stepEnabled, i), v.stepEnabled[i]);
        setParam (stepParamId (ids::stepLevel, i),   v.stepLevel[i]);
        setParam (stepParamId (ids::stepPan, i),     v.stepPan[i]);
    }
}

juce::String EchoMatrixModeAudioProcessor::matchFactoryPreset() const
{
    for (int i = 0; i < kNumPresets; ++i)
    {
        const auto& v = presetValues ((Preset) i);

        auto match = true;
        match &= std::abs (apvts.getRawParameterValue (ids::delayTime)->load() - v.delayTime) < 0.005f;
        match &= std::abs (apvts.getRawParameterValue (ids::feedback)->load() - v.feedback) < 0.005f;
        match &= std::abs (apvts.getRawParameterValue (ids::mix)->load() - v.mix) < 0.005f;
        match &= std::abs (apvts.getRawParameterValue (ids::inputGain)->load() - v.inputGain) < 0.005f;
        match &= std::abs (apvts.getRawParameterValue (ids::outputGain)->load() - v.outputGain) < 0.005f;
        match &= std::abs (apvts.getRawParameterValue (ids::hpfCutoff)->load() - v.hpfCutoff) < 0.5f;
        match &= std::abs (apvts.getRawParameterValue (ids::lpfCutoff)->load() - v.lpfCutoff) < 5.0f;
        match &= std::abs (apvts.getRawParameterValue (ids::drive)->load() - v.drive) < 0.005f;
        match &= apvts.getRawParameterValue (ids::sync)->load() < 0.5f;

        for (int s = 0; s < echomatrix::Engine::kNumSteps && match; ++s)
        {
            match &= std::abs (apvts.getRawParameterValue (stepParamId (ids::stepEnabled, s))->load() - v.stepEnabled[s]) < 0.01f;
            match &= std::abs (apvts.getRawParameterValue (stepParamId (ids::stepLevel, s))->load() - v.stepLevel[s]) < 0.01f;
            match &= std::abs (apvts.getRawParameterValue (stepParamId (ids::stepPan, s))->load() - v.stepPan[s]) < 0.01f;
        }

        if (match)
            return getFactoryPresetName ((Preset) i);
    }

    return {};
}

//==============================================================================
juce::File EchoMatrixModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("ECHOMATRIX-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return EchoMatrixModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray EchoMatrixModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool EchoMatrixModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("ECHOMATRIX-MODE-PRESET");
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

bool EchoMatrixModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("ECHOMATRIX-MODE-PRESET"))
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

bool EchoMatrixModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

//==============================================================================
void EchoMatrixModeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate;
    engine.prepare (sampleRate);
}

void EchoMatrixModeAudioProcessor::releaseResources() {}

bool EchoMatrixModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void EchoMatrixModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Read host BPM for tempo sync.
    if (auto* playhead = getPlayHead())
    {
        if (const auto position = playhead->getPosition())
            if (const auto bpm = position->getBpm())
                if (*bpm > 0.0)
                    hostBpm = *bpm;
    }
    engine.setBpm (hostBpm);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    echomatrix::Settings s;
    s.delayTime = apvts.getRawParameterValue (ids::delayTime)->load();
    s.syncEnabled = apvts.getRawParameterValue (ids::sync)->load() > 0.5f;
    s.feedback = apvts.getRawParameterValue (ids::feedback)->load();
    s.mix = apvts.getRawParameterValue (ids::mix)->load();
    s.inputGain = apvts.getRawParameterValue (ids::inputGain)->load();
    s.outputGain = apvts.getRawParameterValue (ids::outputGain)->load();
    s.hpfCutoff = apvts.getRawParameterValue (ids::hpfCutoff)->load();
    s.lpfCutoff = apvts.getRawParameterValue (ids::lpfCutoff)->load();
    s.drive = apvts.getRawParameterValue (ids::drive)->load();

    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        s.steps[i].enabled = apvts.getRawParameterValue (stepParamId (ids::stepEnabled, i))->load() > 0.5f;
        s.steps[i].level = apvts.getRawParameterValue (stepParamId (ids::stepLevel, i))->load();
        s.steps[i].pan = apvts.getRawParameterValue (stepParamId (ids::stepPan, i))->load();
    }

    engine.setSettings (s);

    const auto processed = buffer.getNumChannels() > 1 ? 2 : 1;

    const float* inputs[2] = { buffer.getReadPointer (0),
                               processed > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0) };
    float* outputs[2] = { buffer.getWritePointer (0),
                          processed > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0) };

    engine.process (inputs, outputs, processed, numSamples);

    for (int channel = processed; channel < numChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* EchoMatrixModeAudioProcessor::createEditor()
{
    return new EchoMatrixModeAudioProcessorEditor (*this);
}

void EchoMatrixModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EchoMatrixModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoMatrixModeAudioProcessor();
}
