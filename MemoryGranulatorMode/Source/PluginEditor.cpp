#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int kWidth = 1440;
constexpr int kHeight = 1024;
constexpr int kMargin = 18;

void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                  float tracking, juce::Justification justification)
{
    g.setFont (g.getCurrentFont().withExtraKerningFactor (tracking));
    g.drawText (text, area, justification, false);
}

void paintCard (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto r = area.toFloat();

    juce::DropShadow (juce::Colour (0x77000000), 18, juce::Point<int> (0, 8))
        .drawForRectangle (g, area);

    juce::ColourGradient fill (MemoryLookAndFeel::cardTop(), r.getX(), r.getY(),
                               MemoryLookAndFeel::card(), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 15.0f);

    g.setGradientFill (MemoryLookAndFeel::accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 14.0f, 1.4f);
}

void paintBar (juce::Graphics& g, juce::Rectangle<int> area, float level,
               juce::Colour colour)
{
    const auto r = area.toFloat();

    g.setColour (juce::Colour (0xff090b10));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (MemoryLookAndFeel::line());
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

    if (level > 0.001f)
    {
        auto lit = r.reduced (2.0f);
        lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth()
                                            * juce::jlimit (0.0f, 1.0f, level)));
        g.setColour (colour);
        g.fillRoundedRectangle (lit, lit.getHeight() * 0.5f);
    }
}
} // namespace

//==============================================================================
juce::Colour MemoryLookAndFeel::base()    { return juce::Colour (0xff07080c); }
juce::Colour MemoryLookAndFeel::card()    { return juce::Colour (0xff11141b); }
juce::Colour MemoryLookAndFeel::cardTop() { return juce::Colour (0xff1b1f28); }
juce::Colour MemoryLookAndFeel::line()    { return juce::Colour (0xff303746); }
juce::Colour MemoryLookAndFeel::ink()     { return juce::Colour (0xfff2f5fb); }
juce::Colour MemoryLookAndFeel::dim()     { return juce::Colour (0xff8b93a6); }
juce::Colour MemoryLookAndFeel::amber()   { return juce::Colour (0xffffb340); }
juce::Colour MemoryLookAndFeel::teal()    { return juce::Colour (0xff3ce0c4); }
juce::Colour MemoryLookAndFeel::coral()   { return juce::Colour (0xffff6f61); }

juce::ColourGradient MemoryLookAndFeel::accentGradient (juce::Rectangle<float> area)
{
    juce::ColourGradient gradient (amber(), area.getX(), area.getCentreY(),
                                   teal(), area.getRight(), area.getCentreY(), false);
    gradient.addColour (0.35, juce::Colour (0xffffe066));
    gradient.addColour (0.70, juce::Colour (0xff8ce8a0));
    return gradient;
}

MemoryLookAndFeel::MemoryLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff14171f));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, amber());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber());
    setColour (juce::PopupMenu::highlightedTextColourId, base());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff14171f));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0d0f14));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, amber().withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, amber());

    setColour (juce::Label::textColourId, ink());
    setColour (juce::ComboBox::textColourId, ink());
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0f1117));
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::arrowColourId, amber());

    setColour (juce::TextButton::buttonColourId, cardTop());
    setColour (juce::TextButton::buttonOnColourId, cardTop().brighter (0.25f));
    setColour (juce::TextButton::textColourOffId, ink());
    setColour (juce::TextButton::textColourOnId, ink());

    setColour (juce::ToggleButton::textColourId, dim());
    setColour (juce::ToggleButton::tickColourId, teal());
    setColour (juce::ToggleButton::tickDisabledColourId, line());
}

juce::Font MemoryLookAndFeel::titleFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font MemoryLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font MemoryLookAndFeel::valueFont (float height) const
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                          height, juce::Font::plain));
}

void MemoryLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width,
                                          int height, float sliderPos,
                                          float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (knob.translated (0.0f, 4.0f).reduced (2.0f));

    const auto radius = knob.getWidth() * 0.5f - 2.0f;
    const auto centre = knob.getCentre();

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (line().withAlpha (0.90f));
    g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path active;
    active.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setGradientFill (accentGradient (knob));
    g.strokePath (active, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    const auto body = knob.reduced (8.0f);
    juce::ColourGradient bodyGradient (cardTop().brighter (0.14f), body.getX(), body.getY(),
                                       card().darker (0.42f), body.getX(), body.getBottom(), false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (body);
    g.setColour (colour.withAlpha (0.55f));
    g.drawEllipse (body.reduced (0.5f), 1.4f);

    const auto tip = juce::Point<float> (centre.x + std::sin (angle) * (radius - 5.0f),
                                         centre.y - std::cos (angle) * (radius - 5.0f));
    const auto root = juce::Point<float> (centre.x + std::sin (angle) * (radius * 0.28f),
                                           centre.y - std::cos (angle) * (radius * 0.28f));
    g.setColour (colour);
    g.drawLine (juce::Line<float> (root, tip), 2.8f);
    g.setColour (juce::Colours::white.withAlpha (0.86f));
    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (tip));
}

juce::Label* MemoryLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (valueFont (8.5f));
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void MemoryLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0f1117));
    g.fillRoundedRectangle (r, 8.0f);
    g.setGradientFill (accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 7.0f, 1.2f);

    const auto x = (float) width - 13.0f;
    const auto y = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (x - 3.5f, y - 2.0f);
    chevron.lineTo (x, y + 2.2f);
    chevron.lineTo (x + 3.5f, y - 2.0f);
    g.setColour (box.isEnabled() ? amber() : dim());
    g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

void MemoryLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto box = juce::Rectangle<float> (0.0f, (bounds.getHeight() - 13.0f) * 0.5f,
                                             13.0f, 13.0f);

    g.setColour (juce::Colour (0xff090b10));
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (button.getToggleState() ? teal() : line());
    g.drawRoundedRectangle (box, 3.0f, 1.2f);

    if (button.getToggleState())
    {
        g.setColour (teal());
        g.fillEllipse (box.reduced (3.5f));
    }

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour (amber().withAlpha (0.20f));
        g.drawRoundedRectangle (bounds.reduced (1.0f), 5.0f, 1.0f);
    }

    g.setColour (button.getToggleState() ? ink() : dim());
    g.setFont (labelFont (8.2f));
    g.drawText (button.getButtonText(),
                bounds.withTrimmedLeft (19.0f).reduced (0.0f, 1.0f),
                juce::Justification::centredLeft, false);
}

