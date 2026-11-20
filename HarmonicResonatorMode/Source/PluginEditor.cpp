#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int kWidth = 1180;
constexpr int kHeight = 760;
constexpr int kMargin = 22;

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

    juce::ColourGradient fill (HarmonicLookAndFeel::cardTop(), r.getX(), r.getY(),
                               HarmonicLookAndFeel::card(), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 16.0f);

    g.setGradientFill (HarmonicLookAndFeel::accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 15.0f, 1.5f);
}

// One horizontal meter: dark rail, accent fill, tick at the top.
void paintBar (juce::Graphics& g, juce::Rectangle<int> area, float level, juce::Colour colour)
{
    const auto r = area.toFloat();

    g.setColour (juce::Colour (0xff0c0e13));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (HarmonicLookAndFeel::line());
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

    if (level > 0.002f)
    {
        auto lit = r.reduced (2.0f);
        lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth()
                                            * juce::jlimit (0.0f, 1.0f, level)));

        g.setColour (colour);
        g.fillRoundedRectangle (lit, lit.getHeight() * 0.5f);
    }
}

const char* sectionTitle (int index)
{
    static const char* titles[] = { "1 - HARMONICS", "2 - RESONATORS", "3 - INTERACTION",
                                    "4 - PARTIALS", "5 - MODULATION", "6 - OUTPUT" };
    return titles[juce::jlimit (0, 5, index)];
}
} // namespace

//==============================================================================
juce::Colour HarmonicLookAndFeel::base()    { return juce::Colour (0xff07080c); }
juce::Colour HarmonicLookAndFeel::card()    { return juce::Colour (0xff12141b); }
juce::Colour HarmonicLookAndFeel::cardTop() { return juce::Colour (0xff1c1f28); }
juce::Colour HarmonicLookAndFeel::line()    { return juce::Colour (0xff313644); }
juce::Colour HarmonicLookAndFeel::ink()     { return juce::Colour (0xfff2f5fb); }
juce::Colour HarmonicLookAndFeel::dim()     { return juce::Colour (0xff8b93a6); }
juce::Colour HarmonicLookAndFeel::amber()   { return juce::Colour (0xffffb340); }
juce::Colour HarmonicLookAndFeel::teal()    { return juce::Colour (0xff3ce0c4); }

juce::ColourGradient HarmonicLookAndFeel::accentGradient (juce::Rectangle<float> area)
{
    juce::ColourGradient gradient (amber(), area.getX(), area.getCentreY(),
                                   teal(), area.getRight(), area.getCentreY(), false);
    gradient.addColour (0.35, juce::Colour (0xffffe066));
    gradient.addColour (0.70, juce::Colour (0xff8ce8a0));
    return gradient;
}

HarmonicLookAndFeel::HarmonicLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff14161d));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, amber());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber());
    setColour (juce::PopupMenu::highlightedTextColourId, base());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff14161d));
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
}

juce::Font HarmonicLookAndFeel::titleFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font HarmonicLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

juce::Font HarmonicLookAndFeel::valueFont (float height) const
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                          height, juce::Font::plain));
}

void HarmonicLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
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
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tip));

    g.setColour (colour.withAlpha (0.95f));
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (centre));
}

juce::Label* HarmonicLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (valueFont (10.0f));
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void HarmonicLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);

    g.setColour (juce::Colour (0xff0f1117));
    g.fillRoundedRectangle (r, 9.0f);

    g.setGradientFill (accentGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 1.3f);

    const auto x = (float) width - 15.0f;
    const auto y = (float) height * 0.5f;

    juce::Path chevron;
    chevron.startNewSubPath (x - 4.0f, y - 2.0f);
    chevron.lineTo (x, y + 2.5f);
    chevron.lineTo (x + 4.0f, y - 2.0f);

    g.setColour (box.isEnabled() ? amber() : dim());
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

juce::Font HarmonicLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (10.5f).withExtraKerningFactor (0.06f);
}

juce::Font HarmonicLookAndFeel::getPopupMenuFont()
{
    return labelFont (11.0f);
}

void HarmonicLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff14161d));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void HarmonicLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::backgroundColourId));
    g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 6.0f);

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                      label.getJustificationType(), 1);
}

