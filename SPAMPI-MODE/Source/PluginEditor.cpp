#include "PluginEditor.h"

#include <SpampiAssets.h>

#include <cmath>

namespace
{
constexpr int kWidth = 940;
constexpr int kHeight = 640;
constexpr int kMargin = 18;

juce::Typeface::Ptr loadFace (const void* data, std::size_t size)
{
    return juce::Typeface::createSystemTypefaceFor (data, size);
}

const juce::Typeface::Ptr& bagelFace()
{
    static const juce::Typeface::Ptr face = loadFace (SpampiAssets::BagelFatOneRegular_ttf,
                                                      (std::size_t) SpampiAssets::BagelFatOneRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& rowdiesBoldFace()
{
    static const juce::Typeface::Ptr face = loadFace (SpampiAssets::RowdiesBold_ttf,
                                                      (std::size_t) SpampiAssets::RowdiesBold_ttfSize);
    return face;
}

const juce::Typeface::Ptr& rowdiesFace()
{
    static const juce::Typeface::Ptr face = loadFace (SpampiAssets::RowdiesRegular_ttf,
                                                      (std::size_t) SpampiAssets::RowdiesRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& titanFace()
{
    static const juce::Typeface::Ptr face = loadFace (SpampiAssets::TitanOneRegular_ttf,
                                                      (std::size_t) SpampiAssets::TitanOneRegular_ttfSize);
    return face;
}

void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                  float tracking, juce::Justification justification)
{
    g.setFont (g.getCurrentFont().withExtraKerningFactor (tracking));
    g.drawText (text, area, justification, false);
}

// The card every section sits on: a soft drop shadow, a warm dark body, a
// rainbow edge and a gloss across the top. The gummy look is all in the corner
// radius: nothing is sharp except the small print.
void paintCard (juce::Graphics& g, juce::Rectangle<int> area, float alpha = 1.0f)
{
    const auto r = area.toFloat();

    juce::DropShadow (juce::Colour (0x77000000), 20, juce::Point<int> (0, 9))
        .drawForRectangle (g, area);

    juce::ColourGradient fill (SpampiLookAndFeel::cardTop(), r.getX(), r.getY(),
                               SpampiLookAndFeel::card(), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 20.0f);

    g.setGradientFill (SpampiLookAndFeel::rainbow (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 19.0f, 2.0f);

    auto gloss = r.reduced (4.0f).withHeight (r.getHeight() * 0.30f);
    juce::ColourGradient shine (juce::Colours::white.withAlpha (0.09f * alpha),
                                gloss.getX(), gloss.getY(),
                                juce::Colours::white.withAlpha (0.0f),
                                gloss.getX(), gloss.getBottom(), false);
    g.setGradientFill (shine);
    g.fillRoundedRectangle (gloss, 16.0f);
}

juce::Path roundedPath (juce::Rectangle<float> r, float radius)
{
    juce::Path path;
    path.addRoundedRectangle (r, radius);
    return path;
}
} // namespace

//==============================================================================
juce::Colour SpampiLookAndFeel::base()    { return juce::Colour (0xff0d0812); }
juce::Colour SpampiLookAndFeel::card()    { return juce::Colour (0xff201530); }
juce::Colour SpampiLookAndFeel::cardTop() { return juce::Colour (0xff2e2044); }
juce::Colour SpampiLookAndFeel::line()    { return juce::Colour (0xff413058); }
juce::Colour SpampiLookAndFeel::ink()     { return juce::Colour (0xfffff5fb); }
juce::Colour SpampiLookAndFeel::dim()     { return juce::Colour (0xffb39ec9); }

juce::Colour SpampiLookAndFeel::accent (int index)
{
    static const juce::Colour colours[] = {
        juce::Colour (0xffff3d7f),   // bubble gum
        juce::Colour (0xffff8a3d),   // tangerine
        juce::Colour (0xffffd93d),   // lemon
        juce::Colour (0xff5ce65c),   // lime
        juce::Colour (0xff3dd6ff),   // pool
        juce::Colour (0xff7a5cff)    // grape
    };

    return colours[((index % 6) + 6) % 6];
}

juce::ColourGradient SpampiLookAndFeel::rainbow (juce::Rectangle<float> area, bool vertical)
{
    juce::ColourGradient gradient;

    if (vertical)
        gradient = juce::ColourGradient (accent (0), area.getCentreX(), area.getY(),
                                         accent (5), area.getCentreX(), area.getBottom(), false);
    else
        gradient = juce::ColourGradient (accent (0), area.getX(), area.getCentreY(),
                                         accent (5), area.getRight(), area.getCentreY(), false);

    gradient.addColour (0.20, accent (1));
    gradient.addColour (0.40, accent (2));
    gradient.addColour (0.60, accent (3));
    gradient.addColour (0.80, accent (4));

    return gradient;
}

SpampiLookAndFeel::SpampiLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1228));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, accent (2).withAlpha (0.9f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent (0));
    setColour (juce::PopupMenu::highlightedTextColourId, base());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1b1228));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff120b1c));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, accent (0).withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, accent (4));

    setColour (juce::Label::textColourId, ink());
    setColour (juce::ComboBox::textColourId, ink());
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff180f24));
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::arrowColourId, accent (2));
}