juce::Font MemoryLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (9.0f).withExtraKerningFactor (0.04f);
}

juce::Font MemoryLookAndFeel::getPopupMenuFont()
{
    return labelFont (10.0f);
}

void MemoryLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff14171f));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void MemoryLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::backgroundColourId));
    g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 5.0f);
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                      label.getJustificationType(), 1);
}

//==============================================================================
MemoryGranulatorModeAudioProcessorEditor::MemoryGranulatorModeAudioProcessorEditor (
    MemoryGranulatorModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    headerArea     = { kMargin, 8, kWidth - 2 * kMargin, 62 };
    presetBarArea  = { kMargin, 76, kWidth - 2 * kMargin, 38 };
    memoryArea     = { kMargin, 122, 456, 338 };
    granularArea   = { 492, 122, 456, 338 };
    pitchArea      = { 966, 122, 456, 338 };
    harmonicsArea  = { kMargin, 470, 456, 310 };
    spaceArea      = { 492, 470, 456, 310 };
    modulationArea = { 966, 470, 456, 310 };
    outputArea     = { kMargin, 790, kWidth - 2 * kMargin, 216 };

    // ---- memory and evolution ----
    addChoice (ids::memoryMode, memoryGroup, 0);
    addChoice (ids::feedbackSource, memoryGroup, 1);
    addToggle (ids::captureEnable, "CAPTURE", memoryGroup, 0);
    addToggle (ids::freeze, "FREEZE", memoryGroup, 1);
    addToggle (ids::captureArmed, "ARM", memoryGroup, 2);
    addToggle (ids::stereoCapture, "STEREO", memoryGroup, 3);
    addToggle (ids::clearTrigger, "CLEAR", memoryGroup, 4);
    addToggle (ids::releaseTrigger, "RELEASE", memoryGroup, 5);
    addKnob (ids::memoryLength, "LENGTH", memoryGroup, 0, 0);
    addKnob (ids::recLevel, "REC LEVEL", memoryGroup, 1, 0);
    addKnob (ids::feedbackAmount, "MEM FB", memoryGroup, 2, 0);
    addKnob (ids::feedbackStability, "STABILITY", memoryGroup, 0, 1);
    addKnob (ids::feedbackFilter, "FB FILTER", memoryGroup, 1, 1);
    addKnob (ids::feedbackDamping, "FB DAMP", memoryGroup, 2, 1);
    addKnob (ids::feedbackSaturation, "FB DRIVE", memoryGroup, 0, 2);
    addKnob (ids::feedbackDiffusion, "FB DIFF", memoryGroup, 1, 2);
    addKnob (ids::feedbackPitch, "FB PITCH", memoryGroup, 2, 2);

    // ---- granular engine ----
    addChoice (ids::grainMode, granularGroup, 0);
    addChoice (ids::grainEnvelope, granularGroup, 1);
    addToggle (ids::tempoSync, "TEMPO SYNC", granularGroup, 0);
    addKnob (ids::grainSize, "SIZE", granularGroup, 0, 0);
    addKnob (ids::density, "DENSITY", granularGroup, 1, 0);
    addKnob (ids::overlap, "OVERLAP", granularGroup, 2, 0);
    addKnob (ids::maxGrains, "MAX", granularGroup, 3, 0);
    addKnob (ids::bufferPosition, "POSITION", granularGroup, 0, 1);
    addKnob (ids::scanSpeed, "SCAN", granularGroup, 1, 1);
    addKnob (ids::spray, "SPRAY", granularGroup, 2, 1);
    addKnob (ids::reverseProbability, "REVERSE", granularGroup, 3, 1);
    addKnob (ids::grainGain, "GAIN", granularGroup, 0, 2);
    addKnob (ids::stereoSpread, "SPREAD", granularGroup, 1, 2);
    addKnob (ids::stereoRandom, "PAN RAND", granularGroup, 2, 2);
    addKnob (ids::hostRate, "SCAN MULT", granularGroup, 3, 2);

    // ---- pitch control ----
    addChoice (ids::pitchSource, pitchCoreGroup, 0);
    addChoice (ids::midiChannel, pitchCoreGroup, 1);
    addChoice (ids::notePriority, pitchCoreGroup, 2);
    addChoice (ids::followMode, pitchCoreGroup, 3);
    addChoice (ids::mapCurve, pitchMapGroup, 0);
    addKnob (ids::noteLow, "NOTE LOW", pitchCoreGroup, 0, 0);
    addKnob (ids::noteHigh, "NOTE HIGH", pitchCoreGroup, 1, 0);
    addKnob (ids::trackRangeLow, "TRACK LOW", pitchCoreGroup, 2, 0);
    addKnob (ids::trackRangeHigh, "TRACK HIGH", pitchCoreGroup, 3, 0);
    addKnob (ids::transposition, "TRANSPOSE", pitchCoreGroup, 0, 1);
    addKnob (ids::pitchSensitivity, "SENSITIVE", pitchCoreGroup, 1, 1);
    addKnob (ids::confidenceThreshold, "CONFIDENCE", pitchCoreGroup, 2, 1);
    addKnob (ids::pitchAttack, "ATTACK", pitchCoreGroup, 3, 1);
    addKnob (ids::pitchRelease, "RELEASE", pitchCoreGroup, 0, 2);
    addKnob (ids::pitchSmoothing, "SMOOTH", pitchCoreGroup, 1, 2);
    addKnob (ids::hybridBlend, "HYBRID", pitchCoreGroup, 2, 2);
    addKnob (ids::releaseCC, "RELEASE CC", pitchCoreGroup, 3, 2);

    addKnob (ids::mapTranspose, "TRANSPOSE", pitchMapGroup, 0, 0);
    addKnob (ids::mapSpeed, "SPEED", pitchMapGroup, 1, 0);
    addKnob (ids::mapPosition, "POSITION", pitchMapGroup, 2, 0);
    addKnob (ids::mapDensity, "DENSITY", pitchMapGroup, 0, 1);
    addKnob (ids::mapSize, "SIZE", pitchMapGroup, 1, 1);
    addKnob (ids::mapRate, "RATE", pitchMapGroup, 2, 1);
    addKnob (ids::mapHarmonic, "HARMONIC", pitchMapGroup, 0, 2);
    addKnob (ids::mapResonant, "RESONANT", pitchMapGroup, 1, 2);
    addKnob (ids::mapReverb, "REVERB", pitchMapGroup, 2, 2);
    addKnob (ids::mapStereo, "STEREO", pitchMapGroup, 0, 3);
    addKnob (ids::mapSmoothing, "SMOOTH", pitchMapGroup, 1, 3);

    // ---- harmonic organisation ----
    addChoice (ids::scale, harmonicsGroup, 0);
    addToggle (ids::quantize, "QUANTIZE", harmonicsGroup, 0);
    addKnob (ids::fundamental, "FUNDAMENTAL", harmonicsGroup, 0, 0);
    addKnob (ids::octaveMix, "OCTAVE", harmonicsGroup, 1, 0);
    addKnob (ids::harmonicLayers, "LAYERS", harmonicsGroup, 2, 0);
    addKnob (ids::intervalA, "INTERVAL A", harmonicsGroup, 3, 0);
    addKnob (ids::intervalB, "INTERVAL B", harmonicsGroup, 4, 0);
    addKnob (ids::harmonicSpread, "SPREAD", harmonicsGroup, 0, 1);
    addKnob (ids::rootNote, "ROOT", harmonicsGroup, 1, 1);
    addKnob (ids::tuning, "TUNING", harmonicsGroup, 2, 1);
    addKnob (ids::detune, "DETUNE", harmonicsGroup, 3, 1);
    addKnob (ids::harmLevel1, "LEVEL 1", harmonicsGroup, 4, 1);
    addKnob (ids::harmLevel2, "LEVEL 2", harmonicsGroup, 0, 2);
    addKnob (ids::harmLevel3, "LEVEL 3", harmonicsGroup, 1, 2);
    addKnob (ids::harmLevel4, "LEVEL 4", harmonicsGroup, 2, 2);

    // ---- spatial engine ----
    addToggle (ids::reverbFreeze, "FREEZE SPACE", spaceGroup, 0);
    addKnob (ids::reverbMix, "MIX", spaceGroup, 0, 0);
    addKnob (ids::reverbDecay, "DECAY", spaceGroup, 1, 0);
    addKnob (ids::roomSize, "ROOM", spaceGroup, 2, 0);
    addKnob (ids::diffusion, "DIFFUSION", spaceGroup, 3, 0);
    addKnob (ids::damping, "DAMPING", spaceGroup, 0, 1);
    addKnob (ids::preDelay, "PRE-DELAY", spaceGroup, 1, 1);
    addKnob (ids::width, "WIDTH", spaceGroup, 2, 1);
    addKnob (ids::spaceModulation, "MODULATION", spaceGroup, 3, 1);
    addKnob (ids::filterFreq, "FILTER", spaceGroup, 0, 2);
    addKnob (ids::resonance, "RESONANCE", spaceGroup, 1, 2);
    addKnob (ids::reverbFeedback, "FEEDBACK", spaceGroup, 2, 2);

    // ---- modulation ----
    addChoice (ids::modSource, modulationGroup, 0);
    addChoice (ids::modDestination, modulationGroup, 1);
    addChoice (ids::modSource2, modulationGroup, 2);
    addChoice (ids::modDestination2, modulationGroup, 3);
    addKnob (ids::modRate, "RATE", modulationGroup, 0, 0);
    addKnob (ids::modDepth, "DEPTH 1", modulationGroup, 1, 0);
    addKnob (ids::modDepth2, "DEPTH 2", modulationGroup, 2, 0);

    // ---- output ----
    addKnob (ids::dryWet, "DRY / WET", outputGroup, 0, 0);
    addKnob (ids::grainLevel, "GRAIN", outputGroup, 1, 0);
    addKnob (ids::reverbLevel, "REVERB", outputGroup, 2, 0);
    addKnob (ids::inputGain, "INPUT", outputGroup, 3, 0);
    addKnob (ids::outputGain, "OUTPUT", outputGroup, 4, 0);

    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    saveButton.onClick = [this] { promptSavePreset(); };
    deleteButton.onClick = [this] { promptDeletePreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);

    refreshPresetList();

    const auto matched = processor.matchFactoryPreset();
    presetName = matched.isNotEmpty() ? matched : "MEMORY BLOOM";
    applyingPreset = true;
    setPresetName (presetName);
    applyingPreset = false;

    setResizable (true, true);
    setResizeLimits (1200, 860, 1920, 1360);
    setSize (kWidth, kHeight);
    startTimerHz (30);
}

