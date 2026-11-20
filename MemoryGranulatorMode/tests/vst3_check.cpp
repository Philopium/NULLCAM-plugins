// Loads the built MemoryGranulatorMode VST3 exactly like a host would and
// checks the real thing: the bundle is found, the plugin is described, an
// instance is created, it exposes a MIDI input event bus, it captures and
// freezes material, and two otherwise identical instances react differently
// to two real MIDI notes.
//
//   memorygranulator_vst3_check [path-to-vst3-bundle]

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <iostream>
#include <memory>

namespace
{
int failures = 0;

void check (bool condition, const juce::String& what)
{
    std::cout << (condition ? "  ok   " : "  FAIL ") << what << std::endl;

    if (! condition)
        ++failures;
}

juce::File findBundle (int argc, char* argv[])
{
    if (argc > 1)
        return juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);

    const auto relative = juce::File::getCurrentWorkingDirectory()
                              .getChildFile ("build")
                              .getChildFile ("MemoryGranulatorMode_artefacts")
                              .getChildFile ("Release")
                              .getChildFile ("VST3")
                              .getChildFile ("MemoryGranulatorMode.vst3");

    if (relative.exists())
        return relative;

    return juce::File::getSpecialLocation (juce::File::windowsLocalAppData)
        .getChildFile ("Programs")
        .getChildFile ("Common")
        .getChildFile ("VST3")
        .getChildFile ("MemoryGranulatorMode.vst3");
}