juce::Font SpampiLookAndFeel::gummyFont (float height) const
{
    return juce::Font (juce::FontOptions (bagelFace()).withHeight (height));
}

juce::Font SpampiLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (rowdiesBoldFace()).withHeight (height));
}

juce::Font SpampiLookAndFeel::bodyFont (float height) const
{
    return juce::Font (juce::FontOptions (rowdiesFace()).withHeight (height));
}

juce::Font SpampiLookAndFeel::numberFont (float height) const
{
    return juce::Font (juce::FontOptions (titanFace()).withHeight (height));
}

void SpampiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // The candy sits on a little shadow, so it looks pressed into the card.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (knob.translated (0.0f, 5.0f).reduced (3.0f));

    // The track, and the part of it the knob has travelled.
    const auto radius = knob.getWidth() * 0.5f - 3.0f;
    const auto centre = knob.getCentre();

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (line().brighter (0.15f).withAlpha (0.75f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path active;
    active.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setGradientFill (rainbow (knob));
    g.strokePath (active, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // The body: a gloss on the top half, a soft sheen on the bottom edge.
    const auto body = knob.reduced (10.0f);

    juce::ColourGradient bodyGradient (cardTop().brighter (0.10f), body.getX(), body.getY(),
                                       card().darker (0.45f), body.getX(), body.getBottom(), false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (body);

    g.setColour (colour.withAlpha (0.55f));
    g.drawEllipse (body.reduced (0.5f), 1.6f);

    auto gloss = body.reduced (body.getWidth() * 0.13f, 0.0f)
                     .withHeight (body.getHeight() * 0.46f)
                     .translated (0.0f, body.getHeight() * 0.07f);
    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.fillEllipse (gloss);

    // The pointer, with a bright tip so the position reads at a glance.
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

juce::Label* SpampiLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (bodyFont (10.0f));
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void SpampiLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour&, bool highlighted,
                                              bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);

    juce::ColourGradient fill (highlighted ? cardTop().brighter (0.28f) : cardTop(),
                               r.getX(), r.getY(),
                               card().darker (down ? 0.25f : 0.05f),
                               r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 13.0f);

    g.setGradientFill (rainbow (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 12.0f, down ? 2.5f : 1.6f);
}

void SpampiLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                        bool, bool)
{
    g.setFont (labelFont (11.0f));
    g.setColour (button.isEnabled() ? ink() : dim().withAlpha (0.5f));
    drawTracked (g, button.getButtonText(), button.getLocalBounds(),
                 0.14f, juce::Justification::centred);
}

void SpampiLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);

    g.setColour (juce::Colour (0xff160e20));
    g.fillRoundedRectangle (r, 12.0f);

    g.setGradientFill (rainbow (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 11.0f, 1.4f);

    // A small chevron, drawn by hand so it matches the round type.
    const auto x = (float) width - 15.0f;
    const auto y = (float) height * 0.5f;

    juce::Path chevron;
    chevron.startNewSubPath (x - 4.0f, y - 2.0f);
    chevron.lineTo (x, y + 2.5f);
    chevron.lineTo (x + 4.0f, y - 2.0f);

    g.setColour (box.isEnabled() ? accent (2) : dim());
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

juce::Font SpampiLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (12.0f).withExtraKerningFactor (0.08f);
}

juce::Font SpampiLookAndFeel::getPopupMenuFont()
{
    return bodyFont (12.0f);
}

void SpampiLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff1b1228));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void SpampiLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::backgroundColourId));
    g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 6.0f);

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                      label.getJustificationType(), 1);
}

