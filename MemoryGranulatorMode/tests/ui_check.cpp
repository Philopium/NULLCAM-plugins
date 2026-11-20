// Renders the MemoryGranulatorMode faceplate without a host and checks the
// things a person would notice: all eight factory presets exist, user presets
// survive save -> change -> load, the plugin state round trips, every preset
// makes sound, the live displays are fed by the engine, and the panel lays
// itself out without empty controls.
//
//   memorygranulator_ui_check [out.png] [scale]

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

bool renderToPng (juce::Component& component, const juce::File& file, float scale)
{
    const auto image = component.createComponentSnapshot (component.getLocalBounds(), false, scale);

    if (! image.isValid())
        return false;

    file.deleteFile();
    juce::FileOutputStream stream (file);

    if (! stream.openedOk())
        return false;

    return juce::PNGImageFormat().writeImageToStream (image, stream);
}

void dumpChildren (juce::Component& component)
{
    std::cout << "editor " << component.getWidth() << " x " << component.getHeight()
              << ", children: " << component.getNumChildComponents() << "\n";

    for (int i = 0; i < component.getNumChildComponents(); ++i)
    {
        auto* child = component.getChildComponent (i);
        std::cout << "  [" << i << "] "
                  << (child->isVisible() ? "visible " : "hidden  ")
                  << child->getBounds().toString() << "  "
                  << typeid (*child).name() << "\n";

        if (child->getBounds().isEmpty())
            ++failures;
    }
}

float playPhrase (MemoryGranulatorModeAudioProcessor& processor, double seconds,
                  bool sendMidi = false)
{
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);
    float peak = 0.0f;

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();
        midi.clear();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto envelope = (float) std::exp (-1.4 * std::fmod (t, 0.25));
            const auto value = 0.34f * (float) std::sin (2.0 * 3.14159265358979 * 220.0 * t)
                             * envelope;
            buffer.setSample (0, i, value);
            buffer.setSample (1, i, value);
        }

        if (sendMidi && block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.9f), 0);

        processor.processBlock (buffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
                peak = std::max (peak, std::abs (buffer.getSample (channel, i)));
    }

    return peak;
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    MemoryGranulatorModeAudioProcessor processor;

    //--------------------------------------------------------------------------
    std::cout << "factory presets:\n";

    for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
        std::cout << "  [" << i << "] "
                  << MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
                         (MemoryGranulatorModeAudioProcessor::Preset) i) << "\n";

    check (processor.matchFactoryPreset() == "MEMORY BLOOM",
           "the defaults are recognised as MEMORY BLOOM");

    bool everyPresetRecognised = true;

    for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
    {
        const auto preset = (MemoryGranulatorModeAudioProcessor::Preset) i;
        processor.applyPreset (preset);
        const auto expected = MemoryGranulatorModeAudioProcessor::getFactoryPresetName (preset);
        const auto found = processor.matchFactoryPreset();

        if (found != expected)
        {
            std::cout << "    " << expected << " came back as \""
                      << (found.isEmpty() ? juce::String ("nothing") : found) << "\n";
            everyPresetRecognised = false;
        }
    }

    check (everyPresetRecognised, "all eight presets are recognised after loading");

    processor.applyPreset (MemoryGranulatorModeAudioProcessor::Preset::memoryBloom);

    //--------------------------------------------------------------------------
    const juce::String name ("UI CHECK MEMORY");
    processor.deleteUserPreset (name);

    processor.setParameterValue (param::Index::feedbackAmount, 0.73f);
    processor.setParameterValue (param::Index::grainSize, 0.31f);
    check (processor.matchFactoryPreset().isEmpty(), "changing a knob leaves the factory preset");
    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name), "the preset shows up in the list");

    processor.applyPreset (MemoryGranulatorModeAudioProcessor::Preset::frozenVerb);
    check (std::abs (processor.getParameterValue (param::Index::grainSize) - 0.31f) > 0.02f,
           "FROZEN VERB moved the grain size away");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (processor.getParameterValue (param::Index::feedbackAmount) - 0.73f) < 1.0e-4f,
           "MEMORY FEEDBACK restored");
    check (std::abs (processor.getParameterValue (param::Index::grainSize) - 0.31f) < 1.0e-4f,
           "GRAIN SIZE restored");
    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name), "the preset is gone from the list");

    //--------------------------------------------------------------------------
    {
        const auto before = processor.getParameterValue (param::Index::reverbDecay);
        juce::MemoryBlock state;
        processor.getStateInformation (state);
        processor.setParameterValue (param::Index::reverbDecay, 0.8f);
        processor.setStateInformation (state.getData(), (int) state.getSize());
        check (std::abs (processor.getParameterValue (param::Index::reverbDecay) - before) < 1.0e-4f,
               "the plugin state restores the reverb decay");
    }

    //--------------------------------------------------------------------------
    processor.prepareToPlay (48000.0, 512);
    bool everyPresetSounds = true;

    for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
    {
        processor.applyPreset ((MemoryGranulatorModeAudioProcessor::Preset) i);
        const auto peak = playPhrase (processor, 1.5, true);
        const auto name = MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
            (MemoryGranulatorModeAudioProcessor::Preset) i);
        std::cout << "  " << name << ": peak " << peak << "\n";

        if (peak < 0.015f || peak > 1.2f)
            everyPresetSounds = false;
    }

    check (everyPresetSounds, "every factory preset makes sound and stays inside the rails");

    //--------------------------------------------------------------------------
    processor.applyPreset (MemoryGranulatorModeAudioProcessor::Preset::memoryBloom);
    playPhrase (processor, 1.0, true);

    const auto& snapshot = processor.getSnapshot();
    check (snapshot.activeGrains.load() > 0, "the live display sees active grains");
    check (snapshot.activeNotes.load() > 0, "the live display sees the MIDI voice");
    check (snapshot.inputLevel.load() > 0.001f, "the input meter saw the phrase");
    check (snapshot.outputLevel.load() > 0.001f, "the output meter saw the result");

    //--------------------------------------------------------------------------
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    check (editor != nullptr, "the processor creates an editor");

    if (editor != nullptr)
    {
        editor->setSize (1440, 1024);
        editor->resized();

        int sliders = 0;
        int combos = 0;
        int toggles = 0;
        bool emptyBounds = false;

        for (int i = 0; i < editor->getNumChildComponents(); ++i)
        {
            auto* child = editor->getChildComponent (i);
            if (child->getBounds().isEmpty())
                emptyBounds = true;

            if (dynamic_cast<juce::Slider*> (child) != nullptr)
                ++sliders;
            else if (dynamic_cast<juce::ComboBox*> (child) != nullptr)
                ++combos;
            else if (dynamic_cast<juce::ToggleButton*> (child) != nullptr)
                ++toggles;
        }

        std::cout << "  sliders: " << sliders << ", combos: " << combos
                  << ", toggles: " << toggles << "\n";
        check (sliders == 76, "the panel shows 76 working knobs");
        check (combos == 15, "the panel shows 14 parameter choices and the preset box");
        check (toggles == 9, "the panel shows 9 working switches");
        check (! emptyBounds, "no control has an empty bounds rectangle");
        dumpChildren (*editor);

        const auto file = argc > 1
            ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
            : juce::File::getCurrentWorkingDirectory().getChildFile ("panel.png");
        const auto scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;
        check (renderToPng (*editor, file, scale), "the faceplate renders to " + file.getFileName());
    }

    std::cout << (failures == 0 ? "\nall UI checks passed\n"
                                : "\n" + juce::String (failures) + " UI check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