MemoryGranulatorModeAudioProcessorEditor::~MemoryGranulatorModeAudioProcessorEditor()
{
    stopTimer();

    if (activeDialog != nullptr)
        activeDialog->exitModalState (0);

    setLookAndFeel (nullptr);
}

void MemoryGranulatorModeAudioProcessorEditor::addKnob (const char* paramId,
                                                        const char* label,
                                                        int group, int column, int row)
{
    auto knob = std::make_unique<Knob>();
    knob->label = label;
    knob->paramId = paramId;
    knob->group = group;
    knob->column = column;
    knob->row = row;

    knob->slider = std::make_unique<juce::Slider> (
        juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    knob->slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId,
                             group == pitchMapGroup ? MemoryLookAndFeel::teal()
                                                    : MemoryLookAndFeel::amber());
    knob->slider->setMouseDragSensitivity (180);

    addAndMakeVisible (*knob->slider);

    knob->attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, paramId, *knob->slider);

    knobs.push_back (std::move (knob));
}

void MemoryGranulatorModeAudioProcessorEditor::addChoice (const char* paramId,
                                                          int group, int slot)
{
    auto choice = std::make_unique<Choice>();
    choice->paramId = paramId;
    choice->group = group;
    choice->slot = slot;

    choice->box = std::make_unique<juce::ComboBox>();
    choice->box->setJustificationType (juce::Justification::centredLeft);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*> (
            processor.apvts.getParameter (paramId)))
        choice->box->addItemList (parameter->choices, 1);

    addAndMakeVisible (*choice->box);

    choice->attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, paramId, *choice->box);

    choices.push_back (std::move (choice));
}

