// Loads the built HarmonicResonatorMode VST3 exactly like a host would and
// checks that it behaves: the bundle is found, the plugin is described, an
// instance is created, it accepts audio, and the resonators keep ringing after
// the input stops.
//
// This is the closest thing to "it loads in Ableton" that can run without a
// DAW: JUCE's VST3 host code opens the bundle, talks to the factory and builds
// a real instance.
//
//   harmonicresonator_vst3_check [path-to-vst3-bundle]

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <iostream>

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
                              .getChildFile ("HarmonicResonatorMode_artefacts")
                              .getChildFile ("Release")
                              .getChildFile ("VST3")
                              .getChildFile ("HarmonicResonatorMode.vst3");

    if (relative.exists())
        return relative;

    // The copy the build drops in the user VST3 folder.
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Programs")
        .getChildFile ("Common")
        .getChildFile ("VST3")
        .getChildFile ("HarmonicResonatorMode.vst3");
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto bundle = findBundle (argc, argv);

    std::cout << "HarmonicResonatorMode VST3 load check" << std::endl;
    std::cout << "bundle: " << bundle.getFullPathName() << "\n\n" << std::endl;

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

    check (description.name == "HarmonicResonatorMode",
           "the plugin is called HarmonicResonatorMode");
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

    check (instance->getName() == "HarmonicResonatorMode",
           "the instance calls itself HarmonicResonatorMode");

    check (instance->getBusCount (true) >= 1, "the instance has an input bus");
    check (instance->getBusCount (false) >= 1, "the instance has an output bus");
    check (instance->getBus (true, 0)->getNumberOfChannels() == 2, "the input is stereo");
    check (instance->getBus (false, 0)->getNumberOfChannels() == 2, "the output is stereo");

    //--------------------------------------------------------------------------
    // The parameters.
    //
    // The plug-in itself has 30. JUCE's VST3 wrapper adds one more, the
    // standard VST3 bypass switch, when the plug-in does not have one of its
    // own, so the host sees 31.
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

        check (ours == 30, "the plug-in exposes its 30 parameters (" + juce::String (ours) + ")");
        check (bypasses == 1, "the host adds the standard VST3 bypass switch");
    }

    //--------------------------------------------------------------------------
    // It makes a sound, and the bank keeps ringing after the input stops.
    instance->prepareToPlay (48000.0, 512);

    const int blockSize = 512;
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    bool finite = true;
    double dryEnergy = 0.0;
    double tailEnergy = 0.0;
    float loudest = 0.0f;

    // One burst at the very beginning, then two seconds of silence: whatever
    // comes out after the burst was made by the resonators.
    for (int block = 0; block < 200; ++block)
    {
        buffer.clear();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto position = block * blockSize + i;

            if (position < 2400)     // 50 ms of a 220 Hz tone
            {
                const auto value = 0.6f * (float) std::sin (2.0 * 3.14159265358979
                                                            * 220.0 * (double) position / 48000.0);
                buffer.setSample (0, i, value);
                buffer.setSample (1, i, value);
            }
        }

        instance->processBlock (buffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
            {
                const auto value = buffer.getSample (channel, i);

                if (! std::isfinite (value))
                    finite = false;

                loudest = std::max (loudest, std::abs (value));

                const auto position = block * blockSize + i;

                if (position < 2400)
                    dryEnergy += (double) value * (double) value;
                else if (position >= 24000 && position < 72000)   // 0.5 s .. 1.5 s
                    tailEnergy += (double) value * (double) value;
            }
    }

    check (finite, "the output never turns into NaN or infinity");
    check (dryEnergy > 0.0, "the signal comes out of the plugin");
    check (loudest < 1.2f, "the output stays inside the rails");

    std::cout << "dry energy: " << dryEnergy
              << ", tail energy 0.5-1.5 s: " << tailEnergy
              << ", loudest sample: " << loudest << "\n";

    check (tailEnergy > 1.0e-6, "the resonators ring after the input stops");

    instance->releaseResources();

    std::cout << (failures == 0 ? "\nall checks passed\n"
                                : "\n" + juce::String (failures) + " check(s) failed\n");

    return failures == 0 ? 0 : 1;
}
