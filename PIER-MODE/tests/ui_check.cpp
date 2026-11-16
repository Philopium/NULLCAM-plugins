// Renders the plugin faceplate without a host and checks the preset system.
//
//   nullcam_ui_check [out.png] [scale]
//
// It builds the real editor, prints every child it placed (a quick way to see
// whether a control was given a size at all) and writes the panel to a PNG so the
// layout can be looked at, diffed and sent around. Everything here is a dev tool:
// it is not part of the installed plugin.

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
                  << "  " << typeid (*child).name();

        if (auto* slider = dynamic_cast<juce::Slider*> (child))
        {
            std::cout << "  text=\"" << slider->getTextFromValue (slider->getValue()) << "\"";

            if (slider->getBounds().isEmpty())
                ++failures;
        }

        std::cout << "\n";
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

float paramValue (NullcamChainAudioProcessor& p, const char* id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (NullcamChainAudioProcessor& p, const char* id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    NullcamChainAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

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
    // A user preset has to survive save -> change -> load, and leave no file behind.
    std::cout << "\npresets in " << NullcamChainAudioProcessor::getUserPresetFolder().getFullPathName() << "\n";

    const juce::String name ("UI CHECK PRESET");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    check (processor.matchFactoryPreset() == "MOTION", "defaults are recognised as MOTION");
    processor.applyPreset (NullcamChainAudioProcessor::Preset::motion);
    setParam (processor, ids::hpf, 412.0f);
    setParam (processor, ids::crush, 0.55f);
    setParam (processor, ids::ring, 0.25f);

    const auto savedHpf = paramValue (processor, ids::hpf);
    const auto savedCrush = paramValue (processor, ids::crush);
    const auto savedRing = paramValue (processor, ids::ring);

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name), "preset shows up in the list");

    processor.applyPreset (NullcamChainAudioProcessor::Preset::clean);
    check (std::abs (paramValue (processor, ids::hpf) - savedHpf) > 1.0f, "clean preset moved the knobs");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (paramValue (processor, ids::hpf) - savedHpf) < 0.5f,
           "HPF back to " + juce::String (savedHpf, 1));
    check (std::abs (paramValue (processor, ids::crush) - savedCrush) < 1.0e-4f, "CRUSH restored");
    check (std::abs (paramValue (processor, ids::ring) - savedRing) < 1.0e-4f, "RING restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name), "preset gone from the list");

    std::cout << (failures == 0 ? "\nall checks passed\n" : "\n" + juce::String (failures) + " check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