//==============================================================================
HarmonicResonatorModeAudioProcessorEditor::HarmonicResonatorModeAudioProcessorEditor (
    HarmonicResonatorModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    headerArea      = { kMargin, 10, kWidth - 2 * kMargin, 60 };
    presetBarArea   = { kMargin, 76, kWidth - 2 * kMargin, 34 };
    harmonicsArea   = { kMargin, 118, 396, 300 };
    resonatorArea   = { kMargin + 408, 118, 396, 300 };
    interactionArea = { kMargin + 816, 118, 320, 300 };
    partialsArea    = { kMargin, 428, 700, 162 };
    modArea         = { kMargin + 712, 428, 424, 162 };
    outputArea      = { kMargin, 600, kWidth - 2 * kMargin, 138 };

    // ---- the nonlinear stage ----
    addChoice (ids::shape, 0, 0);
    addKnob (ids::drive,      "DRIVE",    0, 0, 0);
    addKnob (ids::density,    "DENSITY",  0, 1, 0);
    addKnob (ids::brightness, "BRIGHT",   0, 2, 0);
    addKnob (ids::evenOdd,    "EVEN/ODD", 0, 0, 1);
    addKnob (ids::symmetry,   "SYMMETRY", 0, 1, 1);

    // ---- the resonator bank ----
    addChoice (ids::tuning, 1, 0);
    addKnob (ids::tune,      "TUNE",   1, 0, 0);
    addKnob (ids::resonance, "RESO",   1, 1, 0);
    addKnob (ids::decay,     "DECAY",  1, 2, 0);
    addKnob (ids::resGain,   "BANK",   1, 0, 1);
    addKnob (ids::spread,    "SPREAD", 1, 1, 1);
    addKnob (ids::align,     "ALIGN",  1, 2, 1);

    // ---- the loop between them ----
    addChoice (ids::routing, 2, 0);
    addKnob (ids::interaction, "INTERACT", 2, 0, 0);
    addKnob (ids::feedback,    "FEEDBK",   2, 1, 0);
    addKnob (ids::fbDrive,     "FB DRIVE", 2, 2, 0);
    addKnob (ids::lowCut,      "LOW CUT",  2, 3, 0);
    addKnob (ids::highCut,     "HIGH CUT", 2, 4, 0);
    addChoice (ids::source, 2, 1);
    addChoice (ids::destination, 2, 2);

    // ---- modulation ----
    addChoice (ids::modMode, 4, 0);
    addChoice (ids::modDest, 4, 1);
    addKnob (ids::modRate,  "RATE",    4, 0, 0);
    addKnob (ids::modDepth, "DEPTH",   4, 1, 0);
    addKnob (ids::envDepth, "ENV>DRV", 4, 2, 0);

    // ---- output ----
    addKnob (ids::inputGain,   "INPUT",       3, 0, 0);
    addKnob (ids::outputGain,  "OUTPUT",      3, 1, 0);
    addKnob (ids::mix,         "DRY / WET",   3, 2, 0);
    addKnob (ids::fundamental, "FUNDAMENTAL", 3, 3, 0);

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
    presetName = matched.isNotEmpty() ? matched : "BELL METAL";
    applyingPreset = true;
    setPresetName (presetName);
    applyingPreset = false;

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

HarmonicResonatorModeAudioProcessorEditor::~HarmonicResonatorModeAudioProcessorEditor()
{
    stopTimer();

    if (activeDialog != nullptr)
        activeDialog->exitModalState (0);

    setLookAndFeel (nullptr);
}

void HarmonicResonatorModeAudioProcessorEditor::addKnob (const char* paramId, const char* label,
                                                          int group, int column, int row)
{
    auto knob = std::make_unique<Knob>();
    knob->label = label;
    knob->paramId = paramId;
    knob->group = group;
    knob->column = column;
    knob->row = row;

    knob->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::NoTextBox);
    knob->slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId,
                             HarmonicLookAndFeel::amber());
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

void HarmonicResonatorModeAudioProcessorEditor::addChoice (const char* paramId, int group, int slot)
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

    choice->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, paramId, *choice->box);

    choices.push_back (std::move (choice));
}

