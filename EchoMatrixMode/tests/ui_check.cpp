// Renders the EchoMatrixMode faceplate without a host and checks the things a
// person would notice: the eight factory presets are really there, a user
// preset survives save -> change -> load, the plugin state round trips, and
// the panel lays itself out.
//
//   echomatrix_ui_check [out.png] [scale]
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

float paramValue (EchoMatrixModeAudioProcessor& p, const juce::String& id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (EchoMatrixModeAudioProcessor& p, const juce::String& id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

// The step editor owns the only two horizontal sliders on the panel.
juce::Slider* nthHorizontalSlider (juce::Component& parent, int index)
{
    int seen = 0;

    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        if (auto* slider = dynamic_cast<juce::Slider*> (parent.getChildComponent (i)))
        {
            if (! slider->isHorizontal())
                continue;

            if (seen++ == index)
                return slider;
        }
    }

    return nullptr;
}

// The text box of a slider is a Label living inside it.
juce::String sliderText (juce::Slider& slider)
{
    for (int i = 0; i < slider.getNumChildComponents(); ++i)
        if (auto* label = dynamic_cast<juce::Label*> (slider.getChildComponent (i)))
            return label->getText();

    return {};
}

// A short phrase, to make the panel show something alive.
void playPhrase (EchoMatrixModeAudioProcessor& processor, double seconds)
{
    const auto blockSize = 512;
    const auto blocks = (int) (seconds * 48000.0 / blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < blocks; ++block)
    {
        juce::AudioBuffer<float> silence (2, blockSize);
        silence.clear();

        buffer.clear();
        buffer.copyFrom (0, 0, silence, 0, 0, blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            const auto t = (double) (block * blockSize + i) / 48000.0;
            const auto value = 0.4f * (float) std::sin (2.0 * 3.14159265358979 * 220.0 * t)
                               * (float) std::exp (-2.0 * std::fmod (t, 0.25));

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

    EchoMatrixModeAudioProcessor processor;

    //--------------------------------------------------------------------------
    // The eight factory presets.
    std::cout << "factory presets:\n";

    for (int i = 0; i < EchoMatrixModeAudioProcessor::kNumPresets; ++i)
    {
        const auto name = EchoMatrixModeAudioProcessor::getFactoryPresetName (
            (EchoMatrixModeAudioProcessor::Preset) i);
        std::cout << "  [" << i << "] " << name << "\n";
    }

    check (processor.matchFactoryPreset() == "DOTTED EIGHTH",
           "the defaults are recognised as DOTTED EIGHTH");

    processor.applyPreset (EchoMatrixModeAudioProcessor::Preset::pingPong);
    check (processor.matchFactoryPreset() == "PING PONG",
           "the PING PONG preset is recognised");

    const auto savedFeedback = paramValue (processor, ids::feedback);
    const auto savedMix = paramValue (processor, ids::mix);
    const auto savedDelay = paramValue (processor, ids::delayTime);

    processor.applyPreset (EchoMatrixModeAudioProcessor::Preset::dottedEighth);
    check (std::abs (paramValue (processor, ids::feedback) - savedFeedback) > 0.05f,
           "DOTTED EIGHTH moved the knobs away from PING PONG");
    check (std::abs (paramValue (processor, ids::delayTime) - savedDelay) > 0.05f,
           "the delay time changed too");

    //--------------------------------------------------------------------------
    // A user preset survives save -> change -> load.
    const juce::String name ("UI CHECK PRESET");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    setParam (processor, ids::feedback, 0.91f);
    setParam (processor, ids::mix, 0.22f);
    check (processor.matchFactoryPreset().isEmpty(),
           "changing a knob leaves the factory preset");

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name),
           "the preset shows up in the list");

    processor.applyPreset (EchoMatrixModeAudioProcessor::Preset::dottedEighth);
    check (std::abs (paramValue (processor, ids::mix) - savedMix) > 0.05f,
           "DOTTED EIGHTH moved the mix away");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (paramValue (processor, ids::feedback) - 0.91f) < 1.0e-4f,
           "FEEDBACK restored");
    check (std::abs (paramValue (processor, ids::mix) - 0.22f) < 1.0e-4f, "MIX restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name),
           "the preset is gone from the list");

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
    // The panel is alive.
    processor.prepareToPlay (48000.0, 512);
    playPhrase (processor, 2.0);

    const auto& snapshot = processor.getSnapshot();
    std::cout << "live: voices=" << snapshot.activeEvents.load()
              << " wet=" << snapshot.wetLevel.load()
              << " step=" << snapshot.currentStep.load() << "\n";

    check (snapshot.currentStep.load() >= 0 && snapshot.currentStep.load() < 16,
           "the pattern playhead is inside the pattern");

    //--------------------------------------------------------------------------
    // The editor renders.
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

    if (auto* panel = dynamic_cast<EchoMatrixModeAudioProcessorEditor*> (editor.get()))
        panel->pullLiveData();

    //--------------------------------------------------------------------------
    // The step editor sliders show the step they are editing, and they say it
    // in a readable way.
    {
        processor.applyPreset (EchoMatrixModeAudioProcessor::Preset::dottedEighth);

        auto* levelSlider = nthHorizontalSlider (*editor, 0);
        auto* panSlider   = nthHorizontalSlider (*editor, 1);

        check (levelSlider != nullptr, "the step editor has a LEVEL slider");
        check (panSlider != nullptr, "the step editor has a PAN slider");

        if (levelSlider != nullptr && panSlider != nullptr)
        {
            const auto levelId = EchoMatrixModeAudioProcessor::stepParamId (ids::stepLevel, 0);
            const auto panId   = EchoMatrixModeAudioProcessor::stepParamId (ids::stepPan, 0);

            std::cout << "step 1: level=" << levelSlider->getValue()
                      << " (\"" << sliderText (*levelSlider) << "\")"
                      << ", pan=" << panSlider->getValue()
                      << " (\"" << sliderText (*panSlider) << "\")\n";

            check (std::abs (levelSlider->getValue() - paramValue (processor, levelId)) < 1.0e-4f,
                   "the LEVEL slider follows the parameter");
            check (std::abs (panSlider->getValue() - paramValue (processor, panId)) < 1.0e-4f,
                   "the PAN slider follows the parameter");

            // The read-out follows the factory value of the step.
            const auto expectedLevel = juce::String ((int) (paramValue (processor, levelId) * 100.0f + 0.5f))
                                       + " %";

            check (sliderText (*levelSlider) == expectedLevel,
                   "the LEVEL read-out is written as a percentage");
            check (sliderText (*panSlider) == "C",
                   "a centred step reads C on the PAN read-out");
        }
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