void MemoryGranulatorModeAudioProcessorEditor::addToggle (const char* paramId,
                                                          const char* label,
                                                          int group, int slot)
{
    auto toggle = std::make_unique<Toggle>();
    toggle->label = label;
    toggle->paramId = paramId;
    toggle->group = group;
    toggle->slot = slot;

    toggle->button = std::make_unique<juce::ToggleButton> (label);
    toggle->button->setTooltip (label);
    addAndMakeVisible (*toggle->button);

    toggle->attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment> (
            processor.apvts, paramId, *toggle->button);

    toggles.push_back (std::move (toggle));
}

void MemoryGranulatorModeAudioProcessorEditor::layoutKnobGroup (int group,
                                                                 juce::Rectangle<int> content,
                                                                 int columns, int rows,
                                                                 int knobSize)
{
    const auto cellWidth = content.getWidth() / juce::jmax (1, columns);
    const auto cellHeight = content.getHeight() / juce::jmax (1, rows);

    for (auto& knob : knobs)
    {
        if (knob->group != group)
            continue;

        const auto cell = juce::Rectangle<int> (
            content.getX() + knob->column * cellWidth,
            content.getY() + knob->row * cellHeight,
            cellWidth, cellHeight);

        knob->cell = cell;
        const auto top = cell.getY() + 1;
        knob->slider->setBounds (cell.getCentreX() - knobSize / 2, top, knobSize, knobSize);
        knob->labelBounds = { cell.getX(), top + knobSize + 1, cellWidth, 11 };
        knob->valueBounds = { cell.getX(), top + knobSize + 11, cellWidth, 11 };
    }
}

void MemoryGranulatorModeAudioProcessorEditor::layoutChoices()
{
    for (auto& choice : choices)
    {
        auto bounds = juce::Rectangle<int>();

        switch (choice->group)
        {
            case memoryGroup:
            {
                const auto width = (memoryArea.getWidth() - 30) / 2;
                bounds = { memoryArea.getX() + 10 + choice->slot * (width + 10),
                           memoryArea.getY() + 122, width, 24 };
                break;
            }

            case granularGroup:
            {
                const auto width = (granularArea.getWidth() - 30) / 2;
                bounds = { granularArea.getX() + 10 + choice->slot * (width + 10),
                           granularArea.getY() + 28, width, 24 };
                break;
            }

            case pitchCoreGroup:
            {
                if (choice->slot == 0)
                    bounds = { pitchArea.getX() + 8, pitchArea.getY() + 28, 136, 24 };
                else if (choice->slot == 1)
                    bounds = { pitchArea.getX() + 150, pitchArea.getY() + 28, 136, 24 };
                else if (choice->slot == 2)
                    bounds = { pitchArea.getX() + 292, pitchArea.getY() + 28, 156, 24 };
                else
                    bounds = { pitchArea.getX() + 8, pitchArea.getY() + 56, 214, 24 };
                break;
            }

            case pitchMapGroup:
                bounds = { pitchArea.getX() + 238, pitchArea.getY() + 56, 210, 24 };
                break;

            case harmonicsGroup:
                bounds = { harmonicsArea.getX() + 12, harmonicsArea.getY() + 28, 224, 24 };
                break;

            case modulationGroup:
            {
                const auto width = (modulationArea.getWidth() - 30) / 2;
                const auto column = choice->slot % 2;
                const auto row = choice->slot / 2;
                bounds = { modulationArea.getX() + 10 + column * (width + 10),
                           modulationArea.getY() + 28 + row * 30, width, 24 };
                break;
            }

            default:
                break;
        }

        choice->box->setBounds (bounds);
    }
}

void MemoryGranulatorModeAudioProcessorEditor::layoutToggles()
{
    for (auto& toggle : toggles)
    {
        auto bounds = juce::Rectangle<int>();

        switch (toggle->group)
        {
            case memoryGroup:
            {
                const auto column = toggle->slot % 3;
                const auto row = toggle->slot / 3;
                const auto width = (memoryArea.getWidth() - 30) / 3;
                bounds = { memoryArea.getX() + 10 + column * (width + 5),
                           memoryArea.getY() + 152 + row * 23, width, 21 };
                break;
            }

            case granularGroup:
                bounds = { granularArea.getRight() - 116, granularArea.getY() + 30, 106, 20 };
                break;

            case harmonicsGroup:
                bounds = { harmonicsArea.getRight() - 122, harmonicsArea.getY() + 28, 110, 22 };
                break;

            case spaceGroup:
                bounds = { spaceArea.getRight() - 132, spaceArea.getY() + 28, 120, 22 };
                break;

            default:
                break;
        }

        toggle->button->setBounds (bounds);
    }
}

void MemoryGranulatorModeAudioProcessorEditor::resized()
{
    presetBox.setBounds (presetBarArea.getX() + 58, presetBarArea.getY() + 4, 270, 29);
    saveButton.setBounds (presetBarArea.getX() + 340, presetBarArea.getY() + 5, 66, 25);
    deleteButton.setBounds (presetBarArea.getX() + 414, presetBarArea.getY() + 5, 56, 25);

    layoutKnobGroup (memoryGroup,
                     { memoryArea.getX() + 10, memoryArea.getY() + 200,
                       memoryArea.getWidth() - 20, memoryArea.getHeight() - 208 },
                     3, 3, 34);

    layoutKnobGroup (granularGroup,
                     { granularArea.getX() + 10, granularArea.getY() + 92,
                       granularArea.getWidth() - 20, granularArea.getHeight() - 100 },
                     4, 3, 48);

    layoutKnobGroup (pitchCoreGroup,
                     { pitchArea.getX() + 8, pitchArea.getY() + 88,
                       216, pitchArea.getHeight() - 96 },
                     4, 3, 38);

    layoutKnobGroup (pitchMapGroup,
                     { pitchArea.getX() + 234, pitchArea.getY() + 88,
                       214, pitchArea.getHeight() - 96 },
                     3, 4, 38);

    layoutKnobGroup (harmonicsGroup,
                     { harmonicsArea.getX() + 10, harmonicsArea.getY() + 62,
                       harmonicsArea.getWidth() - 20, harmonicsArea.getHeight() - 70 },
                     5, 3, 44);

    layoutKnobGroup (spaceGroup,
                     { spaceArea.getX() + 10, spaceArea.getY() + 60,
                       spaceArea.getWidth() - 20, spaceArea.getHeight() - 68 },
                     4, 3, 48);

    layoutKnobGroup (modulationGroup,
                     { modulationArea.getX() + 10, modulationArea.getY() + 100,
                       modulationArea.getWidth() - 20, 100 },
                     3, 1, 54);

    layoutKnobGroup (outputGroup,
                     { outputArea.getX() + 18, outputArea.getY() + 62, 690,
                       outputArea.getHeight() - 72 },
                     5, 1, 68);

    layoutChoices();
    layoutToggles();
}