void HarmonicResonatorModeAudioProcessorEditor::layoutKnobGroup (int group,
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

        const auto cell = juce::Rectangle<int> (content.getX() + knob->column * cellWidth,
                                                content.getY() + knob->row * cellHeight,
                                                cellWidth, cellHeight);

        knob->cell = cell;

        const auto top = cell.getY() + 2;

        knob->slider->setBounds (cell.getCentreX() - knobSize / 2, top, knobSize, knobSize);
        knob->labelBounds = { cell.getX(), top + knobSize + 4, cellWidth, 12 };
        knob->valueBounds = { cell.getX(), top + knobSize + 16, cellWidth, 13 };
    }
}

void HarmonicResonatorModeAudioProcessorEditor::resized()
{
    presetBox.setBounds (presetBarArea.getX() + 58, presetBarArea.getY() + 2, 250, 30);
    saveButton.setBounds (presetBarArea.getX() + 320, presetBarArea.getY() + 3, 64, 24);
    deleteButton.setBounds (presetBarArea.getX() + 392, presetBarArea.getY() + 3, 54, 24);

    layoutKnobGroup (0, { harmonicsArea.getX() + 14, harmonicsArea.getY() + 98,
                          harmonicsArea.getWidth() - 28, harmonicsArea.getHeight() - 110 },
                     3, 2, 54);

    layoutKnobGroup (1, { resonatorArea.getX() + 14, resonatorArea.getY() + 98,
                          resonatorArea.getWidth() - 28, resonatorArea.getHeight() - 110 },
                     3, 2, 54);

    layoutKnobGroup (2, { interactionArea.getX() + 10, interactionArea.getY() + 100,
                          interactionArea.getWidth() - 20, 92 },
                     5, 1, 48);

    layoutKnobGroup (3, { outputArea.getX() + 14, outputArea.getY() + 30,
                          4 * 108, outputArea.getHeight() - 38 },
                     4, 1, 60);

    layoutKnobGroup (4, { modArea.getX() + 14, modArea.getY() + 64,
                          modArea.getWidth() - 28, modArea.getHeight() - 72 },
                     3, 1, 56);

    for (auto& choice : choices)
    {
        auto bounds = juce::Rectangle<int>();

        switch (choice->group)
        {
            case 0:
                bounds = { harmonicsArea.getX() + 14, harmonicsArea.getY() + 32,
                           harmonicsArea.getWidth() - 28, 26 };
                break;

            case 1:
                bounds = { resonatorArea.getX() + 14, resonatorArea.getY() + 32,
                           resonatorArea.getWidth() - 28, 26 };
                break;

            case 2:
            {
                if (choice->slot == 0)
                {
                    bounds = { interactionArea.getX() + 10, interactionArea.getY() + 30,
                               interactionArea.getWidth() - 20, 24 };
                }
                else
                {
                    const auto width = (interactionArea.getWidth() - 20 - 8) / 2;

                    bounds = { interactionArea.getX() + 10 + (choice->slot - 1) * (width + 8),
                               interactionArea.getY() + 204, width, 24 };
                }
                break;
            }

            case 4:
            default:
            {
                const auto width = (modArea.getWidth() - 28 - 8) / 2;

                bounds = { modArea.getX() + 14 + choice->slot * (width + 8),
                           modArea.getY() + 32, width, 26 };
                break;
            }
        }

        choice->box->setBounds (bounds);
    }
}

