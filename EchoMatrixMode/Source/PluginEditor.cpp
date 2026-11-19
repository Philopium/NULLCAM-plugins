#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int kWidth = 980;
constexpr int kHeight = 640;
constexpr int kMargin = 22;

// A private colour id used to mark the selected pad without touching the
// on/off state of the toggle button.
constexpr int kSelectedPadColourId = 0x20000e01;
const juce::Colour kSelectedPadMarker (0xffff00ff);

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

    juce::ColourGradient fill (EchoMatrixLookAndFeel::cardTop(), r.getX(), r.getY(),
                               EchoMatrixLookAndFeel::card(), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 16.0f);

    g.setGradientFill (EchoMatrixLookAndFeel::accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 15.0f, 1.5f);
}

// A thin ruler used to show where the playhead is inside the pattern.
void paintTick (juce::Graphics& g, juce::Rectangle<int> area, float phase, juce::Colour colour)
{
    const auto r = area.toFloat();

    g.setColour (juce::Colour (0xff111318));
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (colour.withAlpha (0.55f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    if (phase > 0.001f)
    {
        auto lit = r.reduced (1.5f);
        lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth() * juce::jlimit (0.0f, 1.0f, phase)));
        g.setColour (colour);
        g.fillRoundedRectangle (lit, 2.0f);
    }
}

} // namespace

//==============================================================================
juce::Colour EchoMatrixLookAndFeel::base()    { return juce::Colour (0xff07080c); }
juce::Colour EchoMatrixLookAndFeel::card()    { return juce::Colour (0xff12141b); }
juce::Colour EchoMatrixLookAndFeel::cardTop() { return juce::Colour (0xff1c1f28); }
juce::Colour EchoMatrixLookAndFeel::line()    { return juce::Colour (0xff313644); }
juce::Colour EchoMatrixLookAndFeel::ink()     { return juce::Colour (0xfff2f5fb); }
juce::Colour EchoMatrixLookAndFeel::dim()     { return juce::Colour (0xff8b93a6); }
juce::Colour EchoMatrixLookAndFeel::cyan()    { return juce::Colour (0xff35dcff); }
juce::Colour EchoMatrixLookAndFeel::magenta() { return juce::Colour (0xffff4ed8); }

juce::ColourGradient EchoMatrixLookAndFeel::accentGradient (juce::Rectangle<float> area)
{
    juce::ColourGradient gradient (cyan(), area.getX(), area.getCentreY(),
                                   magenta(), area.getRight(), area.getCentreY(), false);
    gradient.addColour (0.35, juce::Colour (0xff7b7cff));
    gradient.addColour (0.70, juce::Colour (0xffb04cff));
    return gradient;
}

EchoMatrixLookAndFeel::EchoMatrixLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff14161d));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, cyan());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, cyan());
    setColour (juce::PopupMenu::highlightedTextColourId, base());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff14161d));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0d0f14));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, cyan().withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, cyan());

    setColour (juce::Label::textColourId, ink());
    setColour (juce::ComboBox::textColourId, ink());
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0f1117));
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::arrowColourId, cyan());

    setColour (juce::TextButton::buttonColourId, cardTop());
    setColour (juce::TextButton::buttonOnColourId, cardTop().brighter (0.25f));
    setColour (juce::TextButton::textColourOffId, ink());
    setColour (juce::TextButton::textColourOnId, ink());
}

juce::Font EchoMatrixLookAndFeel::titleFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font EchoMatrixLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font EchoMatrixLookAndFeel::valueFont (float height) const
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                          height, juce::Font::plain));
}

void EchoMatrixLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (knob.translated (0.0f, 5.0f).reduced (3.0f));

    const auto radius = knob.getWidth() * 0.5f - 3.0f;
    const auto centre = knob.getCentre();

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (line().withAlpha (0.85f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path active;
    active.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setGradientFill (accentGradient (knob));
    g.strokePath (active, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const auto body = knob.reduced (10.0f);

    juce::ColourGradient bodyGradient (cardTop().brighter (0.14f), body.getX(), body.getY(),
                                       card().darker (0.45f), body.getX(), body.getBottom(), false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (body);

    g.setColour (colour.withAlpha (0.55f));
    g.drawEllipse (body.reduced (0.5f), 1.6f);

    auto gloss = body.reduced (body.getWidth() * 0.13f, 0.0f)
                     .withHeight (body.getHeight() * 0.46f)
                     .translated (0.0f, body.getHeight() * 0.07f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillEllipse (gloss);

    const auto tip = juce::Point<float> (centre.x + std::sin (angle) * (radius - 7.0f),
                                         centre.y - std::cos (angle) * (radius - 7.0f));
    const auto root = juce::Point<float> (centre.x + std::sin (angle) * (radius * 0.32f),
                                          centre.y - std::cos (angle) * (radius * 0.32f));

    g.setColour (colour);
    g.drawLine (juce::Line<float> (root, tip), 3.4f);

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (tip));

    g.setColour (colour.withAlpha (0.95f));
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (centre));
}

juce::Label* EchoMatrixLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (valueFont (10.0f));
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void EchoMatrixLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    // A pad: dark when off, lit with the accent gradient when on.
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto on = button.getToggleState();

    if (on)
    {
        juce::ColourGradient fill (EchoMatrixLookAndFeel::cyan().withAlpha (0.30f),
                                   r.getX(), r.getY(),
                                   EchoMatrixLookAndFeel::magenta().withAlpha (0.22f),
                                   r.getX(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, 9.0f);

        g.setGradientFill (accentGradient (r));
        g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 1.6f);
    }
    else
    {
        juce::ColourGradient fill (EchoMatrixLookAndFeel::cardTop().darker (0.25f),
                                   r.getX(), r.getY(),
                                   EchoMatrixLookAndFeel::card().darker (0.35f),
                                   r.getX(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, 9.0f);

        g.setColour (EchoMatrixLookAndFeel::line());
        g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 1.2f);
    }

    if (shouldDrawButtonAsHighlighted && ! on)
    {
        g.setColour (EchoMatrixLookAndFeel::cyan().withAlpha (0.45f));
        g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 1.2f);
    }

    // The selected pad wears a bright ring.
    if (button.findColour (kSelectedPadColourId) == kSelectedPadMarker)
    {
        g.setColour (EchoMatrixLookAndFeel::ink());
        g.drawRoundedRectangle (r.reduced (2.5f), 7.0f, 1.4f);
    }

    if (shouldDrawButtonAsDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r, 9.0f);
    }

    g.setFont (labelFont (button.getButtonText().length() > 2 ? 10.0f : 13.0f));
    g.setColour (on ? EchoMatrixLookAndFeel::ink()
                    : EchoMatrixLookAndFeel::dim().withAlpha (0.85f));

    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred, false);
}

void EchoMatrixLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                           int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);

    g.setColour (juce::Colour (0xff0f1117));
    g.fillRoundedRectangle (r, 10.0f);

    g.setGradientFill (accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 9.0f, 1.3f);

    const auto x = (float) width - 16.0f;
    const auto y = (float) height * 0.5f;

    juce::Path chevron;
    chevron.startNewSubPath (x - 4.0f, y - 2.0f);
    chevron.lineTo (x, y + 2.5f);
    chevron.lineTo (x + 4.0f, y - 2.0f);

    g.setColour (box.isEnabled() ? cyan() : dim());
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

juce::Font EchoMatrixLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (11.0f).withExtraKerningFactor (0.06f);
}

juce::Font EchoMatrixLookAndFeel::getPopupMenuFont()
{
    return labelFont (11.0f);
}

void EchoMatrixLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff14161d));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void EchoMatrixLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::backgroundColourId));
    g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 6.0f);

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                      label.getJustificationType(), 1);
}

