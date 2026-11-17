#include "PluginEditor.h"

#include <GiannisAssets.h>

#include <cmath>

namespace
{
constexpr int kWidth = 900;
constexpr int kHeight = 600;
constexpr int kMargin = 22;

juce::Typeface::Ptr loadFace (const void* data, std::size_t size)
{
    return juce::Typeface::createSystemTypefaceFor (data, size);
}

const juce::Typeface::Ptr& bagelFace()
{
    static const juce::Typeface::Ptr face = loadFace (GiannisAssets::BagelFatOneRegular_ttf,
                                                      (std::size_t) GiannisAssets::BagelFatOneRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& rowdiesBoldFace()
{
    static const juce::Typeface::Ptr face = loadFace (GiannisAssets::RowdiesBold_ttf,
                                                      (std::size_t) GiannisAssets::RowdiesBold_ttfSize);
    return face;
}

const juce::Typeface::Ptr& rowdiesFace()
{
    static const juce::Typeface::Ptr face = loadFace (GiannisAssets::RowdiesRegular_ttf,
                                                      (std::size_t) GiannisAssets::RowdiesRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& titanFace()
{
    static const juce::Typeface::Ptr face = loadFace (GiannisAssets::TitanOneRegular_ttf,
                                                      (std::size_t) GiannisAssets::TitanOneRegular_ttfSize);
    return face;
}

void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                  float tracking, juce::Justification justification)
{
    g.setFont (g.getCurrentFont().withExtraKerningFactor (tracking));
    g.drawText (text, area, justification, false);
}

void paintCard (juce::Graphics& g, juce::Rectangle<int> area, float alpha = 1.0f)
{
    const auto r = area.toFloat();

    juce::DropShadow (juce::Colour (0x77000000), 20, juce::Point<int> (0, 9))
        .drawForRectangle (g, area);

    juce::ColourGradient fill (GiannisLookAndFeel::cardTop(), r.getX(), r.getY(),
                               GiannisLookAndFeel::card(), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 20.0f);

    g.setGradientFill (GiannisLookAndFeel::inkGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 19.0f, 2.0f);

    auto gloss = r.reduced (4.0f).withHeight (r.getHeight() * 0.30f);
    juce::ColourGradient shine (juce::Colours::white.withAlpha (0.09f * alpha),
                                gloss.getX(), gloss.getY(),
                                juce::Colours::white.withAlpha (0.0f),
                                gloss.getX(), gloss.getBottom(), false);
    g.setGradientFill (shine);
    g.fillRoundedRectangle (gloss, 16.0f);
}

} // namespace

//==============================================================================
juce::Colour GiannisLookAndFeel::base()    { return juce::Colour (0xff0a0a0a); }
juce::Colour GiannisLookAndFeel::card()    { return juce::Colour (0xff1b1b1b); }
juce::Colour GiannisLookAndFeel::cardTop() { return juce::Colour (0xff282828); }
juce::Colour GiannisLookAndFeel::line()    { return juce::Colour (0xff3a3a3a); }
juce::Colour GiannisLookAndFeel::ink()     { return juce::Colour (0xfffffef6); }
juce::Colour GiannisLookAndFeel::dim()     { return juce::Colour (0xffb8b8a8); }

juce::Colour GiannisLookAndFeel::lime()  { return juce::Colour (0xffb5f46a); }
juce::Colour GiannisLookAndFeel::pool()  { return juce::Colour (0xff6ccff2); }
juce::Colour GiannisLookAndFeel::violet(){ return juce::Colour (0xffc88aff); }
juce::Colour GiannisLookAndFeel::rose()  { return juce::Colour (0xffff7fa8); }

juce::ColourGradient GiannisLookAndFeel::inkGradient (juce::Rectangle<float> area, bool vertical)
{
    juce::ColourGradient gradient;

    if (vertical)
        gradient = juce::ColourGradient (pool(), area.getCentreX(), area.getY(),
                                         lime(), area.getCentreX(), area.getBottom(), false);
    else
        gradient = juce::ColourGradient (rose(), area.getX(), area.getCentreY(),
                                         violet(), area.getRight(), area.getCentreY(), false);

    gradient.addColour (0.25, pool());
    gradient.addColour (0.50, lime());
    gradient.addColour (0.75, violet());
    return gradient;
}

GiannisLookAndFeel::GiannisLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff161616));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, lime().withAlpha (0.9f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, lime());
    setColour (juce::PopupMenu::highlightedTextColourId, base());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff161616));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f0f0f));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, lime().withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, pool());

    setColour (juce::Label::textColourId, ink());
    setColour (juce::ComboBox::textColourId, ink());
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff121212));
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::arrowColourId, lime());
}