//==============================================================================
SpampiModeAudioProcessorEditor::SpampiModeAudioProcessorEditor (SpampiModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    headerArea = { kMargin, 14, kWidth - 2 * kMargin, 74 };
    presetBarArea = { kMargin, 96, kWidth - 2 * kMargin, 34 };
    recordingArea = { kMargin, 140, 300, 232 };
    pitchArea = { kMargin, 380, 300, 224 };
    gridArea = { 330, 140, kWidth - 330 - kMargin, 464 };

    addKnob (ids::root, "ROOT", 0, true, false);
    addKnob (ids::follow, "FOLLOW", 1, false, false);
    addKnob (ids::interval, "INTERVAL", 2, false, false);
    addKnob (ids::trigger, "TRIGGER", 3, false, true);
    addKnob (ids::spread, "SPREAD", 4, false, false);
    addKnob (ids::glide, "GLIDE", 5, false, false);
    addKnob (ids::drift, "DRIFT", 0, false, false);
    addKnob (ids::machine, "MACHINE", 1, false, false);
    addKnob (ids::sens, "SENS", 2, false, false);
    addKnob (ids::tone, "TONE", 3, false, false);
    addKnob (ids::mix, "MIX", 4, false, false);
    addKnob (ids::level, "LEVEL", 5, false, false);

    // The recording, and the preset.
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    sampleBox.setJustificationType (juce::Justification::centredLeft);
    sampleBox.onChange = [this] { chooseSample (sampleBox.getSelectedId()); };
    addAndMakeVisible (sampleBox);

    saveButton.onClick = [this] { promptSavePreset(); };
    deleteButton.onClick = [this] { promptDeletePreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);

    loadButton.onClick = [this] { loadSampleFile(); };
    addAndMakeVisible (loadButton);

    refreshSampleList();
    refreshPresetList();

    const auto matched = processor.matchFactoryPreset();
    setPresetName (matched.isNotEmpty() ? matched : "CUSTOM");

    for (const auto* id : { ids::sampleIndex, ids::root, ids::follow, ids::interval,
                            ids::spread, ids::glide, ids::drift, ids::machine,
                            ids::trigger, ids::sens, ids::tone, ids::mix, ids::level })
        processor.apvts.addParameterListener (id, this);

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

SpampiModeAudioProcessorEditor::~SpampiModeAudioProcessorEditor()
{
    stopTimer();

    for (const auto* id : { ids::sampleIndex, ids::root, ids::follow, ids::interval,
                            ids::spread, ids::glide, ids::drift, ids::machine,
                            ids::trigger, ids::sens, ids::tone, ids::mix, ids::level })
        processor.apvts.removeParameterListener (id, this);

    if (activeDialog != nullptr)
        activeDialog->exitModalState (0);

    setLookAndFeel (nullptr);
}

void SpampiModeAudioProcessorEditor::addKnob (const char* paramId, const char* label,
                                              int accent, bool isNote, bool isStepped)
{
    auto knob = std::make_unique<Knob>();
    knob->label = label;
    knob->paramId = paramId;
    knob->accent = accent;
    knob->isNote = isNote;
    knob->isStepped = isStepped;

    knob->slider = std::make_unique<juce::Slider>();
    knob->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob->slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob->slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId,
                             SpampiLookAndFeel::accent (accent));
    knob->slider->setMouseDragSensitivity (180);

    knob->slider->onValueChange = [this, index = knobs.size()]
    {
        if (index < knobs.size())
            repaint (knobs[index]->cell);
    };

    addAndMakeVisible (*knob->slider);

    knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, *knob->slider);

    knobs.push_back (std::move (knob));
}

//==============================================================================
void SpampiModeAudioProcessorEditor::resized()
{
    presetBox.setBounds (presetBarArea.getX(), presetBarArea.getY() + 3, 262, 28);
    saveButton.setBounds (presetBarArea.getX() + 274, presetBarArea.getY() + 4, 64, 26);
    deleteButton.setBounds (presetBarArea.getX() + 346, presetBarArea.getY() + 4, 54, 26);

    loadButton.setBounds (presetBarArea.getRight() - 148, presetBarArea.getY() + 4, 148, 26);

    sampleBox.setBounds (recordingArea.getX() + 14, recordingArea.getBottom() - 40,
                         recordingArea.getWidth() - 28, 26);

    const auto columns = 4;
    const auto rows = (int) knobs.size() / columns;
    const auto cellWidth = gridArea.getWidth() / columns;
    const auto cellHeight = gridArea.getHeight() / juce::jmax (1, rows);

    for (int i = 0; i < (int) knobs.size(); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;

        const juce::Rectangle<int> cell (gridArea.getX() + column * cellWidth,
                                         gridArea.getY() + row * cellHeight,
                                         cellWidth, cellHeight);

        knobs[(std::size_t) i]->cell = cell;

        const auto size = juce::jmin (92, cellWidth - 34);
        knobs[(std::size_t) i]->slider->setBounds (cell.getCentreX() - size / 2,
                                                   cell.getY() + 8, size, size);
    }
}