//==============================================================================
void MemoryGranulatorModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (MemoryLookAndFeel::base());

    juce::ColourGradient glow (MemoryLookAndFeel::amber().withAlpha (0.10f), 0.0f, 0.0f,
                               MemoryLookAndFeel::teal().withAlpha (0.0f),
                               (float) kWidth * 0.72f, (float) kHeight * 0.62f, true);
    g.setGradientFill (glow);
    g.fillAll();

    juce::ColourGradient lowerGlow (MemoryLookAndFeel::teal().withAlpha (0.06f),
                                    (float) kWidth, (float) kHeight,
                                    MemoryLookAndFeel::amber().withAlpha (0.0f),
                                    (float) kWidth * 0.25f, (float) kHeight * 0.32f, true);
    g.setGradientFill (lowerGlow);
    g.fillAll();

    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.026f));
    for (int y = 0; y < kHeight; y += 16)
        for (int x = (y / 16) % 2 == 0 ? 8 : 0; x < kWidth; x += 16)
            g.fillRect (x, y, 1, 1);

    paintBaseplate (g);
    paintHeader (g);
    paintPresetBar (g);
    paintMemory (g);
    paintGranular (g);
    paintPitch (g);
    paintHarmonics (g);
    paintSpace (g);
    paintModulation (g);
    paintOutput (g);

    g.setColour (MemoryLookAndFeel::line().withAlpha (0.9f));
    g.drawRect (getLocalBounds(), 1);
}

void MemoryGranulatorModeAudioProcessorEditor::paintBaseplate (juce::Graphics& g)
{
    const auto base = juce::Rectangle<int> (0, 0, kWidth, kHeight);
    g.setColour (juce::Colour (0xff0d0f15));
    g.fillRoundedRectangle (base.toFloat(), 23.0f);
    g.setGradientFill (MemoryLookAndFeel::accentGradient (base.toFloat()));
    g.drawRoundedRectangle (base.reduced (1).toFloat(), 22.0f, 1.8f);
}

void MemoryGranulatorModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const auto titleArea = juce::Rectangle<int> (headerArea.getX(), headerArea.getY() - 2,
                                                 830, 52);
    g.setFont (lookAndFeel.titleFont (43.0f));
    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.drawText ("MEMORY GRANULATOR", titleArea.translated (0, 3),
                juce::Justification::centredLeft, false);
    g.setGradientFill (MemoryLookAndFeel::accentGradient (titleArea.toFloat()));
    g.drawText ("MEMORY GRANULATOR", titleArea, juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (9.5f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.60f));
    drawTracked (g, "PAST AUDIO - PRESENT PITCH - REAL MIDI CONTROL",
                 { headerArea.getX() + 4, headerArea.getY() + 45, 720, 15 },
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.titleFont (20.0f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.86f));
    drawTracked (g, "NULLCAM",
                 { headerArea.getRight() - 300, headerArea.getY() + 8, 300, 25 },
                 0.30f, juce::Justification::centredRight);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "48 GRAINS - HYBRID PITCH - VST3 MIDI BUS",
                 { headerArea.getRight() - 340, headerArea.getY() + 36, 340, 15 },
                 0.15f, juce::Justification::centredRight);
}

void MemoryGranulatorModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    paintCard (g, presetBarArea);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "PRESET", { presetBarArea.getX() + 16, presetBarArea.getY() + 11, 40, 14 },
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (edited ? MemoryLookAndFeel::teal() : MemoryLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, edited ? "EDITED" : "FACTORY",
                 { presetBarArea.getX() + 480, presetBarArea.getY() + 11, 80, 14 },
                 0.16f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "EIGHT FACTORY PRESETS - USER PRESETS ARE XML",
                 { presetBarArea.getRight() - 390, presetBarArea.getY() + 11, 370, 14 },
                 0.13f, juce::Justification::centredRight);
}

void MemoryGranulatorModeAudioProcessorEditor::paintCardTitle (juce::Graphics& g,
                                                                juce::Rectangle<int> area,
                                                                const juce::String& title,
                                                                const juce::String& subtitle)
{
    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, title, { area.getX() + 12, area.getY() + 10, area.getWidth() - 24, 14 },
                 0.21f, juce::Justification::centredLeft);

    if (subtitle.isNotEmpty())
    {
        g.setFont (lookAndFeel.labelFont (7.8f));
        g.setColour (MemoryLookAndFeel::dim().withAlpha (0.72f));
        drawTracked (g, subtitle,
                     { area.getRight() - 220, area.getY() + 10, 208, 13 },
                     0.10f, juce::Justification::centredRight);
    }
}

void MemoryGranulatorModeAudioProcessorEditor::paintKnobs (juce::Graphics& g, int group)
{
    for (const auto& knob : knobs)
    {
        if (knob->group != group)
            continue;

        g.setFont (lookAndFeel.labelFont (7.8f));
        g.setColour (MemoryLookAndFeel::dim());
        drawTracked (g, knob->label, knob->labelBounds, 0.12f, juce::Justification::centred);

        g.setFont (lookAndFeel.valueFont (8.4f));
        g.setColour (group == pitchMapGroup ? MemoryLookAndFeel::teal()
                                            : MemoryLookAndFeel::amber());
        g.drawText (valueText (*knob), knob->valueBounds, juce::Justification::centred, false);
    }
}

