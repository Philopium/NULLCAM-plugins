#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
// Every value a factory preset sets. The defaults are the first preset, so a
// preset only has to write down what it changes.
struct PresetValues
{
    float inputGain = 1.0f;
    float outputGain = 0.8f;
    float mix = 0.80f;
    float fundamental = 0.10f;

    float drive = 0.25f;
    int   shape = 0;            // soft
    float evenOdd = -0.25f;
    float symmetry = 0.15f;
    float density = 0.55f;
    float brightness = 0.50f;

    float tune = 220.0f;
    int   tuning = 4;           // metallic
    float resonance = 0.70f;
    float decay = 3.0f;
    float spread = 0.60f;
    float resGain = 0.80f;
    float align = 0.0f;

    int   routing = 0;          // dist -> res
    float interaction = 0.55f;
    float feedback = 0.50f;
    float fbDrive = 0.30f;
    float lowCut = 120.0f;
    float highCut = 9000.0f;
    int   source = 0;           // res
    int   destination = 0;      // both

    int   modMode = 0;          // lfo
    int   modDest = 0;          // tune
    float modRate = 0.40f;
    float modDepth = 0.0f;
    float envDepth = 0.0f;
};

PresetValues makePreset (HarmonicResonatorModeAudioProcessor::Preset preset)
{
    PresetValues v;

    switch (preset)
    {
        case HarmonicResonatorModeAudioProcessor::Preset::bellMetal:
            // The default: an inharmonic bank with a long tail.
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::tubeChoir:
            v.tune = 146.8f;
            v.tuning = 0;
            v.decay = 2.2f;
            v.drive = 0.45f;
            v.evenOdd = -0.60f;
            v.density = 0.50f;
            v.brightness = 0.40f;
            v.interaction = 0.40f;
            v.feedback = 0.35f;
            v.resGain = 0.75f;
            v.mix = 0.75f;
            v.spread = 0.45f;
            v.lowCut = 80.0f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::ironGong:
            v.tune = 98.0f;
            v.decay = 6.5f;
            v.drive = 0.60f;
            v.shape = 2;             // fold
            v.interaction = 0.70f;
            v.feedback = 0.60f;
            v.fbDrive = 0.50f;
            v.lowCut = 150.0f;
            v.highCut = 14000.0f;
            v.resGain = 0.85f;
            v.mix = 0.85f;
            v.spread = 0.70f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::glassHarmonics:
            v.tune = 440.0f;
            v.tuning = 1;            // odd
            v.decay = 0.70f;
            v.drive = 0.50f;
            v.shape = 4;             // cheby
            v.density = 0.80f;
            v.brightness = 0.70f;
            v.interaction = 0.25f;
            v.feedback = 0.30f;
            v.resGain = 0.65f;
            v.mix = 0.70f;
            v.highCut = 16000.0f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::subAnchor:
            v.tune = 55.0f;
            v.tuning = 0;
            v.decay = 1.2f;
            v.resonance = 0.50f;
            v.drive = 0.20f;
            v.fundamental = 0.60f;
            v.mix = 0.60f;
            v.resGain = 0.50f;
            v.interaction = 0.20f;
            v.feedback = 0.25f;
            v.lowCut = 30.0f;
            v.highCut = 2500.0f;
            v.spread = 0.25f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::wreckedRadio:
            v.tune = 330.0f;
            v.decay = 0.25f;
            v.drive = 0.75f;
            v.shape = 5;             // crush
            v.density = 0.70f;
            v.resonance = 0.60f;
            v.interaction = 0.60f;
            v.feedback = 0.40f;
            v.fbDrive = 0.70f;
            v.modMode = 2;           // sample and hold
            v.modRate = 6.0f;
            v.modDepth = 0.35f;
            v.highCut = 5000.0f;
            v.mix = 0.85f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::resonantFold:
            v.tune = 130.8f;
            v.tuning = 3;            // chord
            v.decay = 1.8f;
            v.drive = 0.70f;
            v.shape = 2;             // fold
            v.interaction = 0.85f;
            v.feedback = 0.70f;
            v.fbDrive = 0.40f;
            v.routing = 2;           // bidirectional
            v.source = 3;            // combined
            v.lowCut = 110.0f;
            v.highCut = 12000.0f;
            v.mix = 0.80f;
            break;

        case HarmonicResonatorModeAudioProcessor::Preset::pitchLock:
            v.align = 1.0f;
            v.decay = 2.5f;
            v.resonance = 0.65f;
            v.drive = 0.30f;
            v.evenOdd = -1.0f;
            v.density = 0.45f;
            v.interaction = 0.40f;
            v.feedback = 0.40f;
            v.fundamental = 0.55f;
            v.mix = 0.70f;
            v.resGain = 0.80f;
            v.spread = 0.40f;
            break;

        default:
            break;
    }

    return v;
}