//==============================================================================
EchoMatrixModeAudioProcessorEditor::EchoMatrixModeAudioProcessorEditor (EchoMatrixModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    headerArea    = { kMargin, 12, kWidth - 2 * kMargin, 64 };
    presetBarArea = { kMargin, 82, kWidth - 2 * kMargin, 34 };
    globalArea    = { kMargin, 124, 466, 244 };
    stepArea      = { 498, 124, kWidth - kMargin - 498, 244 };
    patternArea   = { kMargin, 376, kWidth - 2 * kMargin, 170 };
    meterArea     = { kMargin, 554, kWidth - 2 * kMargin, 64 };

    // The delay engine: eight knobs, four columns by two rows.
    addKnob (ids::delayTime,  "DELAY",  0, 0);
    addKnob (ids::feedback,   "FEEDBK", 1, 0);
    addKnob (ids::mix,        "MIX",    2, 0);
    addKnob (ids::drive,      "DRIVE",  3, 0);
    addKnob (ids::inputGain,  "IN",     0, 1);
    addKnob (ids::outputGain, "OUT",    1, 1);
    addKnob (ids::hpfCutoff,  "HPF",    2, 1);
    addKnob (ids::lpfCutoff,  "LPF",    3, 1);

    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    saveButton.onClick = [this] { promptSavePreset(); };
    deleteButton.onClick = [this] { promptDeletePreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);

    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ids::sync, syncButton);

    // The 16 pattern pads.
    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        auto& button = stepButtons[(std::size_t) i];
        button = std::make_unique<juce::ToggleButton> (juce::String (i + 1));
        button->setClickingTogglesState (true);

        button->onClick = [this, i]
        {
            auto* pad = stepButtons[(std::size_t) i].get();

            if (auto* parameter = processor.apvts.getParameter (
                    EchoMatrixModeAudioProcessor::stepParamId (ids::stepEnabled, i)))
                parameter->setValueNotifyingHost (
                    parameter->convertTo0to1 (pad->getToggleState() ? 1.0f : 0.0f));

            selectStep (i);
        };

        addAndMakeVisible (*button);
    }

    // The selected step: level and pan.
    stepLevelSlider = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal,
                                                       juce::Slider::NoTextBox);
    stepPanSlider = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal,
                                                     juce::Slider::NoTextBox);

    for (auto* slider : { stepLevelSlider.get(), stepPanSlider.get() })
    {
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 74, 22);
        slider->setColour (juce::Slider::trackColourId, juce::Colour (0xff232733));
        slider->setColour (juce::Slider::thumbColourId, EchoMatrixLookAndFeel::cyan());
        addAndMakeVisible (slider);
    }

    // The read-out text comes from the parameter itself (see the APVTS
    // layout), so the host and the panel always say the same thing.

    refreshPresetList();

    const auto matched = processor.matchFactoryPreset();
    presetName = matched.isNotEmpty() ? matched : "DOTTED EIGHTH";
    applyingPreset = true;
    setPresetName (presetName);
    applyingPreset = false;

    selectStep (0);
    syncStepButtons();

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

EchoMatrixModeAudioProcessorEditor::~EchoMatrixModeAudioProcessorEditor()
{
    stopTimer();

    if (activeDialog != nullptr)
        activeDialog->exitModalState (0);

    setLookAndFeel (nullptr);
}

void EchoMatrixModeAudioProcessorEditor::addKnob (const char* paramId, const char* label,
                                                   int column, int row)
{
    auto knob = std::make_unique<Knob>();
    knob->label = label;
    knob->paramId = paramId;
    knob->column = column;
    knob->row = row;

    knob->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::NoTextBox);
    knob->slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId,
                             EchoMatrixLookAndFeel::cyan());
    knob->slider->setMouseDragSensitivity (180);

    knob->slider->onValueChange = [this, index = knobs.size()]
    {
        if (index < knobs.size())
            repaint (knobs[(std::size_t) index]->cell);
    };

    addAndMakeVisible (*knob->slider);

    knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, *knob->slider);

    knobs.push_back (std::move (knob));
}