void MemoryGranulatorModeAudioProcessorEditor::paintMemory (juce::Graphics& g)
{
    paintCard (g, memoryArea);
    paintCardTitle (g, memoryArea, "1 - MEMORY", "CIRCULAR CAPTURE / EVOLUTION");

    const auto plot = juce::Rectangle<int> (memoryArea.getX() + 10, memoryArea.getY() + 28,
                                            memoryArea.getWidth() - 20, 88);
    g.setColour (juce::Colour (0xff080a0f));
    g.fillRoundedRectangle (plot.toFloat(), 8.0f);
    g.setColour (MemoryLookAndFeel::line().withAlpha (0.72f));
    g.drawRoundedRectangle (plot.toFloat(), 8.0f, 1.0f);

    g.setColour (MemoryLookAndFeel::line().withAlpha (0.34f));
    g.drawLine ((float) plot.getCentreX(), (float) plot.getY(),
                (float) plot.getCentreX(), (float) plot.getBottom(), 1.0f);

    const auto& snapshot = processor.getSnapshot();
    const auto pointWidth = (float) plot.getWidth() / memorygranulator::kWaveformPoints;

    for (int i = 0; i < memorygranulator::kWaveformPoints; ++i)
    {
        const auto amplitude = snapshot.waveform[i].load (std::memory_order_relaxed);
        const auto x = (float) plot.getX() + i * pointWidth;
        const auto half = amplitude * (float) plot.getHeight() * 0.46f;
        g.setColour (MemoryLookAndFeel::teal().withAlpha (0.72f));
        g.drawLine (x, (float) plot.getCentreY() - half,
                    x, (float) plot.getCentreY() + half, juce::jmax (1.0f, pointWidth));
    }

    const auto playheadX = (float) plot.getX() + playhead * (float) plot.getWidth();
    g.setColour (MemoryLookAndFeel::amber());
    g.drawLine (playheadX, (float) plot.getY(), playheadX, (float) plot.getBottom(), 1.6f);
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (
        juce::Point<float> (playheadX, (float) plot.getY() + 3.0f)));

    const auto capturing = paramValue (ids::captureEnable) > 0.5f;
    g.setFont (lookAndFeel.valueFont (8.0f));
    g.setColour (capturing ? MemoryLookAndFeel::amber() : MemoryLookAndFeel::dim());
    g.drawText (capturing ? "CAPTURING" : "CAPTURE OFF",
                juce::Rectangle<int> (plot.getX() + 8, plot.getBottom() - 15, 100, 12),
                juce::Justification::centredLeft, false);

    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.72f));
    g.drawText ("MEMORY WAVEFORM - PLAYHEAD",
                juce::Rectangle<int> (plot.getRight() - 170, plot.getBottom() - 15, 162, 12),
                juce::Justification::centredRight, false);

    paintKnobs (g, memoryGroup);
}

void MemoryGranulatorModeAudioProcessorEditor::paintGranular (juce::Graphics& g)
{
    paintCard (g, granularArea);
    paintCardTitle (g, granularArea, "2 - GRANULAR", "OVERLAPPING INDEPENDENT GRAINS");

    const auto plot = juce::Rectangle<int> (granularArea.getX() + 10, granularArea.getY() + 58,
                                            granularArea.getWidth() - 20, 28);
    g.setColour (juce::Colour (0xff080a0f));
    g.fillRoundedRectangle (plot.toFloat(), 5.0f);

    const auto& snapshot = processor.getSnapshot();

    for (int i = 0; i < memorygranulator::kMaxGrains; ++i)
    {
        const auto life = snapshot.grainLife[i].load (std::memory_order_relaxed);

        if (life <= 0.0f)
            continue;

        const auto position = snapshot.grainPosition[i].load (std::memory_order_relaxed);
        const auto x = (float) plot.getX() + position * (float) plot.getWidth();
        const auto y = (float) plot.getCentreY() + (1.0f - life) * (float) plot.getHeight() * 0.34f;
        g.setColour (MemoryLookAndFeel::amber().withAlpha (0.35f + life * 0.65f));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (
            juce::Point<float> (x, y)));
    }

    const auto playheadX = (float) plot.getX() + playhead * (float) plot.getWidth();
    g.setColour (MemoryLookAndFeel::teal().withAlpha (0.85f));
    g.drawLine (playheadX, (float) plot.getY(), playheadX, (float) plot.getBottom(), 1.0f);

    g.setFont (lookAndFeel.valueFont (7.8f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.72f));
    g.drawText (juce::String (activeGrains) + " ACTIVE GRAINS",
                juce::Rectangle<int> (plot.getX() + 7, plot.getY() + 3, 120, 11),
                juce::Justification::centredLeft, false);
    g.drawText ("POSITION / LIFE",
                juce::Rectangle<int> (plot.getRight() - 110, plot.getY() + 3, 102, 11),
                juce::Justification::centredRight, false);

    paintKnobs (g, granularGroup);
}