juce::String hzText (float v)
{
    if (v >= 1000.0f)
        return juce::String (v / 1000.0f, 1) + " K";

    return juce::String ((int) (v + 0.5f)) + " HZ";
}

juce::String noteText (float v)
{
    const auto note = (int) std::round (69.0 + 12.0 * std::log2 (std::max (1.0f, v) / 440.0));
    const auto name = juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, note), true, true, 3);

    return name + " " + hzText (v);
}

juce::String secondsText (float v)
{
    if (v < 1.0f)
        return juce::String ((int) (v * 1000.0f + 0.5f)) + " MS";

    return juce::String (v, 2) + " S";
}

const juce::StringArray& shapeNames()
{
    static const juce::StringArray names { "SOFT", "HARD", "FOLD", "ASYM", "CHEBY", "CRUSH", "RING" };
    return names;
}

const juce::StringArray& tuningNames()
{
    static const juce::StringArray names { "HARMONIC", "ODD", "EVEN", "CHORD", "METALLIC" };
    return names;
}

const juce::StringArray& routingNames()
{
    static const juce::StringArray names { "DIST > RES", "RES > DIST", "RES < > DIST" };
    return names;
}

const juce::StringArray& sourceNames()
{
    static const juce::StringArray names { "RESONATOR", "DISTORTION", "CLEAN", "COMBINED", "BAND" };
    return names;
}

const juce::StringArray& destinationNames()
{
    static const juce::StringArray names { "BOTH STAGES", "DISTORTION", "RESONATOR" };
    return names;
}

const juce::StringArray& modModeNames()
{
    static const juce::StringArray names { "LFO", "RANDOM", "S & H", "ENVELOPE" };
    return names;
}

const juce::StringArray& modDestNames()
{
    static const juce::StringArray names { "TUNE", "RESONANCE", "DECAY", "DRIVE", "INTERACTION", "PAN" };
    return names;
}
} // namespace

//==============================================================================
HarmonicResonatorModeAudioProcessor::HarmonicResonatorModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    resolveValuePointers();
    applyPreset (Preset::bellMetal);
}