void EchoMatrixModeAudioProcessorEditor::resized()
{
    presetBox.setBounds (presetBarArea.getX() + 58, presetBarArea.getY() + 2, 250, 30);
    saveButton.setBounds (presetBarArea.getX() + 320, presetBarArea.getY() + 3, 64, 24);
    deleteButton.setBounds (presetBarArea.getX() + 392, presetBarArea.getY() + 3, 54, 24);
    syncButton.setBounds (presetBarArea.getRight() - 92, presetBarArea.getY() + 2, 92, 30);

    // Eight knobs in the delay card.
    {
        const auto content = juce::Rectangle<int> (globalArea.getX() + 16,
                                                   globalArea.getY() + 38,
                                                   globalArea.getWidth() - 32,
                                                   globalArea.getHeight() - 50);
        const auto cellWidth = content.getWidth() / 4;
        const auto cellHeight = content.getHeight() / 2;

        for (auto& knob : knobs)
        {
            const auto cell = juce::Rectangle<int> (content.getX() + knob->column * cellWidth,
                                                    content.getY() + knob->row * cellHeight,
                                                    cellWidth, cellHeight);

            knob->cell = cell;

            const auto size = juce::jmin (64, cellWidth - 16);
            knob->slider->setBounds (cell.getCentreX() - size / 2, cell.getY() + 2, size, size);
        }
    }

    // The 16 pads: eight columns by two rows.
    {
        const auto content = patternArea.reduced (16, 42);
        const auto cellWidth = content.getWidth() / 8;
        const auto cellHeight = content.getHeight() / 2;

        for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
        {
            const auto pad = juce::Rectangle<int> (content.getX() + (i % 8) * cellWidth + 3,
                                                   content.getY() + (i / 8) * cellHeight + 2,
                                                   cellWidth - 6, cellHeight - 4);

            stepButtons[(std::size_t) i]->setBounds (pad);
        }
    }

    // The selected step controls.
    {
        const auto left = stepArea.getX() + 16;
        const auto width = stepArea.getWidth() - 32;

        stepLevelSlider->setBounds (left, stepArea.getY() + 94, width, 26);
        stepPanSlider->setBounds (left, stepArea.getY() + 142, width, 26);
    }

    updatePresetButtons();
}

void EchoMatrixModeAudioProcessorEditor::selectStep (int step)
{
    selectedStep = juce::jlimit (0, echomatrix::Engine::kNumSteps - 1, step);

    stepLevelAttachment.reset();
    stepPanAttachment.reset();

    stepLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts,
        EchoMatrixModeAudioProcessor::stepParamId (ids::stepLevel, selectedStep),
        *stepLevelSlider);

    stepPanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts,
        EchoMatrixModeAudioProcessor::stepParamId (ids::stepPan, selectedStep),
        *stepPanSlider);

    // The read-outs have to be written with our own formatter.
    stepLevelSlider->updateText();
    stepPanSlider->updateText();

    updateStepSelectionUI();
}

void EchoMatrixModeAudioProcessorEditor::syncStepButtons()
{
    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
    {
        const auto on = processor.apvts.getRawParameterValue (
            EchoMatrixModeAudioProcessor::stepParamId (ids::stepEnabled, i))->load() > 0.5f;

        stepButtons[(std::size_t) i]->setToggleState (on, juce::dontSendNotification);
    }
}

void EchoMatrixModeAudioProcessorEditor::updateStepSelectionUI()
{
    for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
        stepButtons[(std::size_t) i]->setColour (
            kSelectedPadColourId,
            i == selectedStep ? kSelectedPadMarker : juce::Colour (0x00000000));

    repaint (stepArea);
    repaint (patternArea);
}