//==============================================================================
void HarmonicResonatorModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (HarmonicLookAndFeel::base());

    // Two coloured corners, so the plate does not look like a dead rectangle.
    {
        juce::ColourGradient glow (HarmonicLookAndFeel::amber().withAlpha (0.12f),
                                   0.0f, 0.0f,
                                   HarmonicLookAndFeel::teal().withAlpha (0.0f),
                                   (float) kWidth * 0.7f, (float) kHeight * 0.6f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    {
        juce::ColourGradient glow (HarmonicLookAndFeel::teal().withAlpha (0.08f),
                                   (float) kWidth, (float) kHeight,
                                   HarmonicLookAndFeel::amber().withAlpha (0.0f),
                                   (float) kWidth * 0.3f, (float) kHeight * 0.3f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    // A very faint dot grid: the plate is printed, not glassy.
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.030f));

    for (int y = 0; y < kHeight; y += 16)
        for (int x = (y / 16) % 2 == 0 ? 8 : 0; x < kWidth; x += 16)
            g.fillRect (x, y, 1, 1);

    paintBaseplate (g);
    paintHeader (g);
    paintPresetBar (g);
    paintHarmonics (g);
    paintResonators (g);
    paintInteraction (g);
    paintPartials (g);
    paintModulation (g);
    paintOutput (g);

    g.setColour (HarmonicLookAndFeel::line().withAlpha (0.9f));
    g.drawRect (getLocalBounds(), 1);
}

void HarmonicResonatorModeAudioProcessorEditor::paintBaseplate (juce::Graphics& g)
{
    const auto base = juce::Rectangle<int> (0, 0, kWidth, kHeight);

    g.setColour (juce::Colour (0xff0e1016));
    g.fillRoundedRectangle (base.toFloat(), 24.0f);

    g.setGradientFill (HarmonicLookAndFeel::accentGradient (base.toFloat()));
    g.drawRoundedRectangle (base.reduced (1).toFloat(), 23.0f, 2.0f);
}

void HarmonicResonatorModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const auto titleArea = juce::Rectangle<int> (headerArea.getX(), headerArea.getY() - 4,
                                                 720, 54);

    g.setFont (lookAndFeel.titleFont (44.0f));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawText ("HARMONIC RESONATOR", titleArea.translated (0, 3),
                juce::Justification::centredLeft, false);

    g.setGradientFill (HarmonicLookAndFeel::accentGradient (titleArea.toFloat()));
    g.drawText ("HARMONIC RESONATOR", titleArea, juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (10.0f));
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.60f));
    drawTracked (g, "DISTORTION AND RESONANCE THAT DRIVE EACH OTHER",
                 juce::Rectangle<int> (headerArea.getX() + 4, headerArea.getY() + 48, 620, 16),
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.titleFont (20.0f));
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.85f));
    drawTracked (g, "NULLCAM",
                 juce::Rectangle<int> (headerArea.getRight() - 300, headerArea.getY() + 10, 300, 26),
                 0.30f, juce::Justification::centredRight);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, "8 PARTIALS - CLOSED LOOP - VST3",
                 juce::Rectangle<int> (headerArea.getRight() - 300, headerArea.getY() + 38, 300, 16),
                 0.18f, juce::Justification::centredRight);
}

void HarmonicResonatorModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    paintCard (g, presetBarArea);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, "PRESET",
                 juce::Rectangle<int> (presetBarArea.getX() + 16, presetBarArea.getY() + 11, 40, 14),
                 0.20f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (edited ? HarmonicLookAndFeel::teal()
                        : HarmonicLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, edited ? "EDITED" : "FACTORY",
                 juce::Rectangle<int> (presetBarArea.getX() + 456, presetBarArea.getY() + 11, 80, 14),
                 0.18f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, "EIGHT FACTORY PRESETS - YOURS SAVE AS XML",
                 juce::Rectangle<int> (presetBarArea.getRight() - 340, presetBarArea.getY() + 11, 320, 14),
                 0.14f, juce::Justification::centredRight);
}

void HarmonicResonatorModeAudioProcessorEditor::paintKnobs (juce::Graphics& g, int group)
{
    for (const auto& knob : knobs)
    {
        if (knob->group != group)
            continue;

        g.setFont (lookAndFeel.labelFont (9.0f));
        g.setColour (HarmonicLookAndFeel::dim());
        drawTracked (g, knob->label, knob->labelBounds, 0.16f,
                     juce::Justification::centred);

        g.setFont (lookAndFeel.valueFont (10.5f));
        g.setColour (group == 2 ? HarmonicLookAndFeel::teal() : HarmonicLookAndFeel::amber());
        g.drawText (valueText (*knob), knob->valueBounds,
                    juce::Justification::centred, false);
    }
}

void HarmonicResonatorModeAudioProcessorEditor::paintHarmonics (juce::Graphics& g)
{
    paintCard (g, harmonicsArea);

    const auto left = harmonicsArea.getX() + 14;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (0),
                 juce::Rectangle<int> (left, harmonicsArea.getY() + 13, 300, 14),
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, "THE NONLINEAR STAGE",
                 juce::Rectangle<int> (harmonicsArea.getRight() - 220, harmonicsArea.getY() + 13, 206, 14),
                 0.14f, juce::Justification::centredRight);

    paintKnobs (g, 0);
}