juce::RangedAudioParameter* findParameter (juce::AudioProcessor& processor,
                                            const juce::String& name)
{
    for (auto* parameter : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            if (ranged->getName (48) == name)
                return ranged;

    return nullptr;
}

void setChoice (juce::AudioProcessor& processor, const juce::String& name, int choice)
{
    if (auto* parameter = findParameter (processor, name))
        if (auto* choices = dynamic_cast<juce::AudioParameterChoice*> (parameter))
            parameter->setValueNotifyingHost (choices->convertTo0to1 ((float) choice));
}

void setFloat (juce::AudioProcessor& processor, const juce::String& name, float value)
{
    if (auto* parameter = findParameter (processor, name))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void setBool (juce::AudioProcessor& processor, const juce::String& name, bool value)
{
    if (auto* parameter = findParameter (processor, name))
        parameter->setValueNotifyingHost (value ? 1.0f : 0.0f);
}

std::vector<float> renderWithNote (const juce::PluginDescription& description, int note)
{
    juce::VST3PluginFormatHeadless format;
    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> instance =
        format.createInstanceFromDescription (description, 48000.0, 512, error);

    if (instance == nullptr)
        return {};

    instance->prepareToPlay (48000.0, 512);
    setChoice (*instance, "PITCH SOURCE", 0);       // MIDI
    setFloat (*instance, "DRY / WET", 1.0f);
    setFloat (*instance, "GRAIN SIZE", 0.05f);
    setFloat (*instance, "DENSITY", 24.0f);
    setFloat (*instance, "SPRAY", 0.0f);
    setFloat (*instance, "GRAIN GAIN", 0.75f);

    const int blockSize = 512;
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    // Capture one deterministic phrase into the circular memory.
    for (int block = 0; block < 38; ++block)
    {
        buffer.clear();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto position = block * blockSize + i;
            const auto t = (double) position / 48000.0;
            const auto value = 0.42f * (float) std::sin (2.0 * 3.14159265358979 * 220.0 * t);
            buffer.setSample (0, i, value);
            buffer.setSample (1, i, value);
        }

        instance->processBlock (buffer, midi);
    }

    setBool (*instance, "FREEZE", true);
    setBool (*instance, "CAPTURE", false);

    std::vector<float> tail;
    tail.reserve ((std::size_t) blockSize * 80);

    for (int block = 0; block < 80; ++block)
    {
        buffer.clear();
        midi.clear();

        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.92f), 0);

        if (block == 40)
            midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);

        instance->processBlock (buffer, midi);

        for (int i = 0; i < blockSize; ++i)
            tail.push_back (buffer.getSample (0, i));
    }

    instance->releaseResources();
    return tail;
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto bundle = findBundle (argc, argv);

    std::cout << "MemoryGranulatorMode VST3 load check" << std::endl;
    std::cout << "bundle: " << bundle.getFullPathName() << "\n\n" << std::endl;

    if (! bundle.exists())
    {
        std::cout << "  FAIL  the bundle is not there: build the VST3 first\n";
        return 1;
    }

    juce::VST3PluginFormatHeadless format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile (descriptions, bundle.getFullPathName());

    check (descriptions.size() == 1,
           "the bundle declares exactly one plugin (" + juce::String (descriptions.size()) + ")");

    if (descriptions.isEmpty())
        return 1;

    const auto& description = *descriptions[0];

    std::cout << "name: " << description.name
              << ", maker: " << description.manufacturerName
              << ", category: " << description.category
              << ", version: " << description.version
              << ", in/out: " << description.numInputChannels
              << "/" << description.numOutputChannels << "\n";

    check (description.name == "MemoryGranulatorMode", "the plugin is called MemoryGranulatorMode");
    check (description.manufacturerName == "NULLCAM", "the maker is NULLCAM");
    check (description.version == "1.5.0", "the bundle carries release version 1.5.0");
    check (description.category == "Fx", "it is an effect, not an instrument");
    check (! description.isInstrument, "it is not a synth");

    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> instance =
        format.createInstanceFromDescription (description, 48000.0, 512, error);

    if (instance == nullptr)
        std::cout << "  (the host said: " << error << ")\n";

    check (instance != nullptr, "the host can create an instance of the plugin");

    if (instance == nullptr)
        return 1;

    check (instance->getName() == "MemoryGranulatorMode",
           "the instance calls itself MemoryGranulatorMode");
    check (instance->acceptsMidi(), "the instance advertises a genuine MIDI input");
    check (instance->getBusCount (true) >= 1, "the instance has an input bus");
    check (instance->getBusCount (false) >= 1, "the instance has an output bus");
    check (instance->getBus (true, 0)->getNumberOfChannels() == 2, "the input is stereo");
    check (instance->getBus (false, 0)->getNumberOfChannels() == 2, "the output is stereo");

    {
        int ours = 0;
        int bypasses = 0;

        for (const auto* p : instance->getParameters())
        {
            if (p->getName (48) == "Bypass")
                ++bypasses;
            else
                ++ours;
        }

        check (ours == 99, "the plug-in exposes its 99 parameters (" + juce::String (ours) + ")");
        check (bypasses == 1, "the host adds the standard VST3 bypass switch");
    }

    //--------------------------------------------------------------------------
    // The real MIDI event bus: the same recorded phrase and two different
    // note-on events must produce two different frozen grain clouds.
    const auto c4 = renderWithNote (description, 60);
    const auto c5 = renderWithNote (description, 72);

    check (! c4.empty() && ! c5.empty(), "both VST3 instances render the MIDI test");

    double difference = 0.0;
    double c4Energy = 0.0;

    if (! c4.empty() && c4.size() == c5.size())
    {
        for (std::size_t i = 0; i < c4.size(); ++i)
        {
            const auto d = (double) c4[i] - (double) c5[i];
            difference += d * d;
            c4Energy += (double) c4[i] * (double) c4[i];
        }

        difference = std::sqrt (difference / (double) c4.size());
    }

    std::cout << "C4 energy: " << c4Energy << ", C4/C5 RMS difference: " << difference << "\n";
    check (c4Energy > 1.0e-4, "the frozen VST3 memory makes audible grains");
    check (difference > 0.001,
           "real VST3 MIDI note events change the grains independently of the captured audio");

    instance->releaseResources();

    std::cout << (failures == 0 ? "\nall checks passed\n"
                                : "\n" + juce::String (failures) + " check(s) failed\n");

    return failures == 0 ? 0 : 1;
}
