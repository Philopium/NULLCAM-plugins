// Renders the HarmonicResonatorMode faceplate without a host and checks the
// things a person would notice: the eight factory presets are really there, a
// user preset survives save -> change -> load, the plugin state round trips,
// every preset makes a sound, the partial display is fed by the engine, and
// the panel lays itself out.
//
//   harmonicresonator_ui_check [out.png] [scale]
//
// Everything here is a dev tool: it is not part of the installed plugin.

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

void check (bool condition, const juce::String& what)
{
    std::cout << (condition ? "  ok   " : "  FAIL ") << what << "\n";

    if (! condition)
        ++failures;
}

void dumpChildren (juce::Component& c)
{
    std::cout << "editor " << c.getWidth() << " x " << c.getHeight()
              << ", children: " << c.getNumChildComponents() << "\n";

    for (int i = 0; i < c.getNumChildComponents(); ++i)
    {
        auto* child = c.getChildComponent (i);
        std::cout << "  [" << i << "] "
                  << (child->isVisible() ? "visible " : "hidden  ")
                  << child->getBounds().toString() << "  "
                  << typeid (*child).name() << "\n";

        if (child->getBounds().isEmpty())
            ++failures;
    }
}

bool renderToPng (juce::Component& c, const juce::File& file, float scale)
{
    const auto image = c.createComponentSnapshot (c.getLocalBounds(), false, scale);

    if (! image.isValid())
        return false;

    file.deleteFile();
    juce::FileOutputStream stream (file);

    if (! stream.openedOk())
        return false;

    return juce::PNGImageFormat().writeImageToStream (image, stream);
}

float paramValue (HarmonicResonatorModeAudioProcessor& p, const juce::String& id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (HarmonicResonatorModeAudioProcessor& p, const juce::String& id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

// The preset box is the only combo that holds the whole factory list; the
// seven choice boxes hold three to five items each.
juce::ComboBox* findPresetBox (juce::Component& parent)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
        if (auto* box = dynamic_cast<juce::ComboBox*> (parent.getChildComponent (i)))
            if (box->getNumItems() >= 8)
                return box;

    return nullptr;
}

// A short phrase, to make the panel show something alive.
// It returns the loudest sample that came out.
float playPhrase (HarmonicResonatorModeAudioProcessor& processor, double seconds)
{
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);
    float peak = 0.0f;

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto value = 0.4f * (float) std::sin (2.0 * 3.14159265358979 * 220.0 * t)
                               * (float) std::exp (-2.0 * std::fmod (t, 0.25));

            buffer.setSample (0, i, value);
            buffer.setSample (1, i, value);
        }

        processor.processBlock (buffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
                peak = std::max (peak, std::abs (buffer.getSample (channel, i)));
    }

    return peak;
}

