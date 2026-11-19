// Loads the built EchoMatrixMode VST3 exactly like a host would and checks that
// it behaves: the bundle is found, the plugin is described, an instance is
// created, it accepts audio, and the delay actually repeats.
//
// This is the closest thing to "it loads in Ableton" that can run without a
// DAW: JUCE's VST3 host code opens the bundle, talks to the factory and builds
// a real instance.
//
//   echomatrix_vst3_check [path-to-vst3-bundle]

#include <juce_audio_processors/juce_audio_processors.h>

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

juce::File findBundle (int argc, char* argv[])
{
    if (argc > 1)
        return juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);

    const auto relative = juce::File::getCurrentWorkingDirectory()
                              .getChildFile ("build")
                              .getChildFile ("EchoMatrixMode_artefacts")
                              .getChildFile ("Release")
                              .getChildFile ("VST3")
                              .getChildFile ("EchoMatrixMode.vst3");

    if (relative.exists())
        return relative;

    // The copy the build drops in the user VST3 folder.
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Programs")
        .getChildFile ("Common")
        .getChildFile ("VST3")
        .getChildFile ("EchoMatrixMode.vst3");
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto bundle = findBundle (argc, argv);

    std::cout << "EchoMatrixMode VST3 load check\n";
    std::cout << "bundle: " << bundle.getFullPathName() << "\n\n";

    if (! bundle.exists())
    {
        std::cout << "  FAIL  the bundle is not there: build the VST3 first\n";
        return 1;
    }

    //--------------------------------------------------------------------------
    // The bundle is described.
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

    check (description.name == "EchoMatrixMode", "the plugin is called EchoMatrixMode");
    check (description.manufacturerName == "NULLCAM", "the maker is NULLCAM");
    check (description.category == "Fx", "it is an effect, not an instrument");
    check (! description.isInstrument, "it is not a synth");

    //--------------------------------------------------------------------------
    // An instance is created and it takes audio.
    std::unique_ptr<juce::AudioPluginInstance> instance;

    {
        juce::String error;
        instance = format.createInstanceFromDescription (description, 48000.0, 512, error);

        if (instance == nullptr)
            std::cout << "  (the host said: " << error << ")\n";
    }

    check (instance != nullptr, "the host can create an instance of the plugin");

    if (instance == nullptr)
        return 1;

    check (instance->getName() == "EchoMatrixMode",
           "the instance calls itself EchoMatrixMode");

    check (instance->getBusCount (true) >= 1, "the instance has an input bus");
    check (instance->getBusCount (false) >= 1, "the instance has an output bus");
    check (instance->getBus (true, 0)->getNumberOfChannels() == 2,
           "the input is stereo");
    check (instance->getBus (false, 0)->getNumberOfChannels() == 2,
           "the output is stereo");

    check (instance->getParameters().size() > 40,
           "the plugin exposes its parameters (" + juce::String (instance->getParameters().size()) + ")");

    //--------------------------------------------------------------------------
    // It makes a sound, and the delay repeats it.
    instance->prepareToPlay (48000.0, 512);

    const int blockSize = 512;
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    bool finite = true;
    double dryEnergy = 0.0;
    double tailEnergy = 0.0;

    // The default preset is DOTTED EIGHTH: the repeat lands 375 ms after the
    // input, so the tail window starts there and runs for one grain.
    const auto tailFrom = (int) (0.375 * 48000.0);
    const auto tailTo = tailFrom + (int) (0.20 * 48000.0);

    // A click at the very beginning, then silence: the repeat has to show up
    // one delay time later.
    for (int block = 0; block < 200; ++block)
    {
        buffer.clear();

        if (block == 0)
            for (int i = 0; i < 8; ++i)
            {
                buffer.setSample (0, i, 1.0f);
                buffer.setSample (1, i, 1.0f);
            }

        instance->processBlock (buffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
            {
                const auto value = buffer.getSample (channel, i);

                if (! std::isfinite (value))
                    finite = false;

                const auto position = block * blockSize + i;

                if (position < 512)
                    dryEnergy += (double) value * (double) value;
                else if (position >= tailFrom && position < tailTo)
                    tailEnergy += (double) value * (double) value;
            }
    }

    check (finite, "the output never turns into NaN or infinity");
    check (dryEnergy > 0.0, "the signal comes out of the plugin");

    std::cout << "dry energy: " << dryEnergy
              << ", tail energy around 375 ms: " << tailEnergy << "\n";

    check (tailEnergy > 0.0, "the delay repeats the input 375 ms later");

    instance->releaseResources();

    std::cout << (failures == 0 ? "\nall checks passed\n"
                                : "\n" + juce::String (failures) + " check(s) failed\n");

    return failures == 0 ? 0 : 1;
}
