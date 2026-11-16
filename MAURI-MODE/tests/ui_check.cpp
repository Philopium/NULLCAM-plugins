// Renders the plugin faceplate without a host and checks the preset system.
//
//   mauri_ui_check [out.png] [scale]
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

float paramValue (MauriModeAudioProcessor& p, const char* id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

void setParam (MauriModeAudioProcessor& p, const char* id, float realValue)
{
    if (auto* parameter = p.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    MauriModeAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    if (editor == nullptr)
    {
        std::cout << "no editor\n";
        return 1;
    }

    dumpChildren (*editor);

    //--------------------------------------------------------------------------
    // Feed the plugin a played phrase so the panel is rendered in a live state:
    // the scope has a waveform in it and the grid shows the generated pattern.
    {
        processor.apvts.getParameter (ids::rate)->setValueNotifyingHost (
            processor.apvts.getParameter (ids::rate)->convertTo0to1 (1.0f));   // 1/8
        processor.apvts.getParameter (ids::style)->setValueNotifyingHost (
            processor.apvts.getParameter (ids::style)->convertTo0to1 (0.0f));  // GROOVE

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        for (int block = 0; block < 200; ++block)
        {
            const auto t0 = block * 512 / 48000.0;

            for (int i = 0; i < 512; ++i)
            {
                const auto t = t0 + (double) i / 48000.0;
                const auto beat = std::fmod (t, 0.25);            // eighth notes at 120 bpm
                const auto hit = std::exp (-beat * 22.0);
                const auto tone = std::sin (2.0 * 3.14159265358979 * 180.0 * t)
                                  * 0.25 + std::sin (2.0 * 3.14159265358979 * 90.0 * t) * 0.15;
                const auto v = tone * (0.08 + 0.92 * hit);
                buffer.setSample (0, i, (float) v);
                buffer.setSample (1, i, (float) (v * 0.9));
            }

            processor.processBlock (buffer, midi);
        }

        std::cout << "live: onsets=" << processor.getSnapshot().onsets.load()
                  << " energy=" << processor.getSnapshot().patternEnergy.load()
                  << " velocity=" << processor.getSnapshot().velocity.load() << "\n";

        // The editor pulls the scope and the playhead into the panel.
        if (auto* panel = dynamic_cast<MauriModeAudioProcessorEditor*> (editor.get()))
            panel->pullLiveData();

        // Back to the factory sound for the preset checks below.
        processor.applyPreset (MauriModeAudioProcessor::Preset::mauri);
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

    //--------------------------------------------------------------------------
    // A user preset has to survive save -> change -> load, and leave no file behind.
    std::cout << "\npresets in " << MauriModeAudioProcessor::getUserPresetFolder().getFullPathName() << "\n";

    const juce::String name ("UI CHECK PRESET");
    processor.deleteUserPreset (name);   // leftovers from an interrupted run

    check (processor.matchFactoryPreset() == "MAURI", "defaults are recognised as MAURI");
    processor.applyPreset (MauriModeAudioProcessor::Preset::odd78);
    check (processor.matchFactoryPreset() == "ODD 7/8", "7/8 preset is recognised");

    setParam (processor, ids::timeSig, 4.0f);      // 6/8
    setParam (processor, ids::style, 3.0f);        // ROLL
    setParam (processor, ids::slice, 0.42f);
    setParam (processor, ids::dynPat, 0.88f);

    const auto savedSig = paramValue (processor, ids::timeSig);
    const auto savedStyle = paramValue (processor, ids::style);
    const auto savedSlice = paramValue (processor, ids::slice);
    const auto savedDyn = paramValue (processor, ids::dynPat);

    check (processor.saveUserPreset (name), "save \"" + name + "\"");
    check (processor.getUserPresetNames().contains (name), "preset shows up in the list");

    processor.applyPreset (MauriModeAudioProcessor::Preset::mauri);
    check (std::abs (paramValue (processor, ids::timeSig) - savedSig) > 0.5f, "MAURI moved the knobs");

    check (processor.loadUserPreset (name), "load \"" + name + "\"");
    check (std::abs (paramValue (processor, ids::timeSig) - savedSig) < 1.0e-4f, "TIME SIG restored");
    check (std::abs (paramValue (processor, ids::style) - savedStyle) < 1.0e-4f, "STYLE restored");
    check (std::abs (paramValue (processor, ids::slice) - savedSlice) < 1.0e-4f, "SLICE restored");
    check (std::abs (paramValue (processor, ids::dynPat) - savedDyn) < 1.0e-4f, "DYN PAT restored");

    check (processor.deleteUserPreset (name), "delete \"" + name + "\"");
    check (! processor.getUserPresetNames().contains (name), "preset gone from the list");

    std::cout << (failures == 0 ? "\nall checks passed\n" : "\n" + juce::String (failures) + " check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