juce::Font GiannisLookAndFeel::gummyFont (float height) const
{
    return juce::Font (juce::FontOptions (bagelFace()).withHeight (height));
}

juce::Font GiannisLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (rowdiesBoldFace()).withHeight (height));
}

juce::Font GiannisLookAndFeel::bodyFont (float height) const
{
    return juce::Font (juce::FontOptions (rowdiesFace()).withHeight (height));
}

juce::Font GiannisLookAndFeel::numberFont (float height) const
{
    return juce::Font (juce::FontOptions (titanFace()).withHeight (height));
}

void GiannisLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillEllipse (knob.translated (0.0f, 5.0f).reduced (3.0f));

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
    g.setGradientFill (inkGradient (knob));
    g.strokePath (active, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

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

juce::Label* GiannisLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (bodyFont (10.0f));
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void GiannisLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
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

    g.setGradientFill (inkGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 12.0f, down ? 2.5f : 1.6f);
}

void GiannisLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                         bool, bool)
{
    g.setFont (labelFont (11.0f));
    g.setColour (button.isEnabled() ? ink() : dim().withAlpha (0.5f));
    drawTracked (g, button.getButtonText(), button.getLocalBounds(),
                 0.14f, juce::Justification::centred);
}

void GiannisLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (1.0f);

    g.setColour (juce::Colour (0xff121212));
    g.fillRoundedRectangle (r, 12.0f);

    g.setGradientFill (inkGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 11.0f, 1.4f);

    const auto x = (float) width - 15.0f;
    const auto y = (float) height * 0.5f;

    juce::Path chevron;
    chevron.startNewSubPath (x - 4.0f, y - 2.0f);
    chevron.lineTo (x, y + 2.5f);
    chevron.lineTo (x + 4.0f, y - 2.0f);

    g.setColour (box.isEnabled() ? lime() : dim());
    g.strokePath (chevron, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

juce::Font GiannisLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return labelFont (12.0f).withExtraKerningFactor (0.08f);
}

juce::Font GiannisLookAndFeel::getPopupMenuFont()
{
    return bodyFont (12.0f);
}

void GiannisLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff161616));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void GiannisLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::backgroundColourId));
    g.fillRoundedRectangle (label.getLocalBounds().toFloat(), 6.0f);

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                      label.getJustificationType(), 1);
}