void HarmonicResonatorModeAudioProcessor::resolveValuePointers()
{
    values.inputGain   = apvts.getRawParameterValue (ids::inputGain);
    values.outputGain  = apvts.getRawParameterValue (ids::outputGain);
    values.mix         = apvts.getRawParameterValue (ids::mix);
    values.fundamental = apvts.getRawParameterValue (ids::fundamental);

    values.drive      = apvts.getRawParameterValue (ids::drive);
    values.shape      = apvts.getRawParameterValue (ids::shape);
    values.evenOdd    = apvts.getRawParameterValue (ids::evenOdd);
    values.symmetry   = apvts.getRawParameterValue (ids::symmetry);
    values.density    = apvts.getRawParameterValue (ids::density);
    values.brightness = apvts.getRawParameterValue (ids::brightness);

    values.tune      = apvts.getRawParameterValue (ids::tune);
    values.tuning    = apvts.getRawParameterValue (ids::tuning);
    values.resonance = apvts.getRawParameterValue (ids::resonance);
    values.decay     = apvts.getRawParameterValue (ids::decay);
    values.spread    = apvts.getRawParameterValue (ids::spread);
    values.resGain   = apvts.getRawParameterValue (ids::resGain);
    values.align     = apvts.getRawParameterValue (ids::align);

    values.routing     = apvts.getRawParameterValue (ids::routing);
    values.interaction = apvts.getRawParameterValue (ids::interaction);
    values.feedback    = apvts.getRawParameterValue (ids::feedback);
    values.fbDrive     = apvts.getRawParameterValue (ids::fbDrive);
    values.lowCut      = apvts.getRawParameterValue (ids::lowCut);
    values.highCut     = apvts.getRawParameterValue (ids::highCut);
    values.source      = apvts.getRawParameterValue (ids::source);
    values.destination = apvts.getRawParameterValue (ids::destination);

    values.modMode  = apvts.getRawParameterValue (ids::modMode);
    values.modDest  = apvts.getRawParameterValue (ids::modDest);
    values.modRate  = apvts.getRawParameterValue (ids::modRate);
    values.modDepth = apvts.getRawParameterValue (ids::modDepth);
    values.envDepth = apvts.getRawParameterValue (ids::envDepth);
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonicResonatorModeAudioProcessor::createLayout()
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
            attributes = attributes.withStringFromValueFunction (
                [text] (float value, int) { return text (value); });

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def, attributes));
    };

    auto addChoice = [&layout] (const juce::String& id, const juce::String& name,
                                const juce::StringArray& choices, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id, 1 }, name, choices, def));
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

    auto tuneRange = juce::NormalisableRange<float> (55.0f, 880.0f, 0.1f);
    tuneRange.setSkewForCentre (220.0f);

    auto decayRange = juce::NormalisableRange<float> (0.05f, 8.0f, 0.001f);
    decayRange.setSkewForCentre (1.0f);

    auto rateRange = juce::NormalisableRange<float> (0.02f, 20.0f, 0.001f);
    rateRange.setSkewForCentre (1.0f);

    auto lowRange = juce::NormalisableRange<float> (20.0f, 900.0f, 1.0f);
    lowRange.setSkewForCentre (120.0f);

    auto highRange = juce::NormalisableRange<float> (500.0f, 18000.0f, 1.0f);
    highRange.setSkewForCentre (6000.0f);

    const auto unit = juce::NormalisableRange<float> (0.0f, 1.0f);
    const auto linear = juce::NormalisableRange<float> (0.0f, 2.0f);

    // ---- output ----
    add (ids::inputGain,   "INPUT",       linear, 1.0f, decibels);
    add (ids::outputGain,  "OUTPUT",      linear, 0.8f, decibels);
    add (ids::mix,         "DRY / WET",   unit,   0.80f, percent);
    add (ids::fundamental, "FUNDAMENTAL", unit,   0.10f, percent);

    // ---- harmonics ----
    add (ids::drive,      "DRIVE",      unit, 0.25f, percent);
    addChoice (ids::shape, "SHAPE", shapeNames(), 0);
    add (ids::evenOdd,    "EVEN / ODD", juce::NormalisableRange<float> (-1.0f, 1.0f), -0.25f,
         [] (float v)
         {
             if (v < -0.02f)
                 return "ODD " + juce::String ((int) (-v * 100.0f + 0.5f));

             if (v > 0.02f)
                 return "EVEN " + juce::String ((int) (v * 100.0f + 0.5f));

             return juce::String ("BALANCED");
         });
    add (ids::symmetry,   "SYMMETRY",   unit, 0.15f, percent);
    add (ids::density,    "DENSITY",    unit, 0.55f, percent);
    add (ids::brightness, "BRIGHTNESS", unit, 0.50f, percent);

    // ---- resonators ----
    add (ids::tune,      "TUNE",      tuneRange,  220.0f, noteText);
    addChoice (ids::tuning, "TUNING", tuningNames(), 4);
    add (ids::resonance, "RESONANCE", unit, 0.70f, percent);
    add (ids::decay,     "DECAY",     decayRange, 3.0f, secondsText);
    add (ids::spread,    "SPREAD",    unit, 0.60f, percent);
    add (ids::resGain,   "BANK",      unit, 0.80f, percent);
    add (ids::align,     "ALIGN",     unit, 0.0f,
         [] (float v) { return v > 0.005f ? juce::String ((int) (v * 100.0f + 0.5f)) + " %"
                                          : juce::String ("FREE"); });

    // ---- interaction ----
    addChoice (ids::routing,  "ROUTING", routingNames(), 0);
    add (ids::interaction, "INTERACTION", unit, 0.55f, percent);
    add (ids::feedback,    "FEEDBACK",    unit, 0.50f, percent);
    add (ids::fbDrive,     "FB DRIVE",    unit, 0.30f, percent);
    add (ids::lowCut,      "LOW CUT",     lowRange, 120.0f, hzText);
    add (ids::highCut,     "HIGH CUT",    highRange, 9000.0f, hzText);
    addChoice (ids::source,      "FB SOURCE", sourceNames(), 0);
    addChoice (ids::destination, "FB DEST",   destinationNames(), 0);

    // ---- modulation ----
    addChoice (ids::modMode, "MOD SOURCE", modModeNames(), 0);
    addChoice (ids::modDest, "MOD DEST",   modDestNames(), 0);
    add (ids::modRate,  "RATE",     rateRange, 0.40f,
         [] (float v) { return juce::String (v, 2) + " HZ"; });
    add (ids::modDepth, "DEPTH",    unit, 0.0f, percent);
    add (ids::envDepth, "ENV>DRIVE", unit, 0.0f, percent);

    return layout;
}