void HarmonicResonatorModeAudioProcessorEditor::paintResonators (juce::Graphics& g)
{
    paintCard (g, resonatorArea);

    const auto left = resonatorArea.getX() + 14;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (1),
                 juce::Rectangle<int> (left, resonatorArea.getY() + 13, 300, 14),
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, "WHAT THE HARMONICS LAND ON",
                 juce::Rectangle<int> (resonatorArea.getRight() - 240, resonatorArea.getY() + 13, 226, 14),
                 0.14f, juce::Justification::centredRight);

    paintKnobs (g, 1);

    // The tracked pitch, when the bank is following the input.
    const auto tracked = processor.getSnapshot().tracked.load (std::memory_order_relaxed);
    const auto trusted = processor.getSnapshot().confidence.load (std::memory_order_relaxed);

    g.setFont (lookAndFeel.valueFont (9.0f));
    g.setColour (trusted > 0.55f ? HarmonicLookAndFeel::teal()
                                 : HarmonicLookAndFeel::dim().withAlpha (0.7f));

    const auto text = tracked > 30.0f
                        ? "TRACK " + juce::String ((int) (tracked + 0.5f)) + " HZ"
                        : juce::String ("TRACK -");

    g.drawText (text,
                juce::Rectangle<int> (left, resonatorArea.getBottom() - 24, 220, 14),
                juce::Justification::centredLeft, false);
}

void HarmonicResonatorModeAudioProcessorEditor::paintInteraction (juce::Graphics& g)
{
    paintCard (g, interactionArea);

    const auto left = interactionArea.getX() + 10;
    const auto width = interactionArea.getWidth() - 20;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (2),
                 juce::Rectangle<int> (left, interactionArea.getY() + 13, 200, 14),
                 0.22f, juce::Justification::centredLeft);

    paintKnobs (g, 2);

    // ---- the loop, drawn from the live coupling ----
    const auto interaction = paramValue (ids::interaction);
    const auto feedback = paramValue (ids::feedback);
    const auto routing = (int) paramValue (ids::routing);
    const auto coupling = juce::jlimit (0.0f, 1.0f, interaction * feedback);

    const auto diagram = juce::Rectangle<int> (left, interactionArea.getY() + 230, width, 60);

    auto drawBox = [&] (juce::Rectangle<int> box, const juce::String& text, float heat)
    {
        const auto r = box.toFloat();

        g.setColour (HarmonicLookAndFeel::cardTop().darker (0.1f).withAlpha (0.9f));
        g.fillRoundedRectangle (r, 7.0f);

        g.setColour (HarmonicLookAndFeel::line().interpolatedWith (HarmonicLookAndFeel::amber(),
                                                                   juce::jlimit (0.0f, 1.0f, heat)));
        g.drawRoundedRectangle (r, 7.0f, 1.4f);

        g.setFont (lookAndFeel.labelFont (9.5f));
        g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.55f + 0.45f * heat));
        g.drawText (text, box, juce::Justification::centred, false);
    };

    const auto boxWidth = 82;
    const auto boxHeight = 28;

    const auto distBox = juce::Rectangle<int> (diagram.getX(), diagram.getY() + 16,
                                               boxWidth, boxHeight);
    const auto resBox = juce::Rectangle<int> (diagram.getRight() - boxWidth, diagram.getY() + 16,
                                              boxWidth, boxHeight);

    drawBox (distBox, "DIST", 1.0f);
    drawBox (resBox, "RES", 1.0f);

    // The forward edge is always open in DIST > RES, and follows the coupling
    // in the other two modes. The return edge is the coupling itself.
    const auto forwardOpen = routing == 0 ? 1.0f : coupling;
    const auto returnOpen = coupling;

    auto drawArrow = [&] (float fromX, float toX, float y, float heat, bool toRight)
    {
        const auto alpha = 0.18f + 0.72f * heat;

        g.setColour (HarmonicLookAndFeel::teal().withAlpha (alpha));
        g.drawLine (fromX, y, toX, y, 1.0f + 4.0f * heat);

        const auto tip = toX;
        const auto direction = toRight ? 1.0f : -1.0f;

        juce::Path head;
        head.startNewSubPath (tip, y);
        head.lineTo (tip - direction * 7.0f, y - 4.0f);
        head.lineTo (tip - direction * 7.0f, y + 4.0f);
        head.closeSubPath();

        g.setColour (HarmonicLookAndFeel::teal().withAlpha (alpha));
        g.fillPath (head);
    };

    drawArrow ((float) (distBox.getRight() + 4), (float) (resBox.getX() - 4),
               (float) diagram.getY() + 12.0f, forwardOpen, true);

    drawArrow ((float) (resBox.getX() - 4), (float) (distBox.getRight() + 4),
               (float) diagram.getY() + 52.0f, returnOpen, false);

    g.setFont (lookAndFeel.labelFont (8.0f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, "CLOSED LOOP " + juce::String ((int) (coupling * 100.0f + 0.5f)) + " %",
                 juce::Rectangle<int> (left, diagram.getBottom() - 2, width, 12),
                 0.14f, juce::Justification::centred);
}

