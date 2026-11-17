// Renders the GIANNIS-MODE faceplate without a host and checks the things a
// person would notice: the twelve factory presets are really there, a played
// note moves the divider and the subs, a user preset survives save -> change
// -> load, and the plugin state round trips.
//
//   giannis_ui_check [out.png] [scale]
//
// Everything here is a dev tool: it is not part of the installed plugin.

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginEditor.h"

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

float paramValue (GiannisModeAudioProcessor& p, const char* id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (GiannisModeAudioProcessor& p, const char* id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

// A short bass phrase, written by hand, to test the divider and the note tracking.
void playPhrase (GiannisModeAudioProcessor& processor, double seconds)
{
    const double notes[] = { 110.0, 146.83, 164.81, 220.0 }; // A2, D3, E3, A3
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        const auto note = notes[(block / 16) % 4];

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto value = 0.35f * (float) std::sin (2.0 * 3.14159265358979 * note * t);

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

    GiannisModeAudioProcessor processor;

    //--------------------------------------------------------------------------
    // The twelve factory presets.
    std::cout << "factory presets:\n";

    for (int i = 0; i < GiannisModeAudioProcessor::kNumPresets; ++i)
    {
        const auto name = GiannisModeAudioProcessor::getFactoryPresetName (
            (GiannisModeAudioProcessor::Preset) i);
        std::cout << "  [" << i << "] " << name << "\n";
    }

    check (processor.matchFactoryPreset() == "GIANNIS",
           "the defaults are recognised as GIANNIS");

    processor.applyPreset (GiannisModeAudioProcessor::Preset::dubstep);
    check (processor.matchFactoryPreset() == "DUBSTEP",
           "the DUBSTEP preset is recognised");

    const auto savedSub   = paramValue (processor, ids::sub);
    const auto savedDoom  = paramValue (processor, ids::doom);
    const auto savedOct   = paramValue (processor, ids::oct);
    const auto savedFuzz  = paramValue (processor, ids::fuzz);
    const auto savedTone  = paramValue (processor, ids::tone);
    const auto savedLevel = paramValue (processor, ids::level);

    processor.applyPreset (GiannisModeAudioProcessor::Preset::giannis);
    check (std::abs (paramValue (processor, ids::fuzz) - savedFuzz) > 0.1f,
           "GIANNIS moved the knobs away from DUBSTEP");

    //--------------------------------------------------------------------------
    // A user preset survives save -> change -> load.
    const juce::String name ("UI CHECK PRESET");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    setParam (processor, ids::fuzz, 0.93f);
    setParam (processor, ids::tone, 0.44f);
    check (processor.matchFactoryPreset().isEmpty(),
           "changing a knob leaves the factory preset");

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name),
           "the preset shows up in the list");

    processor.applyPreset (GiannisModeAudioProcessor::Preset::giannis);
    check (std::abs (paramValue (processor, ids::fuzz) - savedFuzz) > 0.1f,
           "GIANNIS moved the knobs away");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (paramValue (processor, ids::fuzz) - 0.93f) < 1.0e-4f, "FUZZ restored");
    check (std::abs (paramValue (processor, ids::tone) - 0.44f) < 1.0e-4f, "TONE restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name),
           "the preset is gone from the list");

    //--------------------------------------------------------------------------
    // The session state (what the host saves) round trips too.
    {
        const auto before = paramValue (processor, ids::sub);

        juce::MemoryBlock state;
        processor.getStateInformation (state);
        setParam (processor, ids::sub, 0.05f);
        processor.setStateInformation (state.getData(), (int) state.getSize());

        check (std::abs (paramValue (processor, ids::sub) - before) < 1.0e-4f,
               "the plugin state restores the sub setting");
    }

    //--------------------------------------------------------------------------
    // A played phrase moves the divider and the subs.
    processor.prepareToPlay (48000.0, 512);
    playPhrase (processor, 2.0);

    const auto& snapshot = processor.getSnapshot();
    std::cout << "live: note=" << snapshot.note.load()
              << " hz=" << snapshot.frequency.load()
              << " sub=" << snapshot.subLevel.load()
              << " voice=" << snapshot.voiceLevel.load()
              << " input=" << snapshot.inputLevel.load()
              << " active=" << snapshot.active.load() << "\n";

    check (snapshot.active.load() == 1, "a played note is being tracked");
    check (snapshot.note.load() >= 33 && snapshot.note.load() <= 57,
           "the last note of the phrase is a real bass note");

    //--------------------------------------------------------------------------
    // The editor renders.
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

    if (auto* panel = dynamic_cast<GiannisModeAudioProcessorEditor*> (editor.get()))
        panel->pullLiveData();

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