//==============================================================================
void EchoMatrixModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (EchoMatrixLookAndFeel::base());

    // Two coloured corners, so the plate does not look like a dead rectangle.
    {
        juce::ColourGradient glow (EchoMatrixLookAndFeel::cyan().withAlpha (0.13f),
                                   0.0f, 0.0f,
                                   EchoMatrixLookAndFeel::magenta().withAlpha (0.0f),
                                   (float) kWidth * 0.7f, (float) kHeight * 0.6f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    {
        juce::ColourGradient glow (EchoMatrixLookAndFeel::magenta().withAlpha (0.09f),
                                   (float) kWidth, (float) kHeight,
                                   EchoMatrixLookAndFeel::cyan().withAlpha (0.0f),
                                   (float) kWidth * 0.3f, (float) kHeight * 0.3f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    // A very faint dot grid: the plate is printed, not glassy.
    g.setColour (EchoMatrixLookAndFeel::ink().withAlpha (0.030f));

    for (int y = 0; y < kHeight; y += 16)
        for (int x = (y / 16) % 2 == 0 ? 8 : 0; x < kWidth; x += 16)
            g.fillRect (x, y, 1, 1);

    paintBaseplate (g);
    paintHeader (g);
    paintPresetBar (g);
    paintGlobalSection (g);
    paintStepEditor (g);
    paintPatternGrid (g);
    paintMeterSection (g);

    g.setColour (EchoMatrixLookAndFeel::line().withAlpha (0.9f));
    g.drawRect (getLocalBounds(), 1);
}

void EchoMatrixModeAudioProcessorEditor::paintBaseplate (juce::Graphics& g)
{
    const auto base = juce::Rectangle<int> (0, 0, kWidth, kHeight);

    g.setColour (juce::Colour (0xff0e1016));
    g.fillRoundedRectangle (base.toFloat(), 24.0f);

    g.setGradientFill (EchoMatrixLookAndFeel::accentGradient (base.toFloat()));
    g.drawRoundedRectangle (base.reduced (1).toFloat(), 23.0f, 2.0f);
}

void EchoMatrixModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const auto titleArea = juce::Rectangle<int> (headerArea.getX(), headerArea.getY() - 4,
                                                 640, 54);

    g.setFont (lookAndFeel.titleFont (46.0f));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawText ("ECHO MATRIX", titleArea.translated (0, 3), juce::Justification::centredLeft, false);

    g.setGradientFill (EchoMatrixLookAndFeel::accentGradient (titleArea.toFloat()));
    g.drawText ("ECHO MATRIX", titleArea, juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (10.0f));
    g.setColour (EchoMatrixLookAndFeel::ink().withAlpha (0.60f));
    drawTracked (g, "PATTERN DELAY - EVERY REPEAT IS AN INDEPENDENT EVENT",
                 juce::Rectangle<int> (headerArea.getX() + 4, headerArea.getY() + 48, 640, 16),
                 0.18f, juce::Justification::centredLeft);

    // The maker mark on the right.
    g.setFont (lookAndFeel.titleFont (20.0f));
    g.setColour (EchoMatrixLookAndFeel::ink().withAlpha (0.85f));
    drawTracked (g, "NULLCAM",
                 juce::Rectangle<int> (headerArea.getRight() - 300, headerArea.getY() + 12, 300, 26),
                 0.30f, juce::Justification::centredRight);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "16 STEPS - 64 EVENT VOICES - VST3",
                 juce::Rectangle<int> (headerArea.getRight() - 300, headerArea.getY() + 40, 300, 16),
                 0.18f, juce::Justification::centredRight);
}

void EchoMatrixModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    paintCard (g, presetBarArea);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "PRESET",
                 juce::Rectangle<int> (presetBarArea.getX() + 16, presetBarArea.getY() + 11, 40, 14),
                 0.20f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (edited ? EchoMatrixLookAndFeel::magenta()
                        : EchoMatrixLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, edited ? "EDITED" : "FACTORY",
                 juce::Rectangle<int> (presetBarArea.getX() + 456, presetBarArea.getY() + 11, 80, 14),
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "FOLLOW THE HOST TEMPO",
                 juce::Rectangle<int> (presetBarArea.getRight() - 300, presetBarArea.getY() + 11, 196, 14),
                 0.14f, juce::Justification::centredRight);
}

void EchoMatrixModeAudioProcessorEditor::paintGlobalSection (juce::Graphics& g)
{
    paintCard (g, globalArea);

    const auto left = globalArea.getX() + 16;
    const auto width = globalArea.getWidth() - 32;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "DELAY ENGINE",
                 juce::Rectangle<int> (left, globalArea.getY() + 13, width, 14),
                 0.22f, juce::Justification::centredLeft);

    for (const auto& knob : knobs)
    {
        const auto& cell = knob->cell;

        g.setFont (lookAndFeel.labelFont (9.5f));
        g.setColour (EchoMatrixLookAndFeel::dim());
        drawTracked (g, knob->label,
                     juce::Rectangle<int> (cell.getX(), cell.getY() + 68, cell.getWidth(), 13),
                     0.18f, juce::Justification::centred);

        g.setFont (lookAndFeel.valueFont (11.0f));
        g.setColour (EchoMatrixLookAndFeel::cyan());
        g.drawText (valueText (*knob),
                    juce::Rectangle<int> (cell.getX(), cell.getY() + 82, cell.getWidth(), 13),
                    juce::Justification::centred, false);
    }
}