void MemoryGranulatorModeAudioProcessorEditor::paintPitch (juce::Graphics& g)
{
    paintCard (g, pitchArea);
    paintCardTitle (g, pitchArea, "3 - PITCH CONTROL", "MIDI + AUDIO + HYBRID MATRIX");

    g.setFont (lookAndFeel.labelFont (7.5f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "CORE", { pitchArea.getX() + 8, pitchArea.getY() + 78, 120, 10 },
                 0.13f, juce::Justification::centredLeft);
    drawTracked (g, "PITCH MAP", { pitchArea.getX() + 236, pitchArea.getY() + 78, 120, 10 },
                 0.13f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.valueFont (8.0f));
    g.setColour (confidence > 0.55f ? MemoryLookAndFeel::teal() : MemoryLookAndFeel::dim());
    g.drawText (trackedHz > 20.0f ? "TRACK " + juce::String ((int) (trackedHz + 0.5f)) + " HZ"
                                  : juce::String ("TRACK -"),
                juce::Rectangle<int> (pitchArea.getRight() - 142, pitchArea.getY() + 10, 130, 12),
                juce::Justification::centredRight, false);

    g.setFont (lookAndFeel.valueFont (7.6f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.75f));
    juce::String notes;

    for (int i = 0; i < 8; ++i)
        if (midiNotes[i] >= 0)
            notes += juce::MidiMessage::getMidiNoteName (midiNotes[i], true, true, 2) + " ";

    g.drawText (notes.isEmpty() ? "NO MIDI NOTES" : notes,
                juce::Rectangle<int> (pitchArea.getX() + 8, pitchArea.getBottom() - 14,
                                      pitchArea.getWidth() - 16, 11),
                juce::Justification::centredLeft, false);

    paintKnobs (g, pitchCoreGroup);
    paintKnobs (g, pitchMapGroup);
}

void MemoryGranulatorModeAudioProcessorEditor::paintHarmonics (juce::Graphics& g)
{
    paintCard (g, harmonicsArea);
    paintCardTitle (g, harmonicsArea, "4 - HARMONICS", "ORGANISE WITHOUT A HARMONISER");

    const float levels[4] = { paramValue (ids::harmLevel1), paramValue (ids::harmLevel2),
                              paramValue (ids::harmLevel3), paramValue (ids::harmLevel4) };
    const auto barArea = juce::Rectangle<int> (harmonicsArea.getX() + 252,
                                               harmonicsArea.getY() + 28,
                                               harmonicsArea.getWidth() - 264, 24);

    for (int i = 0; i < 4; ++i)
    {
        const auto width = barArea.getWidth() / 4;
        const auto bar = juce::Rectangle<int> (barArea.getX() + i * width + 3,
                                               barArea.getY() + 12,
                                               juce::jmax (2, (int) ((width - 6) * levels[i])),
                                               7);
        g.setColour (MemoryLookAndFeel::amber().withAlpha (0.25f + levels[i] * 0.75f));
        g.fillRoundedRectangle (bar.toFloat(), 3.0f);
    }

    paintKnobs (g, harmonicsGroup);
}

void MemoryGranulatorModeAudioProcessorEditor::paintSpace (juce::Graphics& g)
{
    paintCard (g, spaceArea);
    paintCardTitle (g, spaceArea, "5 - SPACE", "GRANULAR CLOUD + EVOLVING TAIL");

    paintKnobs (g, spaceGroup);

    const auto bar = juce::Rectangle<int> (spaceArea.getX() + 12, spaceArea.getBottom() - 23,
                                           spaceArea.getWidth() - 24, 7);
    paintBar (g, bar, feedbackLevel * 4.0f, MemoryLookAndFeel::coral());

    g.setFont (lookAndFeel.labelFont (7.2f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "MEMORY FEEDBACK ACTIVITY",
                 { spaceArea.getX() + 12, spaceArea.getBottom() - 35, 180, 10 },
                 0.10f, juce::Justification::centredLeft);
}

void MemoryGranulatorModeAudioProcessorEditor::paintModulation (juce::Graphics& g)
{
    paintCard (g, modulationArea);
    paintCardTitle (g, modulationArea, "6 - MODULATION", "TWO SOURCES / TWO DESTINATIONS");

    paintKnobs (g, modulationGroup);

    const auto bar = juce::Rectangle<int> (modulationArea.getX() + 12,
                                           modulationArea.getBottom() - 48,
                                           modulationArea.getWidth() - 24, 10);
    paintBar (g, bar, (modulationValue + 1.0f) * 0.5f, MemoryLookAndFeel::teal());

    g.setFont (lookAndFeel.labelFont (7.5f));
    g.setColour (MemoryLookAndFeel::dim());
    drawTracked (g, "LIVE MODULATION VALUE",
                 { modulationArea.getX() + 12, bar.getY() - 14, 180, 11 },
                 0.11f, juce::Justification::centredLeft);
    g.setFont (lookAndFeel.valueFont (8.5f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.75f));
    g.drawText (juce::String (modulationValue, 3),
                juce::Rectangle<int> (modulationArea.getRight() - 92, bar.getY() - 14, 80, 11),
                juce::Justification::centredRight, false);
}

void MemoryGranulatorModeAudioProcessorEditor::paintOutput (juce::Graphics& g)
{
    paintCard (g, outputArea);
    paintCardTitle (g, outputArea, "7 - OUTPUT", "BOUNDED FEEDBACK / TRUE PEAK METERS");

    paintKnobs (g, outputGroup);

    const auto meterX = outputArea.getX() + 750;
    const auto meterWidth = outputArea.getWidth() - 780;

    auto meter = [&] (float level, const juce::String& label, juce::Colour colour, int y)
    {
        g.setFont (lookAndFeel.labelFont (8.0f));
        g.setColour (MemoryLookAndFeel::dim());
        drawTracked (g, label, { meterX, y, 80, 12 }, 0.14f, juce::Justification::centredLeft);
        paintBar (g, { meterX + 86, y + 1, meterWidth - 86, 9 }, level, colour);
    };

    meter (inputLevel, "INPUT", MemoryLookAndFeel::dim().brighter (0.35f), outputArea.getY() + 48);
    meter (grainLevel, "GRAINS", MemoryLookAndFeel::amber(), outputArea.getY() + 78);
    meter (reverbLevel, "SPACE", MemoryLookAndFeel::teal(), outputArea.getY() + 108);
    meter (outputLevel, "OUTPUT", MemoryLookAndFeel::coral(), outputArea.getY() + 138);

    g.setFont (lookAndFeel.valueFont (9.0f));
    g.setColour (MemoryLookAndFeel::ink().withAlpha (0.74f));
    g.drawText (juce::String (activeNotes) + " MIDI VOICES - " + juce::String (activeGrains) + " GRAINS",
                juce::Rectangle<int> (meterX, outputArea.getBottom() - 30, 320, 14),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (7.5f));
    g.setColour (MemoryLookAndFeel::dim().withAlpha (0.78f));
    drawTracked (g, "THE DRY PATH, MEMORY, GRAIN CLOUD AND SPATIAL TAIL ARE ONE INSTRUMENT",
                 { meterX, outputArea.getBottom() - 17, meterWidth, 12 },
                 0.09f, juce::Justification::centredLeft);
}

//==============================================================================
juce::String MemoryGranulatorModeAudioProcessorEditor::valueText (const Knob& knob) const
{
    const auto value = paramValue (knob.paramId);

    if (auto* parameter = processor.apvts.getParameter (knob.paramId))
        return parameter->getText (value, 18);

    return juce::String (value, 2);
}

float MemoryGranulatorModeAudioProcessorEditor::paramValue (const juce::String& id) const
{
    if (auto* parameter = processor.apvts.getParameter (id))
        return parameter->convertFrom0to1 (parameter->getValue());

    return 0.0f;
}

void MemoryGranulatorModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
        presetBox.addItem (MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
                               (MemoryGranulatorModeAudioProcessor::Preset) i), i + 1);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MINE");

        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }

    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
            if (matched == MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
                               (MemoryGranulatorModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);
    }
    else if (userPresets.contains (presetName))
    {
        presetBox.setSelectedId (100 + userPresets.indexOf (presetName),
                                 juce::dontSendNotification);
    }
    else
    {
        presetBox.setSelectedId (0, juce::dontSendNotification);
    }

    updatePresetButtons();
}

void MemoryGranulatorModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId >= 1 && itemId <= MemoryGranulatorModeAudioProcessor::kNumPresets)
    {
        processor.applyPreset ((MemoryGranulatorModeAudioProcessor::Preset) (itemId - 1));
        setPresetName (MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
                           (MemoryGranulatorModeAudioProcessor::Preset) (itemId - 1)));
        edited = false;
        repaint();
        return;
    }

    if (itemId >= 100)
    {
        const auto name = userPresets[itemId - 100];

        if (processor.loadUserPreset (name))
        {
            setPresetName (name);
            edited = false;
            repaint();
        }
    }
}

void MemoryGranulatorModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;

    if (userPresets.contains (name))
        presetBox.setSelectedId (100 + userPresets.indexOf (name), juce::dontSendNotification);
    else
        for (int i = 0; i < MemoryGranulatorModeAudioProcessor::kNumPresets; ++i)
            if (name == MemoryGranulatorModeAudioProcessor::getFactoryPresetName (
                            (MemoryGranulatorModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);

    updatePresetButtons();
}

void MemoryGranulatorModeAudioProcessorEditor::updatePresetButtons()
{
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void MemoryGranulatorModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || suggested == "CUSTOM" || userPresets.contains (suggested))
        suggested = "MY MEMORY";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + processor.getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;
    juce::Component::SafePointer<MemoryGranulatorModeAudioProcessorEditor> safe (this);

    dialog->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe, dialog] (int result) mutable
        {
            if (safe == nullptr)
                return;

            safe->activeDialog = nullptr;

            if (result != 1)
                return;

            const auto name = dialog->getTextEditorContents ("name").trim();

            if (name.isEmpty())
                return;

            if (safe->processor.saveUserPreset (name))
            {
                safe->refreshPresetList();
                safe->setPresetName (name);
                safe->edited = false;
                safe->repaint();
            }
        }), true);
}

void MemoryGranulatorModeAudioProcessorEditor::promptDeletePreset()
{
    if (activeDialog != nullptr || ! userPresets.contains (presetName))
        return;

    auto* dialog = new juce::AlertWindow ("DELETE PRESET",
                                          "Delete \"" + presetName + "\"?\n\n"
                                              + "The file is removed from the preset folder.",
                                          juce::MessageBoxIconType::WarningIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addButton ("DELETE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;
    juce::Component::SafePointer<MemoryGranulatorModeAudioProcessorEditor> safe (this);

    dialog->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe] (int result) mutable
        {
            if (safe == nullptr)
                return;

            safe->activeDialog = nullptr;

            if (result != 1 || ! safe->userPresets.contains (safe->presetName))
                return;

            safe->processor.deleteUserPreset (safe->presetName);
            safe->presetName.clear();
            safe->refreshPresetList();
            safe->repaint();
        }), true);
}

void MemoryGranulatorModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();

    if (++frame % 15 == 0)
    {
        const auto matched = processor.matchFactoryPreset();

        if (! applyingPreset)
        {
            if (matched.isNotEmpty())
            {
                if (presetName != matched)
                    setPresetName (matched);
                edited = false;
            }
            else if (userPresets.contains (presetName))
            {
                edited = false;
            }
            else
            {
                edited = true;
            }

            repaint (presetBarArea);
        }
    }
}

void MemoryGranulatorModeAudioProcessorEditor::pullLiveData()
{
    const auto& snapshot = processor.getSnapshot();
    inputLevel += (snapshot.inputLevel.load (std::memory_order_relaxed) - inputLevel) * 0.22f;
    grainLevel += (snapshot.grainLevel.load (std::memory_order_relaxed) - grainLevel) * 0.22f;
    reverbLevel += (snapshot.reverbLevel.load (std::memory_order_relaxed) - reverbLevel) * 0.22f;
    outputLevel += (snapshot.outputLevel.load (std::memory_order_relaxed) - outputLevel) * 0.22f;
    feedbackLevel += (snapshot.feedbackLevel.load (std::memory_order_relaxed) - feedbackLevel) * 0.18f;
    trackedHz += (snapshot.trackedHz.load (std::memory_order_relaxed) - trackedHz) * 0.18f;
    confidence += (snapshot.confidence.load (std::memory_order_relaxed) - confidence) * 0.18f;
    playhead += (snapshot.playhead.load (std::memory_order_relaxed) - playhead) * 0.30f;
    modulationValue += (snapshot.modulationValue.load (std::memory_order_relaxed)
                         - modulationValue) * 0.20f;
    activeGrains = snapshot.activeGrains.load (std::memory_order_relaxed);
    activeNotes = snapshot.activeNotes.load (std::memory_order_relaxed);

    for (int i = 0; i < 8; ++i)
        midiNotes[i] = snapshot.midiNotes[i].load (std::memory_order_relaxed);

    repaint (memoryArea);
    repaint (granularArea);
    repaint (pitchArea);
    repaint (harmonicsArea);
    repaint (spaceArea);
    repaint (modulationArea);
    repaint (outputArea);
}