void HarmonicResonatorModeAudioProcessorEditor::paintPartials (juce::Graphics& g)
{
    paintCard (g, partialsArea);

    const auto left = partialsArea.getX() + 16;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (3),
                 juce::Rectangle<int> (left, partialsArea.getY() + 13, 300, 14),
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, "LIVE ENERGY PER RESONATOR - NOT A DRAWING OF THE KNOBS",
                 juce::Rectangle<int> (partialsArea.getRight() - 380, partialsArea.getY() + 13, 364, 14),
                 0.12f, juce::Justification::centredRight);

    const auto plot = juce::Rectangle<int> (partialsArea.getX() + 16, partialsArea.getY() + 36,
                                            partialsArea.getWidth() - 32, 92);

    // The frequency axis is logarithmic, 40 Hz to 16 kHz.
    const auto lowHz = 40.0f;
    const auto highHz = 16000.0f;
    const auto logSpan = std::log (highHz / lowHz);

    auto xFor = [&] (float hz)
    {
        const auto clamped = juce::jlimit (lowHz, highHz, hz);
        return (float) plot.getX()
               + (float) plot.getWidth() * std::log (clamped / lowHz) / logSpan;
    };

    // Rails and grid.
    g.setColour (HarmonicLookAndFeel::line().withAlpha (0.55f));
    g.drawLine ((float) plot.getX(), (float) plot.getBottom(),
                (float) plot.getRight(), (float) plot.getBottom(), 1.0f);

    static const float marks[] = { 55.0f, 110.0f, 220.0f, 440.0f, 880.0f, 1760.0f, 3520.0f, 7040.0f };

    for (auto hz : marks)
    {
        const auto x = xFor (hz);

        g.setColour (HarmonicLookAndFeel::line().withAlpha (0.30f));
        g.drawLine (x, (float) plot.getY(), x, (float) plot.getBottom(), 1.0f);

        g.setFont (lookAndFeel.valueFont (8.0f));
        g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.65f));

        const auto label = hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + "K"
                                         : juce::String ((int) hz);

        g.drawText (label,
                    juce::Rectangle<int> ((int) x - 20, plot.getBottom() + 2, 40, 11),
                    juce::Justification::centred, false);
    }

    // The partials themselves.
    for (int i = 0; i < harmonicresonator::kNumResonators; ++i)
    {
        const auto hz = partialFreq[(std::size_t) i];
        const auto energy = juce::jlimit (0.0f, 1.0f, partialEnergy[(std::size_t) i]);

        if (hz < 20.0f)
            continue;

        const auto x = xFor (hz);

        // A decibel scale, not a linear one: the quiet partials of a bank are
        // real, they are just a long way down, and a linear bar would hide
        // every one of them behind a flat line.
        const auto db = 20.0f * std::log10 (std::max (energy, 1.0e-6f));
        const auto level = juce::jlimit (0.0f, 1.0f, (db + 72.0f) / 72.0f);
        const auto height = juce::jmax (1.0f, level * (float) plot.getHeight());

        auto bar = juce::Rectangle<float> (x - 3.0f, (float) plot.getBottom() - height, 6.0f, height);

        g.setColour (HarmonicLookAndFeel::amber().withAlpha (0.20f));
        g.fillRoundedRectangle (bar.expanded (3.0f, 0.0f), 4.0f);

        g.setGradientFill (HarmonicLookAndFeel::accentGradient (bar));
        g.fillRoundedRectangle (bar, 3.0f);
    }

    // The read-out under the plot.
    g.setFont (lookAndFeel.valueFont (9.5f));
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.75f));

    g.drawText (juce::String (activePartials) + " OF 8 PARTIALS RINGING",
                juce::Rectangle<int> (left, partialsArea.getBottom() - 22, 240, 14),
                juce::Justification::centredLeft, false);

    g.setColour (HarmonicLookAndFeel::teal());
    g.drawText ("BANK " + juce::String (paramValue (ids::tune), 0) + " HZ",
                juce::Rectangle<int> (left + 250, partialsArea.getBottom() - 22, 200, 14),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (8.0f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.7f));
    drawTracked (g, "BAR HEIGHT = LEVEL (DB), POSITION = FREQUENCY",
                 juce::Rectangle<int> (partialsArea.getRight() - 330, partialsArea.getBottom() - 22, 314, 14),
                 0.10f, juce::Justification::centredRight);
}