//==============================================================================
GiannisModeAudioProcessorEditor::GiannisModeAudioProcessorEditor (GiannisModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    headerArea = { kMargin, 14, kWidth - 2 * kMargin, 74 };
    presetBarArea = { kMargin, 96, kWidth - 2 * kMargin, 34 };
    noseCardArea = { kMargin, 140, 330, 332 };
    knobCardArea = { 362, 140, kWidth - 362 - kMargin, 332 };
    meterCardArea = { kMargin, 482, kWidth - 2 * kMargin, 106 };

    noseArea = { noseCardArea.getX() + 16, noseCardArea.getY() + 100, 180, 200 };

    // SUB, DOOM, OCT, FUZZ, TONE, LEVEL  in due righe da tre.
    addKnob (ids::sub,   "SUB",   0, 0, 0);
    addKnob (ids::doom,  "DOOM",  1, 1, 0);
    addKnob (ids::oct,   "OCT",   2, 2, 0);
    addKnob (ids::fuzz,  "FUZZ",  3, 0, 1);
    addKnob (ids::tone,  "TONE",  4, 1, 1);
    addKnob (ids::level, "LEVEL", 5, 2, 1);

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
    presetName = matched.isNotEmpty() ? matched : "GIANNIS";
    applyingPreset = true;
    setPresetName (presetName);
    applyingPreset = false;

    for (const auto* id : { ids::sub, ids::doom, ids::oct, ids::fuzz, ids::tone, ids::level })
        processor.apvts.addParameterListener (id, this);

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

GiannisModeAudioProcessorEditor::~GiannisModeAudioProcessorEditor()
{
    stopTimer();

    for (const auto* id : { ids::sub, ids::doom, ids::oct, ids::fuzz, ids::tone, ids::level })
        processor.apvts.removeParameterListener (id, this);

    if (activeDialog != nullptr)
        activeDialog->exitModalState (0);

    setLookAndFeel (nullptr);
}

void GiannisModeAudioProcessorEditor::addKnob (const char* paramId, const char* label,
                                               int accent, int column, int row)
{
    auto knob = std::make_unique<Knob>();
    knob->label = label;
    knob->paramId = paramId;
    knob->accent = accent;
    knob->column = column;
    knob->row = row;

    knob->slider = std::make_unique<juce::Slider>();
    knob->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob->slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob->slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    knob->slider->setColour (juce::Slider::rotarySliderFillColourId,
                             GiannisLookAndFeel::lime());
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

void GiannisModeAudioProcessorEditor::resized()
{
    presetBox.setBounds (presetBarArea.getX(), presetBarArea.getY() + 2, 262, 30);
    saveButton.setBounds (presetBarArea.getX() + 274, presetBarArea.getY() + 3, 64, 24);
    deleteButton.setBounds (presetBarArea.getX() + 346, presetBarArea.getY() + 3, 54, 24);

    const auto columns = 3;
    const auto rows = 2;
    const auto cellWidth = knobCardArea.getWidth() / columns;
    const auto cellHeight = knobCardArea.getHeight() / rows;

    for (int i = 0; i < (int) knobs.size(); ++i)
    {
        const auto column = knobs[(std::size_t) i]->column;
        const auto row = knobs[(std::size_t) i]->row;

        const juce::Rectangle<int> cell (knobCardArea.getX() + column * cellWidth,
                                         knobCardArea.getY() + row * cellHeight,
                                         cellWidth, cellHeight);

        knobs[(std::size_t) i]->cell = cell;

        const auto size = juce::jmin (92, cellWidth - 34);
        knobs[(std::size_t) i]->slider->setBounds (cell.getCentreX() - size / 2,
                                                   cell.getY() + 30, size, size);
    }

    updatePresetButtons();
}

// PART 3 MARKER

//==============================================================================
void GiannisModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (GiannisLookAndFeel::base());

    // Fondo scuro con due aloni colorati agli angoli superiori.
    {
        juce::ColourGradient glow (GiannisLookAndFeel::lime().withAlpha (0.15f),
                                   0.0f, 0.0f,
                                   GiannisLookAndFeel::rose().withAlpha (0.0f),
                                   (float) kWidth * 0.7f, (float) kHeight * 0.5f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    {
        juce::ColourGradient glow (GiannisLookAndFeel::pool().withAlpha (0.10f),
                                   (float) kWidth, 0.0f,
                                   GiannisLookAndFeel::violet().withAlpha (0.0f),
                                   (float) kWidth * 0.3f, (float) kHeight * 0.7f, true);
        g.setGradientFill (glow);
        g.fillAll();
    }

    // Reticolo molto tenue: il pedale è stampato su carta, non su vetro.
    g.setColour (GiannisLookAndFeel::ink().withAlpha (0.035f));

    for (int y = 0; y < kHeight; y += 16)
        for (int x = (y / 16) % 2 == 0 ? 8 : 0; x < kWidth; x += 16)
            g.fillRect (x, y, 1, 1);

    paintBaseplate (g);
    paintHeader (g);
    paintPresetBar (g);
    paintNoseCard (g);
    paintKnobCard (g);
    paintMeterCard (g);

    g.setColour (GiannisLookAndFeel::line().withAlpha (0.9f));
    g.drawRect (getLocalBounds(), 1);
}

void GiannisModeAudioProcessorEditor::paintBaseplate (juce::Graphics& g)
{
    const auto base = juce::Rectangle<int> (0, 0, kWidth, kHeight);

    g.setColour (juce::Colour (0xff121212));
    g.fillRoundedRectangle (base.toFloat(), 26.0f);

    g.setGradientFill (GiannisLookAndFeel::inkGradient (base.toFloat()));
    g.drawRoundedRectangle (base.reduced (1.0f).toFloat(), 25.0f, 2.0f);
}

void GiannisModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const juce::Rectangle<int> titleArea (headerArea.getX(), headerArea.getY() - 2,
                                          headerArea.getWidth() - 220, 58);

    // Lo strillo: GIANNIS-MODE, ombra morbida per leggersi sul buio.
    auto titleFont = lookAndFeel.gummyFont (46.0f);

    g.setFont (titleFont);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawText ("GIANNIS-MODE", titleArea.translated (0, 3), juce::Justification::centredLeft, false);

    g.setGradientFill (GiannisLookAndFeel::inkGradient (titleArea.toFloat()));
    g.drawText ("GIANNIS-MODE", titleArea, juce::Justification::centredLeft, false);

    // Il sottotitolo sotto lo strillo.
    g.setFont (lookAndFeel.bodyFont (10.0f));
    g.setColour (GiannisLookAndFeel::ink().withAlpha (0.62f));
    drawTracked (g, "OCTAVER + SUB-OCTAVER - NO MIDI, JUST THE INPUT",
                 { headerArea.getX() + 4, headerArea.getY() + 54, 560, 16 },
                 0.16f, juce::Justification::centredLeft);

    // Il naso e il livello in ingresso, a destra.
    const juce::Rectangle<int> meter (headerArea.getRight() - 186, headerArea.getY() + 26, 186, 26);

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "INPUT", { meter.getX(), meter.getY() - 2, 46, 14 },
                 0.20f, juce::Justification::centredLeft);

    auto bar = juce::Rectangle<float> ((float) meter.getX() + 46.0f, (float) meter.getY() + 2.0f,
                                       (float) meter.getWidth() - 46.0f, 10.0f);

    g.setColour (juce::Colour (0xff151515));
    g.fillRoundedRectangle (bar, 5.0f);
    g.setColour (GiannisLookAndFeel::line());
    g.drawRoundedRectangle (bar, 5.0f, 1.0f);

    if (inputAmount > 0.002f)
    {
        auto lit = bar.reduced (2.0f);
        lit = lit.withWidth (juce::jmax (2.0f, lit.getWidth() * juce::jlimit (0.0f, 1.0f, inputAmount)));
        g.setGradientFill (GiannisLookAndFeel::inkGradient (bar));
        g.fillRoundedRectangle (lit, 4.0f);
    }

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (trackedNote >= 0 ? GiannisLookAndFeel::lime() : GiannisLookAndFeel::dim());
    drawTracked (g, trackedNote >= 0 ? "TRACKING" : "WAITING",
                 { meter.getX() + 46, meter.getY() + 14, 120, 12 }, 0.18f,
                 juce::Justification::centredLeft);
}

void GiannisModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "PRESET", { presetBarArea.getX(), presetBarArea.getY() + 10, 48, 14 },
                 0.20f, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.labelFont (9.0f));
    g.setColour (edited ? GiannisLookAndFeel::rose() : GiannisLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, edited ? "EDITED" : "FACTORY",
                 { presetBarArea.getX() + 274, presetBarArea.getY() + 31, 120, 10 },
                 0.18f, juce::Justification::centredLeft);
}

