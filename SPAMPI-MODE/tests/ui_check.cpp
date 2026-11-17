// Renders the plugin faceplate without a host and checks the things a person
// would notice: the twelve recordings are really there and really pitched, the
// root follows the selected recording, a played note moves the sampler, a user
// preset survives a round trip, and a file dropped on LOAD SAMPLE is imported.
//
//   spampi_ui_check [out.png] [scale]
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
                  << child->getBounds().toString()
                  << "  " << typeid (*child).name() << "\n";

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

float paramValue (SpampiModeAudioProcessor& p, const char* id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (SpampiModeAudioProcessor& p, const char* id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

// A short recording of a plucked note, written by hand, to test the importer.
juce::File writeTestRecording (const juce::File& file, double hz)
{
    file.deleteFile();

    auto stream = file.createOutputStream();

    if (stream == nullptr)
        return {};

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.get(), 48000.0, 1, 16, {}, 0));

    if (writer == nullptr)
        return {};

    stream.release();   // the writer owns it now

    juce::AudioBuffer<float> buffer (1, 48000);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto t = (double) i / 48000.0;
        const auto value = std::sin (2.0 * 3.14159265358979 * hz * t)
                           + 0.4 * std::sin (4.0 * 3.14159265358979 * hz * t);

        buffer.setSample (0, i, (float) (0.35 * value));
    }

    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    writer.reset();

    return file;
}

void playPhrase (SpampiModeAudioProcessor& processor, double seconds)
{
    const double notes[] = { 220.0, 261.63, 329.63, 392.0 };
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        const auto note = notes[(block / 24) % 4];

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto value = 0.3 * (std::sin (2.0 * 3.14159265358979 * note * t)
                                      + 0.4 * std::sin (4.0 * 3.14159265358979 * note * t));

            buffer.setSample (0, i, (float) value);
            buffer.setSample (1, i, (float) value);
        }

        processor.processBlock (buffer, midi);
    }
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    SpampiModeAudioProcessor processor;

    //--------------------------------------------------------------------------
    // The twelve recordings, and the pitch each one was measured at.
    std::cout << "recordings:\n";

    const auto count = processor.getNumSamples();
    check (count >= SpampiModeAudioProcessor::kNumPresets,
           "at least the twelve factory recordings are loaded");

    check (processor.getSampleName (0) == "MONKEY HOWL", "the first recording is the monkey");

    for (int i = 0; i < count; ++i)
    {
        const auto midi = processor.getSampleMeasuredMidi (i);
        std::cout << "  [" << i << "] " << processor.getSampleName (i)
                  << "  measured: "
                  << (midi >= 0 ? juce::String (spampi::noteLabel (midi).c_str()) : juce::String ("no pitch"))
                  << "\n";

        if (midi >= 0)
            check (midi >= 21 && midi <= 108, "recording " + juce::String (i) + " has a playable root");
    }

    //--------------------------------------------------------------------------
    // The root follows the recording that is selected.
    const auto rootZero = (int) paramValue (processor, ids::root);
    check (rootZero == processor.getSampleMeasuredMidi (0),
           "the default root is the pitch of the default recording");

    setParam (processor, ids::sampleIndex, 2.0f);
    processor.sampleChosenByUser();
    const auto rootTwo = (int) paramValue (processor, ids::root);
    check (rootTwo == processor.getSampleMeasuredMidi (2),
           "choosing another recording moves the root to its pitch");

    processor.applyPreset (SpampiModeAudioProcessor::Preset::monkey);

    //--------------------------------------------------------------------------
    // A played phrase moves the sampler.
    processor.prepareToPlay (48000.0, 512);
    playPhrase (processor, 2.0);

    const auto& snapshot = processor.getSnapshot();
    std::cout << "live: note=" << snapshot.note.load()
              << " hz=" << snapshot.frequency.load()
              << " rate=" << snapshot.playbackRate.load()
              << " active=" << snapshot.active.load() << "\n";

    check (snapshot.active.load() == 1, "a played note is being followed");
    check (snapshot.note.load() >= 55.0f && snapshot.note.load() <= 80.0f,
           "the last note of the phrase is a real note");
    check (snapshot.playbackRate.load() > 0.2f && snapshot.playbackRate.load() < 5.0f,
           "and the recording is being played at a musical speed");

    //--------------------------------------------------------------------------
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

    // The editor pulls the trace and the readouts into the panel.
    if (auto* panel = dynamic_cast<SpampiModeAudioProcessorEditor*> (editor.get()))
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

    //--------------------------------------------------------------------------
    // The factory presets, and a user preset surviving save -> change -> load.
    std::cout << "\npresets in " << SpampiModeAudioProcessor::getUserPresetFolder().getFullPathName() << "\n";

    const juce::String name ("UI CHECK PRESET");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    check (processor.matchFactoryPreset() == "MONKEY", "the defaults are recognised as MONKEY");

    processor.applyPreset (SpampiModeAudioProcessor::Preset::laugh);
    check (processor.matchFactoryPreset() == "EVIL LAUGH", "the EVIL LAUGH preset is recognised");

    setParam (processor, ids::machine, 0.93f);
    setParam (processor, ids::spread, 0.44f);
    check (processor.matchFactoryPreset().isEmpty(), "changing a knob leaves the factory preset");

    const auto savedMachine = paramValue (processor, ids::machine);
    const auto savedSpread = paramValue (processor, ids::spread);
    const auto savedSample = (int) paramValue (processor, ids::sampleIndex);

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name), "the preset shows up in the list");

    processor.applyPreset (SpampiModeAudioProcessor::Preset::monkey);
    check (std::abs (paramValue (processor, ids::machine) - savedMachine) > 0.1f,
           "MONKEY moved the knobs away");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (processor.getSampleIndexForName (processor.getSampleName (savedSample)) >= 0,
           "the saved recording is still in the list");
    check (std::abs (paramValue (processor, ids::machine) - savedMachine) < 1.0e-4f, "MACHINE restored");
    check (std::abs (paramValue (processor, ids::spread) - savedSpread) < 1.0e-4f, "SPREAD restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name), "the preset is gone from the list");

    //--------------------------------------------------------------------------
    // The session state (what the host saves) round trips too.
    {
        const auto before = paramValue (processor, ids::machine);

        juce::MemoryBlock state;
        processor.getStateInformation (state);
        setParam (processor, ids::machine, 0.05f);
        processor.setStateInformation (state.getData(), (int) state.getSize());

        check (std::abs (paramValue (processor, ids::machine) - before) < 1.0e-4f,
               "the plugin state restores the machine setting");
    }

    //--------------------------------------------------------------------------
    // Importing a recording: copied into the user folder, measured, selectable.
    {
        const auto source = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("spampi ui check.wav");

        check (writeTestRecording (source, 110.0).existsAsFile(), "a test recording was written");

        const auto before = processor.getNumSamples();
        const auto index = processor.importUserSample (source);

        check (index == before, "importUserSample returns the new index");
        check (processor.getNumSamples() == before + 1, "the recording is added to the list");
        check (processor.getSampleName (index) == "spampi ui check", "the recording keeps its name");
        check (juce::File (processor.getSamplePath (index)).existsAsFile(),
               "the recording was copied into the user sample folder");
        check (processor.getSampleMeasuredMidi (index) == 45,
               "the imported A2 recording is measured as A2");

        // Do not leave the check recording behind in the real sample folder.
        juce::File (processor.getSamplePath (index)).deleteFile();
        source.deleteFile();
    }

    std::cout << (failures == 0 ? "\nall checks passed\n"
                                : "\n" + juce::String (failures) + " check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