void HarmonicResonatorModeAudioProcessorEditor::paintModulation (juce::Graphics& g)
{
    paintCard (g, modArea);

    const auto left = modArea.getX() + 14;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (4),
                 juce::Rectangle<int> (left, modArea.getY() + 13, 220, 14),
                 0.22f, juce::Justification::centredLeft);

    paintKnobs (g, 4);

    // The moving value, so the modulator is not just a promise.
    const auto& snapshot = processor.getSnapshot();
    const auto lfo = snapshot.lfoValue.load (std::memory_order_relaxed);
    const auto env = snapshot.envValue.load (std::memory_order_relaxed);

    const auto bar = juce::Rectangle<int> (left, modArea.getBottom() - 26, 110, 8);
    paintBar (g, bar, (lfo + 1.0f) * 0.5f, HarmonicLookAndFeel::teal());

    g.setFont (lookAndFeel.labelFont (8.0f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, "LFO", juce::Rectangle<int> (left, bar.getY() - 12, 110, 11),
                 0.14f, juce::Justification::centredLeft);

    const auto envBar = juce::Rectangle<int> (left + 130, modArea.getBottom() - 26, 110, 8);
    paintBar (g, envBar, env * 4.0f, HarmonicLookAndFeel::amber());

    g.setFont (lookAndFeel.labelFont (8.0f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, "FOLLOW", juce::Rectangle<int> (left + 130, envBar.getY() - 12, 110, 11),
                 0.14f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.valueFont (9.5f));
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.7f));
    g.drawText ("TRACKED " + juce::String ((int) (trackedHz + 0.5f)) + " HZ",
                juce::Rectangle<int> (left + 260, modArea.getBottom() - 30, 160, 14),
                juce::Justification::centredLeft, false);
}

void HarmonicResonatorModeAudioProcessorEditor::paintOutput (juce::Graphics& g)
{
    paintCard (g, outputArea);

    const auto left = outputArea.getX() + 14;

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, sectionTitle (5),
                 juce::Rectangle<int> (left, outputArea.getY() + 13, 200, 14),
                 0.22f, juce::Justification::centredLeft);

    paintKnobs (g, 3);

    // ---- meters ----
    const auto meterX = outputArea.getX() + 470;

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, "IN",
                 juce::Rectangle<int> (meterX, outputArea.getY() + 18, 200, 12),
                 0.18f, juce::Justification::centredLeft);
    paintBar (g, { meterX, outputArea.getY() + 32, 200, 10 }, inLevel,
              HarmonicLookAndFeel::dim().brighter (0.4f));

    g.setFont (lookAndFeel.labelFont (8.5f));
    g.setColour (HarmonicLookAndFeel::dim());
    drawTracked (g, "WET",
                 juce::Rectangle<int> (meterX + 220, outputArea.getY() + 18, 200, 12),
                 0.18f, juce::Justification::centredLeft);
    paintBar (g, { meterX + 220, outputArea.getY() + 32, 200, 10 }, wetLevel,
              HarmonicLookAndFeel::teal());

    g.setFont (lookAndFeel.valueFont (9.5f));
    g.setColour (HarmonicLookAndFeel::ink().withAlpha (0.75f));
    g.drawText (juce::String (activePartials) + " PARTIALS ACTIVE",
                juce::Rectangle<int> (meterX, outputArea.getY() + 56, 200, 14),
                juce::Justification::centredLeft, false);

    g.setColour (confidence > 0.55f ? HarmonicLookAndFeel::teal()
                                    : HarmonicLookAndFeel::dim());
    g.drawText (confidence > 0.55f
                    ? "PITCH LOCK " + juce::String ((int) (trackedHz + 0.5f)) + " HZ"
                    : juce::String ("PITCH FREE"),
                juce::Rectangle<int> (meterX + 220, outputArea.getY() + 56, 220, 14),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.labelFont (8.0f));
    g.setColour (HarmonicLookAndFeel::dim().withAlpha (0.7f));
    drawTracked (g, "THE DRY SIGNAL IS NEVER REMOVED, ONLY MIXED",
                 juce::Rectangle<int> (meterX, outputArea.getY() + 92, 440, 12),
                 0.10f, juce::Justification::centredLeft);
}