// PART 4 MARKER

//==============================================================================
void GiannisModeAudioProcessorEditor::paintNoseCard (juce::Graphics& g)
{
    paintCard (g, noseCardArea);

    const auto left = noseCardArea.getX() + 16;
    const auto width = noseCardArea.getWidth() - 32;

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "THE NOSE", { left, noseCardArea.getY() + 12, width, 14 },
                 0.22f, juce::Justification::centredLeft);

    // Il nasone: simbolo del plugin, pompa col sub attivo.
    paintNose (g, noseArea, subAmount);

    // Letture: nota, Hz, sub.
    g.setFont (lookAndFeel.gummyFont (44.0f));
    g.setGradientFill (GiannisLookAndFeel::inkGradient (
        juce::Rectangle<float> ((float) left, (float) noseCardArea.getY() + 66,
                                (float) width - 120, 56), false));

    const auto noteText = trackedNote >= 0
        ? juce::String (giannis::noteName (trackedNote)) + juce::String ((trackedNote / 12) - 1)
        : juce::String ("--");

    g.drawText (noteText, juce::Rectangle<int> (left, noseCardArea.getY() + 66, 120, 54),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.numberFont (22.0f));
    g.setColour (GiannisLookAndFeel::ink());
    g.drawText (trackedHz > 0.0f ? juce::String (trackedHz, 0) + " HZ" : juce::String ("NO PITCH"),
                juce::Rectangle<int> (left + 118, noseCardArea.getY() + 74, width - 118, 30),
                juce::Justification::centredLeft, false);

    g.setFont (lookAndFeel.bodyFont (9.0f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "SUB " + juce::String (juce::roundToInt (subAmount * 100.0f)) + "%   "
                     + juce::String (juce::roundToInt (voiceAmount * 100.0f)) + "% VOICE",
                 { left, noseCardArea.getY() + 118, width, 14 },
                 0.14f, juce::Justification::centredLeft);

    // Barra del sub: quanto pesa la voce d'otto sul mixer.
    const juce::Rectangle<int> subBar (left, noseCardArea.getY() + 138, width, 8);
    auto subBarF = subBar.toFloat();

    g.setColour (juce::Colour (0xff151515));
    g.fillRoundedRectangle (subBarF, 4.0f);
    g.setColour (GiannisLookAndFeel::line());
    g.drawRoundedRectangle (subBarF, 4.0f, 1.0f);

    if (subAmount > 0.01f)
    {
        auto lit = subBarF.reduced (1.5f);
        lit = lit.withWidth (juce::jmax (3.0f, lit.getWidth() * juce::jlimit (0.0f, 1.0f, subAmount)));
        g.setColour (GiannisLookAndFeel::lime());
        g.fillRoundedRectangle (lit, 3.0f);
    }

    // Legenda delle voci: divide per 2, per 4, per 2.
    g.setFont (lookAndFeel.bodyFont (8.0f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "divide 2   divide 4   octave up",
                 { left, noseCardArea.getBottom() - 24, width, 12 },
                 0.10f, juce::Justification::centred);
}

void GiannisModeAudioProcessorEditor::paintNose (juce::Graphics& g, juce::Rectangle<int> area, float amount)
{
    // Il nasone: ponte nasale largo, punta arrotondata, due narici. Disegnato a
    // mano con path e gradienti, mai una foto, così resta nitido a ogni scala.
    const auto cx = (float) area.getCentreX();
    const auto top = (float) area.getY();
    const auto bottom = (float) area.getBottom();
    const auto width = (float) area.getWidth();

    const auto pump = 1.0f + amount * 0.12f;   // pompa quando il sub gira
    const auto w = width * pump;
    const auto h = (bottom - top) * pump;

    juce::Path nose;
    // Ponte nasale: due curve che partono alte e si stringono verso la punta.
    nose.startNewSubPath (cx - w * 0.34f, top + h * 0.08f);
    nose.cubicTo (cx - w * 0.18f, top + h * 0.30f, cx - w * 0.06f, top + h * 0.62f,
                  cx - w * 0.08f, top + h * 0.92f);
    // Punta: ariosa e arrotondata.
    nose.cubicTo (cx - w * 0.10f, top + h * 1.02f, cx + w * 0.10f, top + h * 1.02f,
                  cx + w * 0.08f, top + h * 0.92f);
    // Rientro verso il ponte dall'altro lato.
    nose.cubicTo (cx + w * 0.06f, top + h * 0.62f, cx + w * 0.18f, top + h * 0.30f,
                  cx + w * 0.34f, top + h * 0.08f);
    nose.closeSubPath();

    // Corpo del naso: pelle chiara, con un'ombra calda sotto la punta.
    juce::ColourGradient bodyGrad (juce::Colour (0xfff5e6d0), cx, top,
                                   juce::Colour (0xffd9b79a), cx, bottom, false);
    g.setGradientFill (bodyGrad);
    g.fillPath (nose);

    g.setColour (juce::Colour (0xffc8a488).withAlpha (0.8f));
    g.strokePath (nose, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Ombra sotto la punta: dà volume e sporge il naso verso chi guarda.
    juce::Path shadow;
    shadow.addEllipse (cx - w * 0.28f, top + h * 0.94f, w * 0.56f, h * 0.12f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (shadow);

    // Narici: due ellissi scavate, scure, leggermente sfumate.
    const auto nostrilW = w * 0.10f * pump;
    const auto nostrilH = h * 0.07f * pump;

    juce::Path leftNostril;
    leftNostril.addEllipse (cx - w * 0.13f, top + h * 0.72f, nostrilW, nostrilH);
    juce::ColourGradient nostrilGrad (juce::Colour (0xff8a5a42), cx - w * 0.13f, top + h * 0.72f,
                                      juce::Colour (0xff4a2a1f), cx - w * 0.13f, top + h * 0.80f, false);
    g.setGradientFill (nostrilGrad);
    g.fillPath (leftNostril);

    juce::Path rightNostril;
    rightNostril.addEllipse (cx + w * 0.13f - nostrilW, top + h * 0.72f, nostrilW, nostrilH);
    g.setGradientFill (nostrilGrad);
    g.fillPath (rightNostril);

    // Riflesso sul ponte: luce che scende dal sopracciglio.
    juce::Path shine;
    shine.addEllipse (cx - w * 0.16f, top + h * 0.14f, w * 0.14f, h * 0.16f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillPath (shine);

    // Contorno esterno sottile che separa il naso dalla carta.
    g.setColour (juce::Colour (0xffb89a80).withAlpha (0.7f));
    g.strokePath (nose, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

// PART 5 MARKER

void GiannisModeAudioProcessorEditor::paintKnobCard (juce::Graphics& g)
{
    paintCard (g, knobCardArea);

    const auto left = knobCardArea.getX() + 16;
    const auto width = knobCardArea.getWidth() - 32;

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "VOICE + DIRT", { left, knobCardArea.getY() + 12, width, 14 },
                 0.22f, juce::Justification::centredLeft);

    for (const auto& knob : knobs)
    {
        const auto& cell = knob->cell;

        g.setFont (lookAndFeel.bodyFont (9.5f));
        g.setColour (GiannisLookAndFeel::dim());
        drawTracked (g, knob->label,
                     { cell.getX(), cell.getY() + 112, cell.getWidth(), 14 },
                     0.22f, juce::Justification::centred);

        g.setFont (lookAndFeel.labelFont (12.0f));
        g.setColour (GiannisLookAndFeel::lime());
        g.drawText (valueText (*knob),
                    juce::Rectangle<int> (cell.getX(), cell.getY() + 128, cell.getWidth(), 18),
                    juce::Justification::centred, false);
    }
}

void GiannisModeAudioProcessorEditor::paintMeterCard (juce::Graphics& g)
{
    paintCard (g, meterCardArea);

    const auto left = meterCardArea.getX() + 16;
    const auto width = meterCardArea.getWidth() - 32;

    g.setFont (lookAndFeel.bodyFont (8.5f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "METER", { left, meterCardArea.getY() + 12, width, 14 },
                 0.22f, juce::Justification::centredLeft);

    // Tre barre: IN (ingresso), SUB (sub voce), VOCE (voce fuzzata).
    struct Meter { const char* label; float value; juce::Colour colour; };
    const Meter meters[3] = {
        { "IN",   inputAmount, GiannisLookAndFeel::pool() },
        { "SUB",  subAmount,   GiannisLookAndFeel::lime() },
        { "VOCE", voiceAmount, GiannisLookAndFeel::rose() }
    };

    const auto barH = 10.0f;
    const auto barY = meterCardArea.getY() + 34;
    const auto labelW = 44.0f;

    for (int i = 0; i < 3; ++i)
    {
        const auto y = barY + (float) i * 22.0f;

        g.setFont (lookAndFeel.bodyFont (8.5f));
        g.setColour (GiannisLookAndFeel::dim());
        drawTracked (g, meters[i].label,
                     { left, (int) y - 2, (int) labelW, 14 },
                     0.18f, juce::Justification::centredLeft);

        auto bar = juce::Rectangle<float> ((float) left + labelW, (float) y + 2.0f,
                                           (float) width - labelW, barH);

        g.setColour (juce::Colour (0xff151515));
        g.fillRoundedRectangle (bar, 5.0f);
        g.setColour (GiannisLookAndFeel::line());
        g.drawRoundedRectangle (bar, 5.0f, 1.0f);

        if (meters[i].value > 0.002f)
        {
            auto lit = bar.reduced (2.0f);
            lit = lit.withWidth (juce::jmax (2.0f, lit.getWidth()
                                             * juce::jlimit (0.0f, 1.0f, meters[i].value)));
            g.setColour (meters[i].colour);
            g.fillRoundedRectangle (lit, 4.0f);
        }
    }

    // Legenda delle tre voci generate.
    g.setFont (lookAndFeel.bodyFont (8.0f));
    g.setColour (GiannisLookAndFeel::dim());
    drawTracked (g, "divide 2   divide 4   octave up",
                 { left, meterCardArea.getBottom() - 22, width, 12 },
                 0.10f, juce::Justification::centred);
}

void GiannisModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");

    for (int i = 0; i < GiannisModeAudioProcessor::kNumPresets; ++i)
        presetBox.addItem (GiannisModeAudioProcessor::getFactoryPresetName (
                               (GiannisModeAudioProcessor::Preset) i), i + 1);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MINE");

        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }

    const auto matched = processor.matchFactoryPreset();

    if (matched.isNotEmpty())
    {
        for (int i = 0; i < GiannisModeAudioProcessor::kNumPresets; ++i)
            if (matched == GiannisModeAudioProcessor::getFactoryPresetName (
                               (GiannisModeAudioProcessor::Preset) i))
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

void GiannisModeAudioProcessorEditor::updatePresetButtons()
{
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void GiannisModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId >= 1 && itemId <= GiannisModeAudioProcessor::kNumPresets)
    {
        processor.applyPreset ((GiannisModeAudioProcessor::Preset) (itemId - 1));
        setPresetName (GiannisModeAudioProcessor::getFactoryPresetName (
            (GiannisModeAudioProcessor::Preset) (itemId - 1)));
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

// PART 6 MARKER

void GiannisModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;

    if (userPresets.contains (name))
        presetBox.setSelectedId (100 + userPresets.indexOf (name), juce::dontSendNotification);
    else
        for (int i = 0; i < GiannisModeAudioProcessor::kNumPresets; ++i)
            if (name == GiannisModeAudioProcessor::getFactoryPresetName (
                            (GiannisModeAudioProcessor::Preset) i))
                presetBox.setSelectedId (i + 1, juce::dontSendNotification);

    deleteButton.setEnabled (userPresets.contains (name));
}

void GiannisModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || suggested == "CUSTOM" || userPresets.contains (suggested))
        suggested = "MY GIANNIS";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + processor.getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<GiannisModeAudioProcessorEditor> safe (this);

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

void GiannisModeAudioProcessorEditor::promptDeletePreset()
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

    juce::Component::SafePointer<GiannisModeAudioProcessorEditor> safe (this);

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

void GiannisModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();

    // Il match con i preset di fabbrica: qualche confronto, due volte al secondo basta.
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
                repaint (presetBarArea);
            }
        }
    }
}

void GiannisModeAudioProcessorEditor::pullLiveData()
{
    const auto& snapshot = processor.getSnapshot();
    const auto targetHz = snapshot.frequency.load (std::memory_order_relaxed);
    const auto targetNote = snapshot.note.load (std::memory_order_relaxed);

    // Smooting: il naso pompa invece di sussultare.
    trackedHz += (targetHz - trackedHz) * 0.15f;
    if (targetNote >= 0)
        trackedNote = targetNote;

    subAmount += (snapshot.subLevel.load (std::memory_order_relaxed) - subAmount) * 0.15f;
    voiceAmount += (snapshot.voiceLevel.load (std::memory_order_relaxed) - voiceAmount) * 0.15f;
    inputAmount += (snapshot.inputLevel.load (std::memory_order_relaxed) - inputAmount) * 0.15f;

    repaint (noseCardArea);
    repaint (headerArea);
    repaint (meterCardArea);
}

void GiannisModeAudioProcessorEditor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset)
        edited = true;
}

juce::String GiannisModeAudioProcessorEditor::valueText (const Knob& knob) const
{
    const auto value = (float) knob.slider->getValue();
    return juce::String (juce::roundToInt (value * 100.0f)) + "%";
}