juce::String HarmonicResonatorModeAudioProcessor::getFactoryPresetName (Preset preset)
{
    static const char* names[] = {
        "BELL METAL", "TUBE CHOIR", "IRON GONG", "GLASS HARMONICS",
        "SUB ANCHOR", "WRECKED RADIO", "RESONANT FOLD", "PITCH LOCK"
    };

    return names[juce::jlimit (0, kNumPresets - 1, (int) preset)];
}

void HarmonicResonatorModeAudioProcessor::setParam (const juce::String& id, float value)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void HarmonicResonatorModeAudioProcessor::applyPreset (Preset preset)
{
    const auto v = makePreset (preset);

    setParam (ids::inputGain,   v.inputGain);
    setParam (ids::outputGain,  v.outputGain);
    setParam (ids::mix,         v.mix);
    setParam (ids::fundamental, v.fundamental);

    setParam (ids::drive,      v.drive);
    setParam (ids::shape,      (float) v.shape);
    setParam (ids::evenOdd,    v.evenOdd);
    setParam (ids::symmetry,   v.symmetry);
    setParam (ids::density,    v.density);
    setParam (ids::brightness, v.brightness);

    setParam (ids::tune,      v.tune);
    setParam (ids::tuning,    (float) v.tuning);
    setParam (ids::resonance, v.resonance);
    setParam (ids::decay,     v.decay);
    setParam (ids::spread,    v.spread);
    setParam (ids::resGain,   v.resGain);
    setParam (ids::align,     v.align);

    setParam (ids::routing,     (float) v.routing);
    setParam (ids::interaction, v.interaction);
    setParam (ids::feedback,    v.feedback);
    setParam (ids::fbDrive,     v.fbDrive);
    setParam (ids::lowCut,      v.lowCut);
    setParam (ids::highCut,     v.highCut);
    setParam (ids::source,      (float) v.source);
    setParam (ids::destination, (float) v.destination);

    setParam (ids::modMode,  (float) v.modMode);
    setParam (ids::modDest,  (float) v.modDest);
    setParam (ids::modRate,  v.modRate);
    setParam (ids::modDepth, v.modDepth);
    setParam (ids::envDepth, v.envDepth);
}

juce::String HarmonicResonatorModeAudioProcessor::matchFactoryPreset() const
{
    for (int i = 0; i < kNumPresets; ++i)
    {
        const auto v = makePreset ((Preset) i);
        auto match = true;

        // Every parameter is compared, so a new preset cannot silently forget
        // one: if it did, the preset would never be recognised again.
        match &= std::abs (values.inputGain->load()   - v.inputGain)   < 0.005f;
        match &= std::abs (values.outputGain->load()  - v.outputGain)  < 0.005f;
        match &= std::abs (values.mix->load()         - v.mix)         < 0.005f;
        match &= std::abs (values.fundamental->load() - v.fundamental) < 0.005f;
        match &= std::abs (values.drive->load()       - v.drive)       < 0.005f;
        match &= std::abs (values.shape->load()       - (float) v.shape) < 0.01f;
        match &= std::abs (values.evenOdd->load()     - v.evenOdd)     < 0.005f;
        match &= std::abs (values.symmetry->load()    - v.symmetry)    < 0.005f;
        match &= std::abs (values.density->load()     - v.density)     < 0.005f;
        match &= std::abs (values.brightness->load()  - v.brightness)  < 0.005f;
        match &= std::abs (values.tune->load()        - v.tune)        < 0.5f;
        match &= std::abs (values.tuning->load()      - (float) v.tuning) < 0.01f;
        match &= std::abs (values.resonance->load()   - v.resonance)   < 0.005f;
        match &= std::abs (values.decay->load()       - v.decay)       < 0.005f;
        match &= std::abs (values.spread->load()      - v.spread)      < 0.005f;
        match &= std::abs (values.resGain->load()     - v.resGain)     < 0.005f;
        match &= std::abs (values.align->load()       - v.align)       < 0.005f;
        match &= std::abs (values.routing->load()     - (float) v.routing) < 0.01f;
        match &= std::abs (values.interaction->load() - v.interaction) < 0.005f;
        match &= std::abs (values.feedback->load()    - v.feedback)    < 0.005f;
        match &= std::abs (values.fbDrive->load()     - v.fbDrive)     < 0.005f;
        match &= std::abs (values.lowCut->load()      - v.lowCut)      < 0.5f;
        match &= std::abs (values.highCut->load()     - v.highCut)     < 5.0f;
        match &= std::abs (values.source->load()      - (float) v.source) < 0.01f;
        match &= std::abs (values.destination->load() - (float) v.destination) < 0.01f;
        match &= std::abs (values.modMode->load()     - (float) v.modMode) < 0.01f;
        match &= std::abs (values.modDest->load()     - (float) v.modDest) < 0.01f;
        match &= std::abs (values.modRate->load()     - v.modRate)     < 0.005f;
        match &= std::abs (values.modDepth->load()    - v.modDepth)    < 0.005f;
        match &= std::abs (values.envDepth->load()    - v.envDepth)    < 0.005f;

        if (match)
            return getFactoryPresetName ((Preset) i);
    }

    return {};
}