void EchoMatrixModeAudioProcessorEditor::paintPatternGrid (juce::Graphics& g)
{
    paintCard (g, patternArea);

    const auto left = patternArea.getX() + 16;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "PATTERN - 16 STEPS",
                 juce::Rectangle<int> (left, patternArea.getY() + 13, 300, 14),
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::cyan().withAlpha (0.85f));
    drawTracked (g, "SELECT A STEP TO EDIT ITS VOICE",
                 juce::Rectangle<int> (patternArea.getRight() - 400, patternArea.getY() + 13, 384, 14),
                 0.14f, juce::Justification::centredRight);
}

void EchoMatrixModeAudioProcessorEditor::paintStepEditor (juce::Graphics& g)
{
    paintCard (g, stepArea);

    const auto left = stepArea.getX() + 16;
    const auto width = stepArea.getWidth() - 32;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "STEP EDITOR",
                 juce::Rectangle<int> (left, stepArea.getY() + 13, width, 14),
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.titleFont (30.0f));
    g.setGradientFill (EchoMatrixLookAndFeel::accentGradient (
        juce::Rectangle<float> ((float) left, (float) stepArea.getY() + 28, 260.0f, 40.0f)));
    g.drawText ("STEP " + juce::String (selectedStep + 1),
                juce::Rectangle<int> (left, stepArea.getY() + 28, 260, 40),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "ONE VOICE PER STEP",
                 juce::Rectangle<int> (left + 250, stepArea.getY() + 44, width - 250, 14),
                 0.14f, juce::Justification::centredRight);

    g.setFont (lookAndFeel.labelFont (9.5f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "LEVEL",
                 juce::Rectangle<int> (left, stepArea.getY() + 78, 60, 14),
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.5f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "PAN",
                 juce::Rectangle<int> (left, stepArea.getY() + 126, 60, 14),
                 0.18f, juce::Justification::centredLeft);

    // What this step does, in plain words.
    const auto delay = effectiveDelaySeconds();
    const auto synced = paramValue (ids::sync) > 0.5f;
    const auto bpm = processor.getSnapshot().bpm.load();

    g.setFont (lookAndFeel.valueFont (10.0f));
    g.setColour (EchoMatrixLookAndFeel::ink().withAlpha (0.75f));

    juce::String info;

    if (synced && bpm > 0.0f)
        info = "TEMPO LOCKED AT " + juce::String ((int) (bpm + 0.5f)) + " BPM";
    else
        info = "REPEATS " + juce::String ((int) (delay * 1000.0f + 0.5f)) + " MS AFTER THE INPUT";

    g.drawText (info,
                juce::Rectangle<int> (left, stepArea.getY() + 180, width, 16),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (EchoMatrixLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, "THE NEW REPEAT NEVER TOUCHES THE ONES ALREADY RINGING",
                 juce::Rectangle<int> (left, stepArea.getY() + 202, width, 14),
                 0.10f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (EchoMatrixLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, "PHASE 2: PITCH, REVERSE, FILTERS AND FX PER STEP",
                 juce::Rectangle<int> (left, stepArea.getY() + 218, width, 14),
                 0.10f, juce::Justification::centredLeft);
}

void EchoMatrixModeAudioProcessorEditor::paintMeterSection (juce::Graphics& g)
{
    paintCard (g, meterArea);

    const auto top = meterArea.getY() + 12;

    // --- active repeats ---
    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (EchoMatrixLookAndFeel::dim());
    drawTracked (g, "ACTIVE REPEATS",
                 juce::Rectangle<int> (meterArea.getX() + 16, top, 160, 13),
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.valueFont (24.0f));
    g.setColour (EchoMatrixLookAndFeel::cyan());
    g.drawText (juce::String ((int) (activeEvents + 0.5f)),
                juce::Rectangle<int> (meterArea.getX() + 16, top + 16, 70, 28),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (EchoMatrixLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, "RINGING",
                 juce::Rectangle<int> (meterArea.getX() + 56, top + 28, 60, 13),
                 0.16f, juce::Justification::centredLeft);

    // --- wet meter ---
    {
        const auto x = meterArea.getX() + 190;
        const auto w = 280;

        g.setFont (lookAndFeel.labelFont (8.5f));
        g.setColour (EchoMatrixLookAndFeel::dim());
        drawTracked (g, "WET",
                     juce::Rectangle<int> (x, top, 60, 13),
                     0.18f, juce::Justification::centredLeft);

        auto bar = juce::Rectangle<float> ((float) x, (float) top + 20.0f, (float) w, 12.0f);

        g.setColour (juce::Colour (0xff0c0e13));
        g.fillRoundedRectangle (bar, 6.0f);
        g.setColour (EchoMatrixLookAndFeel::line());
        g.drawRoundedRectangle (bar, 6.0f, 1.0f);

        if (wetLevel > 0.002f)
        {
            auto lit = bar.reduced (2.0f);
            lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth()
                                             * juce::jlimit (0.0f, 1.0f, wetLevel)));
            g.setGradientFill (EchoMatrixLookAndFeel::accentGradient (bar));
            g.fillRoundedRectangle (lit, 5.0f);
        }
    }

    // --- pattern position ---
    {
        const auto x = meterArea.getX() + 500;
        const auto w = meterArea.getRight() - 16 - x;

        g.setFont (lookAndFeel.labelFont (8.5f));
        g.setColour (EchoMatrixLookAndFeel::dim());
        drawTracked (g, "PATTERN POSITION",
                     juce::Rectangle<int> (x, top, 200, 13),
                     0.18f, juce::Justification::centredLeft);

        const auto segWidth = (float) w / (float) echomatrix::Engine::kNumSteps;

        for (int i = 0; i < echomatrix::Engine::kNumSteps; ++i)
        {
            const auto seg = juce::Rectangle<float> ((float) x + (float) i * segWidth + 1.0f,
                                                     (float) top + 20.0f,
                                                     segWidth - 3.0f, 12.0f);

            const auto isCurrent = i == currentStep;
            const auto isOn = processor.apvts.getRawParameterValue (
                EchoMatrixModeAudioProcessor::stepParamId (ids::stepEnabled, i))->load() > 0.5f;

            g.setColour (isCurrent ? EchoMatrixLookAndFeel::cyan().withAlpha (0.85f)
                                   : (isOn ? EchoMatrixLookAndFeel::line().brighter (0.25f)
                                           : juce::Colour (0xff171a22)));
            g.fillRoundedRectangle (seg, 3.0f);

            if (isCurrent)
            {
                auto lit = seg.reduced (1.0f);
                lit = lit.withWidth (juce::jmax (2.0f, lit.getWidth()
                                                 * juce::jlimit (0.0f, 1.0f, stepPhase)));
                g.setColour (EchoMatrixLookAndFeel::ink().withAlpha (0.9f));
                g.fillRoundedRectangle (lit, 2.0f);
            }
        }
    }
}

//==============================================================================
void EchoMatrixModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < EchoMatrixModeAudioProcessor::kNumPresets; ++i)
        presetBox.addItem (EchoMatrixModeAudioProcessor::getFactoryPresetName (
                               (EchoMatrixModeAudioProcessor::Preset) i), i + 1);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MINE");

        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }

    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        for (int i = 0; i < EchoMatrixModeAudioProcessor::kNumPresets; ++i)
            if (matched == EchoMatrixModeAudioProcessor::getFactoryPresetName (
                               (EchoMatrixModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);
    }
    else if (userPresets.contains (presetName))
    {
        presetBox.setSelectedId (100 + userPresets.indexOf (presetName), juce::dontSendNotification);
    }
    else
    {
        presetBox.setSelectedId (0, juce::dontSendNotification);
    }

    updatePresetButtons();
}

void EchoMatrixModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId >= 1 && itemId <= EchoMatrixModeAudioProcessor::kNumPresets)
    {
        processor.applyPreset ((EchoMatrixModeAudioProcessor::Preset) (itemId - 1));
        setPresetName (EchoMatrixModeAudioProcessor::getFactoryPresetName (
            (EchoMatrixModeAudioProcessor::Preset) (itemId - 1)));
        syncStepButtons();
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
            syncStepButtons();
            edited = false;
            repaint();
        }
    }
}

void EchoMatrixModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;

    if (userPresets.contains (name))
        presetBox.setSelectedId (100 + userPresets.indexOf (name), juce::dontSendNotification);
    else
        for (int i = 0; i < EchoMatrixModeAudioProcessor::kNumPresets; ++i)
            if (name == EchoMatrixModeAudioProcessor::getFactoryPresetName (
                            (EchoMatrixModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);

    updatePresetButtons();
}

void EchoMatrixModeAudioProcessorEditor::updatePresetButtons()
{
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void EchoMatrixModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || suggested == "CUSTOM" || userPresets.contains (suggested))
        suggested = "MY ECHO";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + processor.getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<EchoMatrixModeAudioProcessorEditor> safe (this);

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

void EchoMatrixModeAudioProcessorEditor::promptDeletePreset()
{
    if (activeDialog != nullptr || ! userPresets.contains (presetName))
        return;

    auto* dialog = new juce::AlertWindow ("DELETE PRESET",
                                          "Delete \"" + presetName + "\"?\n\n"
                                              "The file is removed from the preset folder.",
                                          juce::MessageBoxIconType::WarningIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addButton ("DELETE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<EchoMatrixModeAudioProcessorEditor> safe (this);

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

void EchoMatrixModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();

    // The pads follow the parameters: a preset load or an automation move has
    // to be visible on the grid straight away.
    syncStepButtons();

    // The factory match does not need to run at full frame rate.
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

void EchoMatrixModeAudioProcessorEditor::pullLiveData()
{
    const auto& snapshot = processor.getSnapshot();

    activeEvents += ((float) snapshot.activeEvents.load (std::memory_order_relaxed) - activeEvents) * 0.25f;
    wetLevel += (snapshot.wetLevel.load (std::memory_order_relaxed) - wetLevel) * 0.25f;

    currentStep = snapshot.currentStep.load (std::memory_order_relaxed);
    stepPhase = snapshot.stepPhase.load (std::memory_order_relaxed);

    repaint (meterArea);
    repaint (stepArea);
}

float EchoMatrixModeAudioProcessorEditor::paramValue (const juce::String& id) const
{
    if (auto* value = processor.apvts.getRawParameterValue (id))
        return value->load();

    return 0.0f;
}

float EchoMatrixModeAudioProcessorEditor::effectiveDelaySeconds() const
{
    const auto bpm = processor.getSnapshot().bpm.load (std::memory_order_relaxed);

    if (paramValue (ids::sync) > 0.5f && bpm > 0.0f)
        return 60.0f / bpm;

    return paramValue (ids::delayTime);
}

juce::String EchoMatrixModeAudioProcessorEditor::valueText (const Knob& knob) const
{
    const auto value = (float) knob.slider->getValue();

    if (knob.paramId == juce::String (ids::delayTime))
        return juce::String ((int) (value * 1000.0f + 0.5f)) + " MS";

    if (knob.paramId == juce::String (ids::hpfCutoff)
        || knob.paramId == juce::String (ids::lpfCutoff))
        return value >= 1000.0f ? juce::String (value / 1000.0f, 1) + " K"
                                : juce::String ((int) (value + 0.5f)) + " HZ";

    if (knob.paramId == juce::String (ids::inputGain)
        || knob.paramId == juce::String (ids::outputGain))
    {
        if (value <= 0.0001f)
            return "-INF DB";

        return juce::String (20.0f * std::log10 (value), 1) + " DB";
    }

    return juce::String ((int) (value * 100.0f + 0.5f)) + "%";
}