// A richer phrase for the picture: a few notes at once, so more of the bank
// rings and the partial display has something to show.
void playChord (HarmonicResonatorModeAudioProcessor& processor, double seconds)
{
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);
    const double notes[] = { 110.0, 220.0, 330.0, 440.0, 660.0 };

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto envelope = (float) std::exp (-1.2 * std::fmod (t, 0.5));

            auto value = 0.0f;

            for (const auto hz : notes)
                value += (float) std::sin (2.0 * 3.14159265358979 * hz * t);

            value *= 0.08f * envelope;

            buffer.setSample (0, i, value);
            buffer.setSample (1, i, value);
        }

        processor.processBlock (buffer, midi);
    }
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    HarmonicResonatorModeAudioProcessor processor;

    //--------------------------------------------------------------------------
    // The eight factory presets.
    std::cout << "factory presets:\n";

    for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
        std::cout << "  [" << i << "] "
                  << HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
                         (HarmonicResonatorModeAudioProcessor::Preset) i)
                  << "\n";

    check (processor.matchFactoryPreset() == "BELL METAL",
           "the defaults are recognised as BELL METAL");

    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::ironGong);
    check (processor.matchFactoryPreset() == "IRON GONG", "the IRON GONG preset is recognised");

    bool everyPresetRecognised = true;

    for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
    {
        const auto preset = (HarmonicResonatorModeAudioProcessor::Preset) i;
        processor.applyPreset (preset);

        const auto expected = HarmonicResonatorModeAudioProcessor::getFactoryPresetName (preset);
        const auto found = processor.matchFactoryPreset();

        if (found != expected)
        {
            std::cout << "    " << expected << " came back as \""
                      << (found.isEmpty() ? juce::String ("nothing") : found) << "\"\n";
            everyPresetRecognised = false;
        }
    }

    check (everyPresetRecognised, "all eight presets are recognised after loading");

    // The loop above leaves PITCH LOCK loaded, so start from BELL METAL before
    // measuring how far another preset moves the knobs.
    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::bellMetal);

    const auto savedTune = paramValue (processor, ids::tune);
    const auto savedDecay = paramValue (processor, ids::decay);

    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::pitchLock);
    check (std::abs (paramValue (processor, ids::tune) - savedTune) > 0.5f
               || std::abs (paramValue (processor, ids::decay) - savedDecay) > 0.05f,
           "loading another preset really moves the knobs");

    //--------------------------------------------------------------------------
    // A user preset survives save -> change -> load.
    const juce::String name ("UI CHECK RESONANCE");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    setParam (processor, ids::interaction, 0.91f);
    setParam (processor, ids::mix, 0.22f);
    check (processor.matchFactoryPreset().isEmpty(), "changing a knob leaves the factory preset");

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name), "the preset shows up in the list");

    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::glassHarmonics);
    check (std::abs (paramValue (processor, ids::mix) - 0.22f) > 0.05f,
           "GLASS HARMONICS moved the mix away");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (paramValue (processor, ids::interaction) - 0.91f) < 1.0e-4f,
           "INTERACTION restored");
    check (std::abs (paramValue (processor, ids::mix) - 0.22f) < 1.0e-4f, "DRY / WET restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name), "the preset is gone from the list");

    //--------------------------------------------------------------------------
    // The session state (what the host saves) round trips too.
    {
        const auto before = paramValue (processor, ids::feedback);

        juce::MemoryBlock state;
        processor.getStateInformation (state);
        setParam (processor, ids::feedback, 0.05f);
        processor.setStateInformation (state.getData(), (int) state.getSize());

        check (std::abs (paramValue (processor, ids::feedback) - before) < 1.0e-4f,
               "the plugin state restores the feedback setting");
    }

    //--------------------------------------------------------------------------
    // Every factory preset makes a sound, and no preset blows up.
    {
        processor.prepareToPlay (48000.0, 512);

        bool allGood = true;

        for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
        {
            processor.applyPreset ((HarmonicResonatorModeAudioProcessor::Preset) i);

            const auto peak = playPhrase (processor, 2.0);
            const auto presetName = HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
                (HarmonicResonatorModeAudioProcessor::Preset) i);

            std::cout << "  " << presetName << ": peak " << peak << "\n";

            // The output limiter has a ceiling of about 1.11, so the rails here
            // are 1.2: a preset may scream, it may not run away.
            if (peak < 0.02f || peak > 1.2f)
                allGood = false;
        }

        check (allGood, "every factory preset makes a sound and stays inside the rails");
    }

    //--------------------------------------------------------------------------
    // The partial display is fed by the engine, not by the knobs.
    processor.prepareToPlay (48000.0, 512);
    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::bellMetal);
    playPhrase (processor, 2.0);

    const auto& snapshot = processor.getSnapshot();

    bool freqsInRange = true;

    for (int i = 0; i < harmonicresonator::kNumResonators; ++i)
    {
        const auto hz = snapshot.partialFreq[i].load();

        std::cout << "  partial " << i << ": " << hz << " Hz, energy "
                  << snapshot.partialEnergy[i].load() << "\n";

        if (hz < 20.0f || hz > 21600.0f)
            freqsInRange = false;
    }

    check (freqsInRange, "the eight partials sit inside the audible range");
    check (snapshot.activePartials.load() > 0, "the bank reports partials ringing");
    check (snapshot.wetLevel.load() > 0.001f, "the wet meter saw the signal");
    check (snapshot.tracked.load() > 150.0f && snapshot.tracked.load() < 300.0f,
           "the tracker found the 220 Hz phrase");

    //--------------------------------------------------------------------------
    // The editor renders, and every control is placed.
    //
    // A chord is played right before the picture is taken, so the partial
    // display shows the bank as it really behaves.
    processor.applyPreset (HarmonicResonatorModeAudioProcessor::Preset::bellMetal);
    playChord (processor, 2.0);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

    int rotaries = 0;
    int combos = 0;

    for (int i = 0; i < editor->getNumChildComponents(); ++i)
    {
        auto* child = editor->getChildComponent (i);

        if (auto* slider = dynamic_cast<juce::Slider*> (child))
        {
            if (! slider->isHorizontal())
                ++rotaries;
        }
        else if (dynamic_cast<juce::ComboBox*> (child) != nullptr)
        {
            ++combos;
        }
    }

    std::cout << "rotary knobs: " << rotaries << ", combo boxes: " << combos << "\n";

    check (rotaries == 23, "the panel shows 23 knobs");
    check (combos == 8, "the panel shows 7 choice boxes and the preset box");

    auto* presetBox = findPresetBox (*editor);

    if (presetBox == nullptr)
        std::cout << "  (the preset box was not found)\n";
    else
    {
        check (presetBox->getNumItems() == HarmonicResonatorModeAudioProcessor::kNumPresets,
               "the preset box lists the eight factory presets");
        check (presetBox->getSelectedId() == 1, "it opens on BELL METAL");
    }

    if (auto* panel = dynamic_cast<HarmonicResonatorModeAudioProcessorEditor*> (editor.get()))
    {
        // The panel reads the live data with a little smoothing, so let it
        // settle before the picture is taken.
        for (int i = 0; i < 40; ++i)
            panel->pullLiveData();
    }

    const auto out = juce::File::getCurrentWorkingDirectory()
                         .getChildFile (argc > 1 ? argv[1] : "panel.png");
    const auto scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 1.0f;

    if (! renderToPng (*editor, out, scale <= 0.0f ? 1.0f : scale))
    {
        std::cout << "could not write " << out.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "wrote " << out.getFullPathName() << "\n";

    std::cout << (failures == 0 ? "\nall checks passed\n"
                                : "\n" + juce::String (failures) + " check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