//==============================================================================
void HarmonicResonatorModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
        presetBox.addItem (HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
                               (HarmonicResonatorModeAudioProcessor::Preset) i), i + 1);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MINE");

        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }

    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
            if (matched == HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
                               (HarmonicResonatorModeAudioProcessor::Preset) i))
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

void HarmonicResonatorModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId >= 1 && itemId <= HarmonicResonatorModeAudioProcessor::kNumPresets)
    {
        processor.applyPreset ((HarmonicResonatorModeAudioProcessor::Preset) (itemId - 1));
        setPresetName (HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
            (HarmonicResonatorModeAudioProcessor::Preset) (itemId - 1)));
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

void HarmonicResonatorModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;

    if (userPresets.contains (name))
        presetBox.setSelectedId (100 + userPresets.indexOf (name), juce::dontSendNotification);
    else
        for (int i = 0; i < HarmonicResonatorModeAudioProcessor::kNumPresets; ++i)
            if (name == HarmonicResonatorModeAudioProcessor::getFactoryPresetName (
                            (HarmonicResonatorModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);

    updatePresetButtons();
}

void HarmonicResonatorModeAudioProcessorEditor::updatePresetButtons()
{
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void HarmonicResonatorModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || suggested == "CUSTOM" || userPresets.contains (suggested))
        suggested = "MY RESONANCE";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + processor.getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<HarmonicResonatorModeAudioProcessorEditor> safe (this);

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

void HarmonicResonatorModeAudioProcessorEditor::promptDeletePreset()
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

    juce::Component::SafePointer<HarmonicResonatorModeAudioProcessorEditor> safe (this);

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

void HarmonicResonatorModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();

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

void HarmonicResonatorModeAudioProcessorEditor::pullLiveData()
{
    const auto& snapshot = processor.getSnapshot();

    inLevel += (snapshot.inLevel.load (std::memory_order_relaxed) - inLevel) * 0.25f;
    wetLevel += (snapshot.wetLevel.load (std::memory_order_relaxed) - wetLevel) * 0.25f;
    trackedHz += (snapshot.tracked.load (std::memory_order_relaxed) - trackedHz) * 0.20f;
    confidence += (snapshot.confidence.load (std::memory_order_relaxed) - confidence) * 0.20f;

    activePartials = snapshot.activePartials.load (std::memory_order_relaxed);

    for (int i = 0; i < harmonicresonator::kNumResonators; ++i)
    {
        partialEnergy[(std::size_t) i] +=
            (snapshot.partialEnergy[i].load (std::memory_order_relaxed)
                - partialEnergy[(std::size_t) i]) * 0.25f;

        partialFreq[(std::size_t) i] = snapshot.partialFreq[i].load (std::memory_order_relaxed);
    }

    repaint (partialsArea);
    repaint (modArea);
    repaint (outputArea);
    repaint (resonatorArea);
    repaint (interactionArea);
}

float HarmonicResonatorModeAudioProcessorEditor::paramValue (const juce::String& id) const
{
    if (auto* value = processor.apvts.getRawParameterValue (id))
        return value->load();

    return 0.0f;
}

juce::String HarmonicResonatorModeAudioProcessorEditor::valueText (const Knob& knob) const
{
    if (knob.slider == nullptr)
        return {};

    return knob.slider->getTextFromValue (knob.slider->getValue()).toUpperCase();
}