//==============================================================================
void SpampiModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    paintBackground (g);
    paintHeader (g);
    paintPresetBar (g);
    paintRecording (g);
    paintPitch (g);
    paintKnobs (g);
    paintLegalNotice (g);

    g.setColour (SpampiLookAndFeel::line().withAlpha (0.9f));
    g.drawRect (getLocalBounds(), 1);
}

void SpampiModeAudioProcessorEditor::paintBackground (juce::Graphics& g)
{
    g.fillAll (SpampiLookAndFeel::base());

    // Two soft rainbow clouds, one at each top corner, so the black is never flat.
    {
        juce::ColourGradient glow (SpampiLookAndFeel::accent (0).withAlpha (0.20f),
                                   0.0f, 0.0f,
                                   SpampiLookAndFeel::accent (5).withAlpha (0.0f),
                                   kWidth * 0.75f, kHeight * 0.55f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    {
        juce::ColourGradient glow (SpampiLookAndFeel::accent (4).withAlpha (0.13f),
                                   (float) kWidth, 0.0f,
                                   SpampiLookAndFeel::accent (2).withAlpha (0.0f),
                                   kWidth * 0.35f, kHeight * 0.8f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    // A very faint dot grid: the toy is printed on paper, not on glass.
    g.setColour (SpampiLookAndFeel::ink().withAlpha (0.035f));

    for (int y = 0; y < kHeight; y += 16)
        for (int x = (y / 16) % 2 == 0 ? 8 : 0; x < kWidth; x += 16)
            g.fillRect (x, y, 1, 1);
}

void SpampiModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const juce::Rectangle<int> titleArea (headerArea.getX(), headerArea.getY() - 2,
                                          headerArea.getWidth() - 210, 58);

    // The shout: rainbow, with a soft shadow so it reads on the dark.
    auto titleFont = lookAndFeel.gummyFont (46.0f);

    g.setFont (titleFont);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawText ("SPAMPI-MODE", titleArea.translated (0, 3), juce::Justification::centredLeft, false);

    g.setGradientFill (SpampiLookAndFeel::rainbow (titleArea.toFloat()));
    g.drawText ("SPAMPI-MODE", titleArea, juce::Justification::centredLeft, false);

    // The small print under the shout.
    g.setFont (lookAndFeel.bodyFont (10.0f));
    g.setColour (SpampiLookAndFeel::ink().withAlpha (0.62f));
    drawTracked (g, "PITCH-FOLLOWING RECORD PLAYER - NO MIDI, JUST THE INPUT",
                 { headerArea.getX() + 4, headerArea.getY() + 54, 520, 16 },
                 0.16f, juce::Justification::centredLeft);

    // The input meter, on the right.
    const juce::Rectangle<int> meter (headerArea.getRight() - 186, headerArea.getY() + 26, 186, 26);

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (SpampiLookAndFeel::dim());
    drawTracked (g, "INPUT", { meter.getX(), meter.getY() - 2, 50, 14 }, 0.20f,
                 juce::Justification::centredLeft);

    auto bar = juce::Rectangle<float> ((float) meter.getX() + 46.0f, (float) meter.getY() + 2.0f,
                                       (float) meter.getWidth() - 46.0f, 10.0f);

    g.setColour (juce::Colour (0xff150d20));
    g.fillRoundedRectangle (bar, 5.0f);
    g.setColour (SpampiLookAndFeel::line());
    g.drawRoundedRectangle (bar, 5.0f, 1.0f);

    if (inputLevel > 0.002f)
    {
        auto lit = bar.reduced (2.0f);
        lit = lit.withWidth (juce::jmax (2.0f, lit.getWidth() * juce::jlimit (0.0f, 1.0f, inputLevel)));
        g.setGradientFill (SpampiLookAndFeel::rainbow (bar));
        g.fillRoundedRectangle (lit, 4.0f);
    }

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (confidence > 0.6f ? SpampiLookAndFeel::accent (3) : SpampiLookAndFeel::dim());
    drawTracked (g, confidence > 0.6f ? "TRACKING" : "WAITING",
                 { meter.getX() + 46, meter.getY() + 14, 120, 12 }, 0.18f,
                 juce::Justification::centredLeft);
}

void SpampiModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (SpampiLookAndFeel::dim());
    drawTracked (g, "VOICE", { presetBarArea.getX(), presetBarArea.getY() + 10, 44, 14 },
                 0.20f, juce::Justification::centredLeft);

    // The preset name is printed under the box, so the shout stays readable.
    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (edited ? SpampiLookAndFeel::accent (1) : SpampiLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, edited ? "EDITED" : "FACTORY", { presetBarArea.getX() + 274,
                                                     presetBarArea.getY() + 31, 120, 10 },
                 0.18f, juce::Justification::centredLeft);
}

void SpampiModeAudioProcessorEditor::paintRecording (juce::Graphics& g)
{
    paintCard (g, recordingArea);

    const auto left = recordingArea.getX() + 16;
    const auto width = recordingArea.getWidth() - 32;

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (SpampiLookAndFeel::dim());
    drawTracked (g, "THE RECORDING", { left, recordingArea.getY() + 12, width, 14 },
                 0.22f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (23.0f));
    g.setColour (SpampiLookAndFeel::ink());
    g.drawFittedText (currentSampleName.isNotEmpty() ? currentSampleName : "NO SAMPLE",
                      { left, recordingArea.getY() + 28, width - 46, 30 },
                      juce::Justification::centredLeft, 1);

    // A little chip on the right when the recording is the player's own.
    if (currentSampleIndex >= SpampiModeAudioProcessor::kNumPresets)
    {
        const juce::Rectangle<int> chip (recordingArea.getRight() - 56, recordingArea.getY() + 30, 42, 16);
        g.setColour (SpampiLookAndFeel::accent (3).withAlpha (0.20f));
        g.fillRoundedRectangle (chip.toFloat(), 8.0f);
        g.setColour (SpampiLookAndFeel::accent (3));
        g.setFont (lookAndFeel.bodyFont (7.5f));
        drawTracked (g, "YOURS", chip, 0.16f, juce::Justification::centred);
    }

    g.setFont (lookAndFeel.bodyFont (9.0f));
    g.setColour (SpampiLookAndFeel::dim());
    drawTracked (g, currentRoot >= 0
                        ? "MEASURED AT " + noteName (currentRoot)
                        : juce::String ("NO PITCH FOUND"),
                 { left, recordingArea.getY() + 60, width, 14 }, 0.16f,
                 juce::Justification::centredLeft);

    // The waveform of the recording that is loaded.
    const juce::Rectangle<int> wave (left, recordingArea.getY() + 78, width, 88);

    g.setColour (juce::Colour (0xff150d20));
    g.fillRoundedRectangle (wave.toFloat(), 12.0f);

    const auto centreY = (float) wave.getCentreY();
    g.setColour (SpampiLookAndFeel::line().withAlpha (0.8f));
    g.drawLine ((float) wave.getX() + 4.0f, centreY, (float) wave.getRight() - 4.0f, centreY, 1.0f);

    const auto bins = (int) waveform.size();
    const auto barWidth = (float) wave.getWidth() / (float) bins;

    juce::Path wavePath;

    for (int i = 0; i < bins; ++i)
    {
        const auto peak = juce::jlimit (0.0f, 1.0f, waveform[(std::size_t) i]);
        const auto height = juce::jmax (1.0f, peak * (float) wave.getHeight() * 0.46f);
        const auto x = (float) wave.getX() + (float) i * barWidth;

        wavePath.addRoundedRectangle (x, centreY - height, juce::jmax (1.4f, barWidth - 0.6f),
                                      height * 2.0f, 1.0f);
    }

    g.setGradientFill (SpampiLookAndFeel::rainbow (wave.toFloat()));
    g.fillPath (wavePath);
}

void SpampiModeAudioProcessorEditor::paintPitch (juce::Graphics& g)
{
    paintCard (g, pitchArea);

    const auto left = pitchArea.getX() + 16;
    const auto width = pitchArea.getWidth() - 32;

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (SpampiLookAndFeel::dim());
    drawTracked (g, "WHAT IT HEARS", { left, pitchArea.getY() + 12, width, 14 },
                 0.22f, juce::Justification::centredLeft);

    // The note, as big as the card allows.
    const auto hasNote = noteValue >= 0.0f;
    const auto noteArea = juce::Rectangle<int> (left - 2, pitchArea.getY() + 26, 120, 58);

    g.setFont (lookAndFeel.gummyFont (48.0f));

    if (hasNote)
    {
        g.setGradientFill (SpampiLookAndFeel::rainbow (noteArea.toFloat()));
        g.drawText (noteName ((int) std::lround (noteValue)), noteArea,
                    juce::Justification::centredLeft, false);
    }
    else
    {
        g.setColour (SpampiLookAndFeel::dim().withAlpha (0.45f));
        g.drawText ("--", noteArea, juce::Justification::centredLeft, false);
    }

    // Hz and cents beside it.
    const juce::Rectangle<int> readout (left + 118, pitchArea.getY() + 30, width - 118, 34);

    g.setFont (lookAndFeel.numberFont (18.0f));
    g.setColour (SpampiLookAndFeel::ink());
    g.drawText (hasNote ? juce::String (frequency, 1) + " HZ" : juce::String ("NO PITCH"),
                readout, juce::Justification::centredLeft, false);

    if (hasNote)
    {
        const auto cents = (int) std::lround ((noteValue - std::floor (noteValue + 0.5f)) * 100.0f);
        const auto sign = cents > 0 ? "+" : (cents < 0 ? "-" : "");

        g.setFont (lookAndFeel.bodyFont (9.0f));
        g.setColour (SpampiLookAndFeel::dim());
        drawTracked (g, sign + juce::String (std::abs (cents)) + " CENTS   RATE x"
                            + juce::String (playbackRate, 2),
                     { readout.getX(), readout.getY() + 36, readout.getWidth(), 13 }, 0.12f,
                     juce::Justification::centredLeft);
    }

    // Confidence.
    const juce::Rectangle<int> barArea (left, pitchArea.getY() + 88, width, 8);
    auto bar = barArea.toFloat();

    g.setColour (juce::Colour (0xff150d20));
    g.fillRoundedRectangle (bar, 4.0f);
    g.setColour (SpampiLookAndFeel::line());
    g.drawRoundedRectangle (bar, 4.0f, 1.0f);

    if (confidence > 0.01f)
    {
        auto lit = bar.reduced (1.5f);
        lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth() * juce::jlimit (0.0f, 1.0f, confidence)));
        g.setGradientFill (SpampiLookAndFeel::rainbow (bar));
        g.fillRoundedRectangle (lit, 3.0f);
    }

    // The trace: the last few seconds of playing, as a rainbow ribbon.
    const juce::Rectangle<int> traceArea (left, pitchArea.getY() + 104, width, 96);

    g.setColour (juce::Colour (0xff150d20));
    g.fillRoundedRectangle (traceArea.toFloat(), 12.0f);

    g.setColour (SpampiLookAndFeel::line().withAlpha (0.6f));

    for (int line = 1; line < 4; ++line)
    {
        const auto y = (float) traceArea.getY() + (float) traceArea.getHeight() * (float) line / 4.0f;
        g.drawLine ((float) traceArea.getX() + 6.0f, y, (float) traceArea.getRight() - 6.0f, y, 1.0f);
    }

    const auto count = (int) pitchTrace.size();
    const auto stepX = (float) (traceArea.getWidth() - 12) / (float) (count - 1);

    // Fold the last seconds into a window a guitar can play, then let the window
    // follow the phrase: a melody spread over two notes still reads as a shape.
    std::array<float, 240> folded {};
    auto low = 1.0e9f, high = -1.0e9f;

    for (int i = 0; i < count; ++i)
    {
        auto midi = pitchTrace[(std::size_t) i];

        if (midi <= 0.0f)
        {
            folded[(std::size_t) i] = 0.0f;
            continue;
        }

        while (midi > 90.0f) midi -= 12.0f;
        while (midi < 38.0f) midi += 12.0f;

        folded[(std::size_t) i] = midi;
        low = juce::jmin (low, midi);
        high = juce::jmax (high, midi);
    }

    if (high - low < 9.0f)
    {
        const auto middle = 0.5f * (low + high);
        low = middle - 4.5f;
        high = middle + 4.5f;
    }

    const auto span = juce::jmax (1.0f, high - low);

    juce::Point<float> previous;
    auto havePrevious = false;
    auto colourIndex = 0;

    for (int i = 0; i < count; ++i)
    {
        const auto midi = folded[(std::size_t) i];

        if (midi <= 0.0f)
        {
            havePrevious = false;
            ++colourIndex;
            continue;
        }

        const auto normalised = juce::jlimit (0.0f, 1.0f, (midi - low) / span);
        const juce::Point<float> point ((float) traceArea.getX() + 6.0f + (float) i * stepX,
                                        (float) traceArea.getBottom() - 8.0f
                                            - normalised * (float) (traceArea.getHeight() - 16));

        if (havePrevious)
        {
            juce::Path segment;
            segment.startNewSubPath (previous);
            segment.lineTo (point);

            // A wide, dim pass under a narrow bright one: the glow.
            g.setColour (SpampiLookAndFeel::accent (colourIndex).withAlpha (0.18f));
            g.strokePath (segment, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour (SpampiLookAndFeel::accent (colourIndex));
            g.strokePath (segment, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        previous = point;
        havePrevious = true;
        ++colourIndex;
    }

    // The octave jump the machine is applying right now.
    if (std::abs (octaveJump) > 0.5f)
    {
        const auto text = (octaveJump > 0 ? "+" : "-") + juce::String ((int) std::abs (octaveJump)) + " OCT";
        const juce::Rectangle<int> chip (traceArea.getRight() - 66, traceArea.getY() + 7, 58, 16);

        g.setColour (SpampiLookAndFeel::accent (0).withAlpha (0.22f));
        g.fillRoundedRectangle (chip.toFloat(), 8.0f);
        g.setColour (SpampiLookAndFeel::accent (0));
        g.setFont (lookAndFeel.bodyFont (7.5f));
        drawTracked (g, text, chip, 0.14f, juce::Justification::centred);
    }
}

void SpampiModeAudioProcessorEditor::paintKnobs (juce::Graphics& g)
{
    for (const auto& knob : knobs)
    {
        const auto& cell = knob->cell;

        g.setFont (lookAndFeel.bodyFont (9.5f));
        g.setColour (SpampiLookAndFeel::dim());
        drawTracked (g, knob->label,
                     { cell.getX(), cell.getY() + 102, cell.getWidth(), 14 },
                     0.22f, juce::Justification::centred);

        g.setFont (lookAndFeel.labelFont (12.0f));
        g.setColour (SpampiLookAndFeel::accent (knob->accent));
        g.drawText (valueText (*knob),
                    juce::Rectangle<int> (cell.getX(), cell.getY() + 118, cell.getWidth(), 18),
                    juce::Justification::centred, false);
    }
}

void SpampiModeAudioProcessorEditor::paintLegalNotice (juce::Graphics& g)
{
    const auto area = juce::Rectangle<int> (kMargin, getHeight() - 15,
                                            getWidth() - 2 * kMargin, 12);

    g.setFont (lookAndFeel.bodyFont (7.0f));
    g.setColour (SpampiLookAndFeel::dim().withAlpha (0.55f));
    drawTracked (g, "(C) 2026 Philopium - AGPLv3 - no warranty - source: github.com/Philopium/NULLCAM-plugins",
                 area, 0.05f, juce::Justification::centredLeft);
}

//==============================================================================
juce::String SpampiModeAudioProcessorEditor::noteName (int midi)
{
    return juce::String (spampi::noteLabel (midi).c_str());
}

juce::String SpampiModeAudioProcessorEditor::valueText (const Knob& knob) const
{
    const auto value = (float) knob.slider->getValue();

    if (knob.isNote)
        return noteName ((int) std::lround (value));

    if (knob.isStepped)
        return spampi::triggerModeName ((int) std::lround (value));

    if (knob.paramId == ids::interval)
        return (value > 0.0f ? "+" : "") + juce::String ((int) std::lround (value)) + " ST";

    return juce::String (juce::roundToInt (value * 100.0f)) + "%";
}

//==============================================================================
void SpampiModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < SpampiModeAudioProcessor::kNumPresets; ++i)
        presetBox.addItem (SpampiModeAudioProcessor::getFactoryPresetName (
                               (SpampiModeAudioProcessor::Preset) i), i + 1);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MINE");

        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }

    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        for (int i = 0; i < SpampiModeAudioProcessor::kNumPresets; ++i)
            if (matched == SpampiModeAudioProcessor::getFactoryPresetName (
                               (SpampiModeAudioProcessor::Preset) i))
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

    deleteButton.setEnabled (userPresets.contains (presetName));
}

void SpampiModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;

    if (userPresets.contains (name))
        presetBox.setSelectedId (100 + userPresets.indexOf (name), juce::dontSendNotification);
    else
        for (int i = 0; i < SpampiModeAudioProcessor::kNumPresets; ++i)
            if (name == SpampiModeAudioProcessor::getFactoryPresetName (
                            (SpampiModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);

    deleteButton.setEnabled (userPresets.contains (name));
}

void SpampiModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId >= 1 && itemId <= SpampiModeAudioProcessor::kNumPresets)
    {
        processor.applyPreset ((SpampiModeAudioProcessor::Preset) (itemId - 1));
        setPresetName (SpampiModeAudioProcessor::getFactoryPresetName (
            (SpampiModeAudioProcessor::Preset) (itemId - 1)));
        edited = false;
        refreshSampleList();
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
            refreshSampleList();
            repaint();
        }
    }
}

void SpampiModeAudioProcessorEditor::refreshSampleList()
{
    sampleBox.clear (juce::dontSendNotification);

    const auto count = processor.getNumSamples();
    sampleBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < count; ++i)
    {
        if (i == SpampiModeAudioProcessor::kNumPresets)
            sampleBox.addSectionHeading ("YOUR RECORDINGS");

        sampleBox.addItem (processor.getSampleName (i), i + 1);
    }

    knownSampleCount = count;

    const auto current = processor.getCurrentSampleIndex();

    if (current >= 0 && current < count)
        sampleBox.setSelectedId (current + 1, juce::dontSendNotification);
}