//==============================================================================
juce::File HarmonicResonatorModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("HARMONIC-RESONATOR-MODE")
        .getChildFile ("presets");
}

namespace
{
juce::File presetFile (const juce::String& name)
{
    return HarmonicResonatorModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray HarmonicResonatorModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool HarmonicResonatorModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("HARMONIC-RESONATOR-MODE-PRESET");
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

bool HarmonicResonatorModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("HARMONIC-RESONATOR-MODE-PRESET"))
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

bool HarmonicResonatorModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

//==============================================================================
void HarmonicResonatorModeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

void HarmonicResonatorModeAudioProcessor::releaseResources() {}

bool HarmonicResonatorModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void HarmonicResonatorModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                        juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    harmonicresonator::Settings s;

    s.inputGain   = values.inputGain->load();
    s.outputGain  = values.outputGain->load();
    s.mix         = values.mix->load();
    s.fundamental = values.fundamental->load();

    s.drive      = values.drive->load();
    s.shape      = (harmonicresonator::Shape) juce::jlimit (0, 6, (int) values.shape->load());
    s.evenOdd    = values.evenOdd->load();
    s.symmetry   = values.symmetry->load();
    s.density    = values.density->load();
    s.brightness = values.brightness->load();

    s.tune      = values.tune->load();
    s.tuning    = (harmonicresonator::Tuning) juce::jlimit (0, 4, (int) values.tuning->load());
    s.resonance = values.resonance->load();
    s.decay     = values.decay->load();
    s.spread    = values.spread->load();
    s.resGain   = values.resGain->load();
    s.align     = values.align->load();

    s.routing     = (harmonicresonator::Routing) juce::jlimit (0, 2, (int) values.routing->load());
    s.interaction = values.interaction->load();
    s.feedback    = values.feedback->load();
    s.fbDrive     = values.fbDrive->load();
    s.lowCut      = values.lowCut->load();
    s.highCut     = values.highCut->load();
    s.source      = (harmonicresonator::Source) juce::jlimit (0, 4, (int) values.source->load());
    s.destination = (harmonicresonator::Destination) juce::jlimit (0, 2, (int) values.destination->load());

    s.modMode  = (harmonicresonator::ModMode) juce::jlimit (0, 3, (int) values.modMode->load());
    s.modDest  = (harmonicresonator::ModDest) juce::jlimit (0, 5, (int) values.modDest->load());
    s.modRate  = values.modRate->load();
    s.modDepth = values.modDepth->load();
    s.envDepth = values.envDepth->load();

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
juce::AudioProcessorEditor* HarmonicResonatorModeAudioProcessor::createEditor()
{
    return new HarmonicResonatorModeAudioProcessorEditor (*this);
}

void HarmonicResonatorModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HarmonicResonatorModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarmonicResonatorModeAudioProcessor();
}