void SpampiModeAudioProcessorEditor::chooseSample (int itemId)
{
    const auto index = itemId - 1;

    if (index < 0 || index >= processor.getNumSamples())
        return;

    if (auto* parameter = processor.apvts.getParameter (ids::sampleIndex))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) index));

    // A new recording means a new root: the machine tunes itself to it.
    processor.sampleChosenByUser();
    edited = processor.matchFactoryPreset().isEmpty();
    repaint();
}

void SpampiModeAudioProcessorEditor::loadSampleFile()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Load a recording into SPAMPI-MODE",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    juce::Component::SafePointer<SpampiModeAudioProcessorEditor> safe (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;

                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              if (safe->processor.importUserSample (file) >= 0)
                              {
                                  safe->refreshSampleList();
                                  safe->setPresetName ("CUSTOM");
                                  safe->edited = true;
                                  safe->repaint();
                              }
                          });
}

void SpampiModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || suggested == "CUSTOM" || userPresets.contains (suggested))
        suggested = "MY SPAMPI";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + SpampiModeAudioProcessor::getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<SpampiModeAudioProcessorEditor> safe (this);

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

void SpampiModeAudioProcessorEditor::promptDeletePreset()
{
    if (activeDialog != nullptr || ! userPresets.contains (presetName))
        return;

    auto* dialog = new juce::AlertWindow ("DELETE PRESET",
                                          "Delete \"" + presetName + "\"?\n\nThe file is removed from the preset folder.",
                                          juce::MessageBoxIconType::WarningIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addButton ("DELETE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<SpampiModeAudioProcessorEditor> safe (this);

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

//==============================================================================
void SpampiModeAudioProcessorEditor::parameterChanged (const juce::String&, float)
{
    edited = true;
}

void SpampiModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();

    // The factory match is a few comparisons: twice a second is plenty.
    if (++frame % 15 == 0)
        updateEditedState();
}

void SpampiModeAudioProcessorEditor::updateEditedState()
{
    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        if (presetName != matched)
            setPresetName (matched);

        edited = false;
        return;
    }

    if (userPresets.contains (presetName))
    {
        edited = false;
        return;
    }

    edited = true;
    repaint (presetBarArea);
}

void SpampiModeAudioProcessorEditor::pullLiveData()
{
    processor.copyPitchHistory (pitchTrace.data(), (int) pitchTrace.size());

    const auto& snapshot = processor.getSnapshot();
    noteValue = snapshot.note.load (std::memory_order_relaxed);
    frequency = snapshot.frequency.load (std::memory_order_relaxed);
    confidence = snapshot.confidence.load (std::memory_order_relaxed);
    inputLevel = snapshot.level.load (std::memory_order_relaxed);
    playbackRate = snapshot.playbackRate.load (std::memory_order_relaxed);
    octaveJump = snapshot.octaveJump.load (std::memory_order_relaxed);

    const auto index = processor.getCurrentSampleIndex();
    const auto count = processor.getNumSamples();

    if (count != knownSampleCount)
        refreshSampleList();
    else if (index >= 0 && index < count && sampleBox.getSelectedId() != index + 1)
        sampleBox.setSelectedId (index + 1, juce::dontSendNotification);

    if (index != currentSampleIndex)
    {
        currentSampleIndex = index;
        currentSampleName = processor.getSampleName (index);
        processor.copySampleWaveform (waveform.data(), (int) waveform.size());
        currentRoot = processor.getSampleMeasuredMidi (index);
        repaint (recordingArea);
    }

    repaint (pitchArea);
    repaint (headerArea);
}
