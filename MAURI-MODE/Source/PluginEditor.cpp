#include "PluginEditor.h"

#include <MauriAssets.h>

namespace
{
constexpr int kWidth = 860;
constexpr int kHeight = 590;
constexpr int kMargin = 18;
constexpr int kHeaderH = 70;
constexpr int kPresetBarH = 30;
constexpr int kGap = 12;
constexpr int kMainH = 350;
constexpr int kRow1H = 210;
constexpr int kRow2H = 128;
constexpr int kGridH = 70;

constexpr int kMauriItem = 1;
constexpr int kOdd78Item = 2;
constexpr int kGhost54Item = 3;
constexpr int kShred68Item = 4;
constexpr int kUserItemFirst = 100;

constexpr std::array<const char*, 3> kClockNames {
    "HOST", "INPUT", "FREE"
};

juce::Typeface::Ptr loadFace (const void* data, std::size_t size)
{
    return juce::Typeface::createSystemTypefaceFor (data, size);
}

const juce::Typeface::Ptr& bungee()
{
    static const juce::Typeface::Ptr face = loadFace (MauriAssets::BungeeRegular_ttf,
                                                      (std::size_t) MauriAssets::BungeeRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& bungeeShade()
{
    static const juce::Typeface::Ptr face = loadFace (MauriAssets::BungeeShadeRegular_ttf,
                                                      (std::size_t) MauriAssets::BungeeShadeRegular_ttfSize);
    return face;
}

void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                  float tracking, juce::Justification justification)
{
    g.setFont (g.getCurrentFont().withExtraKerningFactor (tracking));
    g.drawText (text, area, justification, false);
}
} // namespace

//==============================================================================
juce::Colour MauriLookAndFeel::steel() { return juce::Colour (0xff0b0c0e); }
juce::Colour MauriLookAndFeel::card()  { return juce::Colour (0xff13151a); }
juce::Colour MauriLookAndFeel::line()  { return juce::Colour (0xff272b34); }
juce::Colour MauriLookAndFeel::ink()   { return juce::Colour (0xffe9ecf2); }
juce::Colour MauriLookAndFeel::dim()   { return juce::Colour (0xff878d9c); }
juce::Colour MauriLookAndFeel::blade() { return juce::Colour (0xffff5a1f); }
juce::Colour MauriLookAndFeel::spark() { return juce::Colour (0xfff5ead6); }

MauriLookAndFeel::MauriLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101116));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, blade().withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, blade());
    setColour (juce::PopupMenu::highlightedTextColourId, steel());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff101116));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff090a0d));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, blade().withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, blade());

    setColour (juce::Label::textColourId, ink());
}

juce::Font MauriLookAndFeel::displayFont (float height) const
{
    return juce::Font (juce::FontOptions (bungee()).withHeight (height));
}

juce::Font MauriLookAndFeel::shadeFont (float height) const
{
    return juce::Font (juce::FontOptions (bungeeShade()).withHeight (height));
}

juce::Font MauriLookAndFeel::monoFont (float height, bool bold) const
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

void MauriLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;

    if (radius <= 6.0f)
        return;

    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.isEnabled() ? blade() : blade().withAlpha (0.3f);

    // Outer calibration ticks
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = rotaryStartAngle + (float) i / 10.0f * (rotaryEndAngle - rotaryStartAngle);
        g.setColour (i % 5 == 0 ? line().brighter (0.55f) : line().brighter (0.18f));
        g.drawLine ({ centre.getPointOnCircumference (radius + 1.5f, a),
                      centre.getPointOnCircumference (radius + (i % 5 == 0 ? 4.5f : 3.0f), a) }, 1.0f);
    }

    const auto arcRadius = radius - 2.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff262a33));
    g.strokePath (track, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, juce::jmax (rotaryStartAngle, angle), true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Steel cylinder body
    const auto body = juce::jmax (6.0f, radius - 6.5f);

    juce::ColourGradient metal (juce::Colour (0xff494e5c), centre.x - body * 0.45f,
                                centre.y - body * 1.05f,
                                juce::Colour (0xff0c0d11), centre.x + body * 0.35f,
                                centre.y + body * 1.1f, false);
    metal.addColour (0.45, juce::Colour (0xff262932));
    metal.addColour (0.75, juce::Colour (0xff16181f));
    g.setGradientFill (metal);
    g.fillEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f);

    // Fine knurling around the skirt
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 32; ++i)
    {
        const auto a = juce::MathConstants<float>::twoPi * (float) i / 32.0f;
        g.drawLine ({ centre.getPointOnCircumference (body * 0.85f, a),
                      centre.getPointOnCircumference (body * 0.995f, a) }, 0.7f);
    }

    // Machined cap with a sliced bevel
    const auto cap = body * 0.72f;
    juce::ColourGradient capFill (juce::Colour (0xff313540), centre.x, centre.y - cap,
                                  juce::Colour (0xff14161c), centre.x, centre.y + cap, false);
    g.setGradientFill (capFill);
    g.fillEllipse (centre.x - cap, centre.y - cap, cap * 2.0f, cap * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawEllipse (centre.x - cap, centre.y - cap, cap * 2.0f, cap * 2.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f, 1.0f);

    // Blade cut pointer: a dark kerf with a hot-orange edge
    const auto tip = centre.getPointOnCircumference (body * 0.94f, angle);
    const auto tail = centre.getPointOnCircumference (body * 0.18f, angle);
    g.setColour (juce::Colour (0xff07080a));
    g.drawLine ({ tail, tip }, 3.4f);
    g.setColour (accent);
    g.drawLine ({ tail, tip }, 1.8f);
    g.fillEllipse (tip.x - 1.8f, tip.y - 1.8f, 3.6f, 3.6f);
}

juce::Label* MauriLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);

    label->setFont (monoFont (10.5f, true));
    label->setJustificationType (juce::Justification::centred);
    label->setColour (juce::Label::textColourId, ink());
    label->setColour (juce::Label::textWhenEditingColourId, blade());
    label->setColour (juce::Label::backgroundColourId, juce::Colour (0xff08090c));
    label->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff15171d));
    label->setColour (juce::Label::outlineColourId, line());
    label->setColour (juce::Label::outlineWhenEditingColourId, blade().withAlpha (0.8f));

    return label;
}

void MauriLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto fill = down ? blade()
                           : (highlighted ? juce::Colour (0xff1c1f27) : juce::Colour (0xff13151b));

    g.setColour (button.isEnabled() ? fill : fill.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (down ? blade()
                      : (highlighted ? blade().withAlpha (0.85f)
                                     : line().withAlpha (button.isEnabled() ? 1.0f : 0.5f)));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);
}

void MauriLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool highlighted, bool down)
{
    g.setFont (monoFont (9.5f, true).withExtraKerningFactor (0.18f));
    g.setColour (! button.isEnabled() ? dim().withAlpha (0.5f)
                                      : (down ? steel() : (highlighted ? blade() : ink())));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void MauriLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);

    g.setColour (juce::Colour (0xff0e1014));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (box.isMouseOver (true) ? blade().withAlpha (0.75f) : line());
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    juce::Path arrow;
    const auto cx = (float) width - 13.0f;
    const auto cy = (float) height * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);

    g.setColour (blade());
    g.fillPath (arrow);
}

juce::Font MauriLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return monoFont (10.0f, true).withExtraKerningFactor (0.16f);
}

juce::Font MauriLookAndFeel::getPopupMenuFont()
{
    return monoFont (11.0f);
}

void MauriLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff101116));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void MauriLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) == nullptr)
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }

    const auto r = label.getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (juce::Colour (0xff07080b));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (label.isBeingEdited() ? blade() : line().brighter (0.22f));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    if (label.isBeingEdited())
        return;

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (3, 0),
                      label.getJustificationType(), 1, 0.9f);
}

//==============================================================================
MauriModeAudioProcessorEditor::MauriModeAudioProcessorEditor (MauriModeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    // 01: Capture scope & dynamics meter (no knobs)
    Card scopeCard;
    scopeCard.number = "01";
    scopeCard.name = "CAPTURE";
    scopeCard.caption = "2.7s ring / live blade";
    scopeCard.isScope = true;
    cards.push_back (std::move (scopeCard));

    // 02: Clock, time signature, subdivision and pattern style (2x2)
    addCard ("02", "RHYTHM", "clock & signature", 2,
             { { ids::clock, "CLOCK", Unit::clock },
               { ids::timeSig, "TIME SIG", Unit::timeSig },
               { ids::rate, "RATE", Unit::rate },
               { ids::style, "STYLE", Unit::style } });

    // 03: Blade slicing (4 across)
    addCard ("03", "BLADE", "slice & chop", 4,
             { { ids::slice, "SLICE", Unit::percent },
               { ids::repeat, "RATCHET", Unit::percent },
               { ids::reverse, "REVERSE", Unit::percent },
               { ids::scatter, "SCATTER", Unit::percent } });

    // 04: Dynamics -> generated pattern (2x2)
    addCard ("04", "DYNAMICS", "playing writes the pattern", 2,
             { { ids::dynPat, "DYN PAT", Unit::percent },
               { ids::dynSlice, "DYN SLICE", Unit::percent },
               { ids::dynRate, "DYN RATCH", Unit::percent },
               { ids::sens, "SENS", Unit::percent } });

    // 05: Out (3 across)
    addCard ("05", "OUTPUT", "filter & blend", 3,
             { { ids::tone, "TONE", Unit::percent },
               { ids::mix, "MIX", Unit::percent },
               { ids::output, "LEVEL", Unit::percent } });

    presetBox.setTextWhenNothingSelected ("LOAD PRESET");
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    saveButton.onClick = [this] { promptSavePreset(); };
    deleteButton.onClick = [this] { promptDeletePreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);

    makeSteelTexture();
    refreshPresetList();

    const auto current = processor.matchFactoryPreset();
    setPresetName (current.isNotEmpty() ? current : "CUSTOM");

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

MauriModeAudioProcessorEditor::~MauriModeAudioProcessorEditor()
{
    stopTimer();

    for (auto& knob : knobs)
        processor.apvts.removeParameterListener (knob->paramId, this);

    if (activeDialog != nullptr)
    {
        activeDialog->setLookAndFeel (nullptr);
        activeDialog->exitModalState (0);
    }

    setLookAndFeel (nullptr);
}

void MauriModeAudioProcessorEditor::addCard (const char* number, const char* name,
                                            const char* caption, int columns,
                                            std::initializer_list<KnobSpec> specs)
{
    Card card;
    card.number = number;
    card.name = name;
    card.caption = caption;
    card.columns = columns;

    for (const auto& spec : specs)
    {
        auto knob = std::make_unique<Knob>();
        knob->paramId = spec.id;
        knob->label = spec.label;
        knob->unit = spec.unit;

        knob->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                       juce::Slider::TextBoxBelow);
        knob->slider->setMouseDragSensitivity (420);
        knob->slider->setScrollWheelEnabled (true);
        addAndMakeVisible (*knob->slider);

        knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, spec.id, *knob->slider);

        const auto unit = knob->unit;
        knob->slider->textFromValueFunction = [this, unit] (double value)
        { return valueText (value, unit); };
        knob->slider->valueFromTextFunction = [this, unit] (const juce::String& text)
        { return valueFromText (text, unit); };
        knob->slider->updateText();

        processor.apvts.addParameterListener (spec.id, this);

        card.knobIndices.push_back ((int) knobs.size());
        knobs.push_back (std::move (knob));
    }

    cards.push_back (std::move (card));
}

juce::String MauriModeAudioProcessorEditor::valueText (double value, Unit unit) const
{
    switch (unit)
    {
        case Unit::percent:
            return juce::String (juce::roundToInt (value * 100.0)) + "%";

        case Unit::clock:
        {
            const auto i = juce::jlimit (0, (int) kClockNames.size() - 1, juce::roundToInt (value));
            return kClockNames[(std::size_t) i];
        }

        case Unit::timeSig:
        {
            const auto i = juce::jlimit (0, mauri::kTimeSigs - 1, juce::roundToInt (value));
            return juce::String (mauri::kTimeSigsInfo[(std::size_t) i].name);
        }

        case Unit::rate:
        {
            const auto i = juce::jlimit (0, (int) mauri::kDivisionNames.size() - 1, juce::roundToInt (value));
            return mauri::kDivisionNames[(std::size_t) i];
        }

        case Unit::style:
        {
            const auto i = juce::jlimit (0, mauri::kStyles - 1, juce::roundToInt (value));
            return juce::String (mauri::kStyleNames[(std::size_t) i]);
        }
    }

    return {};
}

double MauriModeAudioProcessorEditor::valueFromText (const juce::String& text, Unit unit) const
{
    const auto trimmed = text.trim().toUpperCase();

    switch (unit)
    {
        case Unit::percent:
            return trimmed.retainCharacters ("0123456789.-").getDoubleValue() / 100.0;

        case Unit::clock:
            for (std::size_t i = 0; i < kClockNames.size(); ++i)
                if (trimmed.contains (kClockNames[i]))
                    return (double) i;
            return trimmed.retainCharacters ("0123456789").getDoubleValue();

        case Unit::timeSig:
            for (std::size_t i = 0; i < mauri::kTimeSigsInfo.size(); ++i)
                if (trimmed.contains (mauri::kTimeSigsInfo[i].name))
                    return (double) i;
            return trimmed.retainCharacters ("0123456789").getDoubleValue();

        case Unit::rate:
            for (std::size_t i = 0; i < mauri::kDivisionNames.size(); ++i)
                if (trimmed == mauri::kDivisionNames[i])
                    return (double) i;
            return trimmed.retainCharacters ("0123456789").getDoubleValue();

        case Unit::style:
            for (std::size_t i = 0; i < mauri::kStyleNames.size(); ++i)
                if (trimmed.contains (mauri::kStyleNames[i]))
                    return (double) i;
            return trimmed.retainCharacters ("0123456789").getDoubleValue();
    }

    return 0.0;
}

void MauriModeAudioProcessorEditor::makeSteelTexture()
{
    constexpr int size = 256;
    steelTexture = juce::Image (juce::Image::ARGB, size, size, true);

    juce::Image::BitmapData data (steelTexture, juce::Image::BitmapData::writeOnly);
    juce::Random rng (0x4d415552);

    // Subtle brushed steel grain with faint horizontal blade scoring
    for (int y = 0; y < size; ++y)
    {
        const auto rowStreak = rng.nextInt (6);

        for (int x = 0; x < size; ++x)
        {
            const auto grain = (juce::uint8) (rng.nextInt (14) + rowStreak);
            data.setPixelColour (x, y, juce::Colours::white.withAlpha ((float) grain / 255.0f));
        }
    }
}

void MauriModeAudioProcessorEditor::bakeWordmarks()
{
    const auto w = juce::jmax (1, getWidth());
    const auto h = juce::jmax (1, getHeight());

    // Header wordmark baked orange with a crisp pale edge
    const auto hw = 470;
    const auto hh = 62;
    wordmarkImage = juce::Image (juce::Image::ARGB, hw, hh, true);
    {
        juce::Graphics g (wordmarkImage);
        g.setFont (lookAndFeel.displayFont (54.0f));
        g.setColour (MauriLookAndFeel::spark().withAlpha (0.30f));
        g.drawText ("MAURI-MODE", juce::Rectangle<int> (0, 0, hw, hh).translated (0, -1),
                    juce::Justification::centredLeft, false);
        g.setColour (MauriLookAndFeel::blade());
        g.drawText ("MAURI-MODE", juce::Rectangle<int> (0, 0, hw, hh),
                    juce::Justification::centredLeft, false);
    }

    // Ghost background wordmark across the chassis
    ghostImage = juce::Image (juce::Image::ARGB, w, h, true);
    {
        juce::Graphics g (ghostImage);
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (-7.0f))
                            .translated ((float) w * 0.52f, (float) h * 0.56f));
        g.setFont (lookAndFeel.shadeFont (184.0f));
        g.setColour (MauriLookAndFeel::blade().withAlpha (0.45f));
        g.drawText ("MAURI-MODE", juce::Rectangle<int> (-540, -140, 1080, 280).translated (-4, -5),
                    juce::Justification::centred, false);
        g.setColour (MauriLookAndFeel::spark().withAlpha (0.45f));
        g.drawText ("MAURI-MODE", juce::Rectangle<int> (-540, -140, 1080, 280),
                    juce::Justification::centred, false);
        g.restoreState();
    }
}

void MauriModeAudioProcessorEditor::paintSlicedImage (juce::Graphics& g, const juce::Image& image,
                                                      juce::Rectangle<int> target, int bands,
                                                      float opacity, int seed)
{
    if (! image.isValid() || target.isEmpty() || bands <= 0)
        return;

    // Small, alternating offsets: the wordmark stays readable but is clearly cut.
    static constexpr std::array<int, 8> kOffsets { 0, 2, -2, 3, -1, 2, -3, 1 };

    const auto srcW = image.getWidth();
    const auto srcH = image.getHeight();

    g.setOpacity (opacity);

    for (int i = 0; i < bands; ++i)
    {
        const auto sy0 = srcH * i / bands;
        const auto sy1 = srcH * (i + 1) / bands;
        const auto sh = juce::jmax (1, sy1 - sy0 - 1);   // 1 px blade kerf between slices

        const auto dy0 = target.getY() + target.getHeight() * i / bands;
        const auto dy1 = target.getY() + target.getHeight() * (i + 1) / bands;
        const auto dh = juce::jmax (1, dy1 - dy0 - 1);

        const auto dx = kOffsets[(std::size_t) ((i + seed) % (int) kOffsets.size())];

        g.drawImage (image,
                     target.getX() + dx, dy0, target.getWidth(), dh,
                     0, sy0, srcW, sh);
    }

    g.setOpacity (1.0f);
}

void MauriModeAudioProcessorEditor::paint (juce::Graphics& g)
{
    paintBackground (g);
    paintHeader (g);
    paintPresetBar (g);

    for (const auto& card : cards)
    {
        paintCard (g, card);

        if (card.isScope)
            paintScope (g, card);
    }

    paintGrid (g);
    paintLegalNotice (g);

    g.setColour (MauriLookAndFeel::line());
    g.drawRect (getLocalBounds(), 1);
}

// The legal notice the AGPLv3 asks for, printed along the bottom edge: the
// copyright, the licence, the absence of warranty and where the source is.
void MauriModeAudioProcessorEditor::paintLegalNotice (juce::Graphics& g)
{
    const auto area = juce::Rectangle<int> (kMargin, getHeight() - 15,
                                            getWidth() - 2 * kMargin, 12);

    g.setFont (lookAndFeel.monoFont (7.0f));
    g.setColour (MauriLookAndFeel::dim().withAlpha (0.55f));
    drawTracked (g, "(C) 2026 Philopium - AGPLv3 - no warranty - source: github.com/Philopium/NULLCAM-plugins",
                 area, 0.05f, juce::Justification::centredLeft);
}

void MauriModeAudioProcessorEditor::paintBackground (juce::Graphics& g)
{
    g.fillAll (MauriLookAndFeel::steel());

    if (steelTexture.isValid())
    {
        g.setTiledImageFill (steelTexture, 0, 0, 1.0f);
        g.fillAll();
    }

    // Sliced ghost wordmark across the chassis
    paintSlicedImage (g, ghostImage, getLocalBounds(), 14, 0.09f, 2);

    // Hot orange blade strip along the top edge
    g.setColour (MauriLookAndFeel::blade());
    g.fillRect (0, 0, getWidth(), 3);
}

void MauriModeAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    g.setFont (lookAndFeel.monoFont (9.0f, true));
    g.setColour (MauriLookAndFeel::dim());
    drawTracked (g, "NULLCAM", headerArea.withHeight (12), 0.42f, juce::Justification::topLeft);

    const auto markRect = juce::Rectangle<int> (headerArea.getX(), headerArea.getY() + 8, 470, 58);
    paintSlicedImage (g, wordmarkImage, markRect, 6, 1.0f, 0);

    // The blade: a hot streak running out of the wordmark and fading away
    const auto streak = juce::Rectangle<float> ((float) markRect.getRight() - 34.0f,
                                                (float) markRect.getBottom() - 3.0f,
                                                240.0f, 2.0f);
    juce::ColourGradient fade (MauriLookAndFeel::blade().withAlpha (0.55f), streak.getX(), 0.0f,
                               MauriLookAndFeel::blade().withAlpha (0.0f), streak.getRight(), 0.0f, false);
    g.setGradientFill (fade);
    g.fillRect (streak);

    const auto right = headerArea.withTrimmedLeft (headerArea.getWidth() / 2);

    g.setFont (lookAndFeel.monoFont (9.0f, true));
    g.setColour (MauriLookAndFeel::blade().withAlpha (0.95f));
    drawTracked (g, "DYNAMIC RHYTHM SLICER", right.withHeight (12).translated (0, 18), 0.28f,
                 juce::Justification::topRight);

    g.setColour (MauriLookAndFeel::dim());
    drawTracked (g, "CAPTURE / SLICE / RATCHET / DYNAMICS",
                 right.withHeight (12).translated (0, 34), 0.2f, juce::Justification::topRight);

    g.setColour (MauriLookAndFeel::dim().withAlpha (0.72f));
    drawTracked (g, "PLAYING CONTROLS THE GRID",
                 right.withHeight (12).translated (0, 49), 0.2f, juce::Justification::topRight);

    g.setColour (MauriLookAndFeel::line());
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 1, headerArea.getWidth(), 1);
    g.setColour (MauriLookAndFeel::blade());
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 2, 58, 2);
}

void MauriModeAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    g.setFont (lookAndFeel.monoFont (8.5f, true));
    g.setColour (MauriLookAndFeel::dim());
    drawTracked (g, "PRESET", { presetBarArea.getX(), presetBarArea.getY(), 54, kPresetBarH }, 0.3f,
                 juce::Justification::centredLeft);

    const auto plate = presetPlateArea.toFloat();

    g.setColour (juce::Colour (0xff0c0d11));
    g.fillRoundedRectangle (plate, 2.0f);
    g.setColour (MauriLookAndFeel::line());
    g.drawRoundedRectangle (plate.reduced (0.5f), 2.0f, 1.0f);

    g.setColour (MauriLookAndFeel::blade());
    g.fillRect (presetPlateArea.getX() + 1, presetPlateArea.getY() + 6, 2,
                presetPlateArea.getHeight() - 12);

    g.setFont (lookAndFeel.displayFont (14.5f));
    g.setColour (MauriLookAndFeel::blade());
    g.drawText (presetName.isEmpty() ? "CUSTOM" : presetName.toUpperCase(),
                presetPlateArea.withTrimmedLeft (12).withTrimmedRight (22),
                juce::Justification::centredLeft, false);

    if (edited)
    {
        g.setFont (lookAndFeel.displayFont (15.0f));
        g.setColour (MauriLookAndFeel::spark().withAlpha (0.9f));
        g.drawText ("*", presetPlateArea.withTrimmedRight (9), juce::Justification::centredRight, false);
    }
}

void MauriModeAudioProcessorEditor::paintCard (juce::Graphics& g, const Card& card)
{
    const auto r = card.area.toFloat();

    juce::ColourGradient fill (MauriLookAndFeel::card().brighter (0.04f), r.getX(), r.getY(),
                               MauriLookAndFeel::card().darker (0.28f), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (MauriLookAndFeel::line());
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

    // Header band inside card
    g.setFont (lookAndFeel.monoFont (8.5f, true));
    g.setColour (MauriLookAndFeel::blade());
    drawTracked (g, card.number, { card.area.getX() + 9, card.area.getY() + 7, 20, 11 }, 0.1f,
                 juce::Justification::topLeft);

    g.setColour (MauriLookAndFeel::ink());
    drawTracked (g, card.name, { card.area.getX() + 28, card.area.getY() + 7,
                                 card.area.getWidth() - 110, 11 }, 0.24f,
                 juce::Justification::topLeft);

    g.setFont (lookAndFeel.monoFont (7.5f));
    g.setColour (MauriLookAndFeel::dim().withAlpha (0.75f));
    drawTracked (g, card.caption, { card.area.getX() + 90, card.area.getY() + 7,
                                    card.area.getWidth() - 99, 11 }, 0.14f,
                 juce::Justification::topRight);

    g.setColour (MauriLookAndFeel::line());
    g.fillRect (card.area.getX() + 9, card.area.getY() + 22, card.area.getWidth() - 18, 1);

    for (int index : card.knobIndices)
    {
        const auto& knob = *knobs[(std::size_t) index];

        g.setFont (lookAndFeel.monoFont (8.0f, true));
        g.setColour (MauriLookAndFeel::dim());
        drawTracked (g, knob.label,
                     { knob.cell.getX(), knob.cell.getY() + 1, knob.cell.getWidth(), 10 },
                     0.18f, juce::Justification::centred);
    }
}

void MauriModeAudioProcessorEditor::paintScope (juce::Graphics& g, const Card&)
{
    const auto& snap = processor.getSnapshot();
    const auto vel = juce::jlimit (0.0f, 1.0f, snap.velocity.load (std::memory_order_relaxed));
    const auto env = juce::jlimit (0.0f, 1.0f, snap.envelope.load (std::memory_order_relaxed));
    const auto gateOpen = snap.gateOpen.load (std::memory_order_relaxed) != 0;
    const auto head = juce::jlimit (0.0f, 1.0f, snap.head.load (std::memory_order_relaxed));
    const auto frac = juce::jlimit (0.0f, 1.0f, snap.sliceFraction.load (std::memory_order_relaxed));
    const auto clockIdx = juce::jlimit (0, 2, snap.clock.load (std::memory_order_relaxed));
    const auto step = snap.step.load (std::memory_order_relaxed);
    const auto len = juce::jmax (1, snap.patternLength.load (std::memory_order_relaxed));
    const auto onsets = snap.onsets.load (std::memory_order_relaxed);

    // ---- waveform capture screen -------------------------------------------
    const auto scr = screenArea.toFloat();
    g.setColour (juce::Colour (0xff07080b));
    g.fillRoundedRectangle (scr, 3.0f);
    g.setColour (MauriLookAndFeel::line().brighter (0.15f));
    g.drawRoundedRectangle (scr.reduced (0.5f), 3.0f, 1.0f);

    // Hairline grid inside screen
    g.setColour (MauriLookAndFeel::line().withAlpha (0.45f));
    g.drawHorizontalLine ((int) scr.getCentreY(), scr.getX() + 2.0f, scr.getRight() - 2.0f);

    for (int i = 1; i < 4; ++i)
    {
        const auto gx = scr.getX() + scr.getWidth() * (float) i * 0.25f;
        g.drawVerticalLine ((int) gx, scr.getY() + 2.0f, scr.getBottom() - 2.0f);
    }

    // Waveform bars
    const auto w = juce::jmax (1, screenArea.getWidth() - 8);
    const auto midY = scr.getCentreY();
    const auto maxAmp = scr.getHeight() * 0.43f;

    for (int px = 0; px < w; px += 2)
    {
        const auto i0 = px * mauri::kScopePoints / w;
        const auto i1 = juce::jmin (mauri::kScopePoints, (px + 2) * mauri::kScopePoints / w);
        float peak = 0.0f;

        for (int idx = i0; idx < i1; ++idx)
            peak = std::max (peak, scope[(std::size_t) idx]);

        const auto barH = juce::jmax (1.5f, peak * maxAmp);
        const auto bx = scr.getX() + 4.0f + (float) px;

        g.setColour (MauriLookAndFeel::blade().withAlpha (0.35f + 0.55f * peak));
        g.fillRect (bx, midY - barH, 1.5f, barH * 2.0f);
    }

    // Read head blade line
    const auto hx = scr.getX() + 4.0f + head * (scr.getWidth() - 8.0f);
    g.setColour (MauriLookAndFeel::blade().withAlpha (gateOpen ? 0.22f : 0.07f));
    g.fillRect (hx - 4.0f, scr.getY() + 2.0f, 8.0f, scr.getHeight() - 4.0f);
    g.setColour (gateOpen ? MauriLookAndFeel::spark() : MauriLookAndFeel::blade().withAlpha (0.5f));
    g.fillRect (hx - 0.5f, scr.getY() + 2.0f, 1.5f, scr.getHeight() - 4.0f);

    // Screen tag
    g.setFont (lookAndFeel.monoFont (7.5f, true));
    g.setColour (MauriLookAndFeel::dim());
    g.drawText ("RING BUFFER", screenArea.reduced (6, 4), juce::Justification::topLeft, false);

    // ---- dynamics meter ----------------------------------------------------
    const auto met = meterArea.toFloat();
    g.setColour (juce::Colour (0xff090a0e));
    g.fillRoundedRectangle (met, 3.0f);
    g.setColour (MauriLookAndFeel::line());
    g.drawRoundedRectangle (met.reduced (0.5f), 3.0f, 1.0f);

    g.setFont (lookAndFeel.monoFont (8.0f, true));
    g.setColour (MauriLookAndFeel::dim());
    drawTracked (g, "DYNAMICS -> PATTERN", meterArea.reduced (8, 5).withHeight (11), 0.16f,
                 juce::Justification::topLeft);

    // Transient LED
    const auto led = juce::Rectangle<float> (met.getRight() - 16.0f, met.getY() + 6.0f, 8.0f, 8.0f);
    g.setColour (onsetFlash > 0.05f ? MauriLookAndFeel::blade().interpolatedWith (MauriLookAndFeel::spark(), onsetFlash)
                                    : juce::Colour (0xff241612));
    g.fillEllipse (led);

    // Velocity bar
    const auto bar = meterArea.reduced (8, 0).withTop (meterArea.getY() + 22).withHeight (11).toFloat();
    g.setColour (juce::Colour (0xff171a21));
    g.fillRoundedRectangle (bar, 2.0f);

    const auto fillW = bar.getWidth() * vel;
    if (fillW > 1.0f)
    {
        juce::ColourGradient grad (MauriLookAndFeel::blade(), bar.getX(), bar.getY(),
                                   MauriLookAndFeel::spark(), bar.getX() + bar.getWidth(), bar.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar.withWidth (fillW), 2.0f);
    }

    // Instant envelope hairline
    const auto envX = bar.getX() + bar.getWidth() * env;
    g.setColour (MauriLookAndFeel::ink().withAlpha (0.75f));
    g.fillRect (envX, bar.getY() - 1.0f, 1.5f, bar.getHeight() + 2.0f);

    g.setFont (lookAndFeel.monoFont (7.5f));
    g.setColour (MauriLookAndFeel::dim());
    g.drawText ("HIT " + juce::String (juce::roundToInt (vel * 100.0f)) + "%",
                meterArea.reduced (8, 4).withTop (meterArea.getY() + 36),
                juce::Justification::left, false);
    g.drawText ("SLICE " + juce::String (juce::roundToInt (frac * 100.0f)) + "%",
                meterArea.reduced (8, 4).withTop (meterArea.getY() + 36),
                juce::Justification::right, false);

    // ---- status readout ----------------------------------------------------
    const auto rd = readoutArea.toFloat();
    g.setColour (juce::Colour (0xff090a0e));
    g.fillRoundedRectangle (rd, 3.0f);
    g.setColour (MauriLookAndFeel::line());
    g.drawRoundedRectangle (rd.reduced (0.5f), 3.0f, 1.0f);

    const auto bpm = processor.displayBpm.load (std::memory_order_relaxed);
    const auto playing = processor.displayPlaying.load (std::memory_order_relaxed);

    auto drawRow = [&] (int row, const juce::String& k, const juce::String& v, juce::Colour vc)
    {
        const auto rowRect = readoutArea.reduced (9, 6).withTop (readoutArea.getY() + 8 + row * 19).withHeight (16);
        g.setFont (lookAndFeel.monoFont (8.5f, true));
        g.setColour (MauriLookAndFeel::dim());
        drawTracked (g, k, rowRect, 0.16f, juce::Justification::left);

        g.setFont (lookAndFeel.monoFont (9.5f, true));
        g.setColour (vc);
        g.drawText (v, rowRect, juce::Justification::right, false);
    };

    const auto clockStr = juce::String (kClockNames[(std::size_t) clockIdx])
                          + (clockIdx == 0 ? (" / " + juce::String (bpm, 1) + " BPM" + (playing ? " *" : "")) : "");

    const auto energy = juce::jlimit (0.0f, 1.0f, snap.patternEnergy.load (std::memory_order_relaxed));

    const char* tierName = "PULSE";
    switch (mauri::complexityTier (energy))
    {
        case mauri::Tier::pulse:  tierName = "PULSE";  break;
        case mauri::Tier::groove: tierName = "GROOVE"; break;
        case mauri::Tier::syncop: tierName = "SYNC";   break;
        case mauri::Tier::shred:  tierName = "SHRED";  break;
    }

    drawRow (0, "CLOCK", clockStr, MauriLookAndFeel::blade());
    drawRow (1, "STEP", juce::String (step + 1).paddedLeft ('0', 2) + " / "
                         + juce::String (len).paddedLeft ('0', 2), MauriLookAndFeel::ink());
    drawRow (2, "PATTERN", juce::String (juce::roundToInt (energy * 100.0f)) + "% " + tierName,
             MauriLookAndFeel::blade());
    drawRow (3, "ONSETS", juce::String (onsets), MauriLookAndFeel::ink());
}

void MauriModeAudioProcessorEditor::paintGrid (juce::Graphics& g)
{
    const auto r = gridArea.toFloat();

    juce::ColourGradient fill (MauriLookAndFeel::card().brighter (0.04f), r.getX(), r.getY(),
                               MauriLookAndFeel::card().darker (0.28f), r.getX(), r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (MauriLookAndFeel::line());
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

    const auto timeSig = juce::roundToInt (processor.apvts.getRawParameterValue (ids::timeSig)->load());
    const auto style = juce::roundToInt (processor.apvts.getRawParameterValue (ids::style)->load());
    const auto sliceKnob = processor.apvts.getRawParameterValue (ids::slice)->load();

    const auto& snap = processor.getSnapshot();
    const auto energy = juce::jlimit (0.0f, 1.0f, snap.patternEnergy.load (std::memory_order_relaxed));
    const auto activeStep = snap.step.load (std::memory_order_relaxed);
    const auto gateOpen = snap.gateOpen.load (std::memory_order_relaxed) != 0;
    const auto total = mauri::stepsForTimeSig (timeSig);

    const char* tierName = "PULSE";
    switch (mauri::complexityTier (energy))
    {
        case mauri::Tier::pulse:  tierName = "PULSE";  break;
        case mauri::Tier::groove: tierName = "GROOVE"; break;
        case mauri::Tier::syncop: tierName = "SYNC";   break;
        case mauri::Tier::shred:  tierName = "SHRED";  break;
    }

    g.setFont (lookAndFeel.monoFont (8.5f, true));
    g.setColour (MauriLookAndFeel::blade());
    drawTracked (g, "06", { gridArea.getX() + 9, gridArea.getY() + 5, 20, 11 }, 0.1f,
                 juce::Justification::topLeft);

    g.setColour (MauriLookAndFeel::ink());
    drawTracked (g, "SEQUENCE", { gridArea.getX() + 28, gridArea.getY() + 5, 120, 11 }, 0.24f,
                 juce::Justification::topLeft);

    g.setColour (MauriLookAndFeel::dim());
    const auto info = juce::String (mauri::kTimeSigsInfo[(std::size_t) timeSig].name) + "  /  "
                      + juce::String (mauri::kStyleNames[(std::size_t) style]) + "  /  DYN "
                      + juce::String (juce::roundToInt (energy * 100.0f)) + "%";
    drawTracked (g, info, { gridArea.getX() + 140, gridArea.getY() + 5, gridArea.getWidth() - 340, 11 },
                 0.16f, juce::Justification::topRight);

    // Complexity tier: what the playing is allowed to do at this moment
    g.setColour (MauriLookAndFeel::blade().withAlpha (0.95f));
    drawTracked (g, juce::String (tierName), { gridArea.getRight() - 185, gridArea.getY() + 5, 92, 11 },
                 0.2f, juce::Justification::topRight);

    // Live energy bar: how much the playing is rewriting the grid right now
    const auto bar = juce::Rectangle<float> ((float) gridArea.getRight() - 78.0f,
                                             (float) gridArea.getY() + 5.0f,
                                             70.0f, 8.0f);
    g.setColour (juce::Colour (0xff171a21));
    g.fillRoundedRectangle (bar, 2.0f);

    if (energy > 0.001f)
    {
        juce::ColourGradient grad (MauriLookAndFeel::blade(), bar.getX(), bar.getY(),
                                   MauriLookAndFeel::spark(), bar.getRight(), bar.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * energy), 2.0f);
    }

    const auto cellsTop = gridArea.getY() + 20;
    const auto cellsH = gridArea.getBottom() - 6 - cellsTop;
    const auto cellsX = gridArea.getX() + 8;
    const auto cellsW = gridArea.getWidth() - 16;
    const auto gap = 3;
    const auto cellW = (cellsW - (mauri::kMaxSteps - 1) * gap) / mauri::kMaxSteps;

    for (int i = 0; i < mauri::kMaxSteps; ++i)
    {
        const auto cell = juce::Rectangle<int> (cellsX + i * (cellW + gap), cellsTop, cellW, cellsH).toFloat();
        const auto inRange = i < total;
        const auto isCurrent = inRange && i == activeStep;
        const auto step = mauri::evaluateStep (timeSig, style, i, energy);

        g.setColour (inRange ? juce::Colour (0xff0b0c10) : juce::Colour (0xff090a0d).withAlpha (0.35f));
        g.fillRoundedRectangle (cell, 2.0f);

        if (step.on && inRange)
        {
            const auto barFrac = juce::jlimit (0.14f, 1.0f, step.len * sliceKnob);
            const auto barH = (cell.getHeight() - 11.0f) * barFrac;
            const auto barRect = juce::Rectangle<float> (cell.getX() + 3.0f,
                                                         cell.getBottom() - 3.0f - barH,
                                                         cell.getWidth() - 6.0f,
                                                         barH);

            g.setColour (isCurrent ? MauriLookAndFeel::blade()
                                   : MauriLookAndFeel::blade().withAlpha (0.40f));
            g.fillRoundedRectangle (barRect, 1.5f);

            // Ratchets: extra ticks above the bar
            if (step.extraRepeats > 0)
            {
                g.setColour (MauriLookAndFeel::spark().withAlpha (0.75f));

                for (int tick = 0; tick < step.extraRepeats; ++tick)
                    g.fillRect (cell.getCentreX() - 4.0f + tick * 5.0f, cell.getY() + 12.0f, 3.0f, 2.0f);
            }
        }

        // Accent marker: the metrical weight of the step
        if (inRange && step.accent)
        {
            juce::Path marker;
            marker.addTriangle (cell.getCentreX() - 3.0f, cell.getY() + 2.0f,
                                cell.getCentreX() + 3.0f, cell.getY() + 2.0f,
                                cell.getCentreX(), cell.getY() + 7.0f);
            g.setColour (isCurrent ? MauriLookAndFeel::spark() : MauriLookAndFeel::blade().withAlpha (0.8f));
            g.fillPath (marker);
        }

        g.setColour (isCurrent ? (gateOpen ? MauriLookAndFeel::spark() : MauriLookAndFeel::blade())
                               : (inRange ? MauriLookAndFeel::line() : MauriLookAndFeel::line().withAlpha (0.35f)));
        g.drawRoundedRectangle (cell.reduced (0.5f), 2.0f, isCurrent ? 1.5f : 1.0f);

        // Step index
        g.setFont (lookAndFeel.monoFont (7.0f, true));
        g.setColour (isCurrent ? MauriLookAndFeel::spark()
                               : (inRange ? MauriLookAndFeel::dim() : MauriLookAndFeel::dim().withAlpha (0.3f)));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2),
                    cell.toNearestInt().reduced (3, 2).withHeight (9),
                    juce::Justification::topLeft, false);

        // Mode glyph
        if (step.on && inRange)
        {
            const char* glyph = "X";
            switch (step.mode)
            {
                case mauri::Mode::live:    glyph = step.reverse ? "<X" : "X"; break;
                case mauri::Mode::scatter: glyph = step.reverse ? "<S" : "S"; break;
                case mauri::Mode::onset:   glyph = step.reverse ? "<O" : "O"; break;
                case mauri::Mode::hold:    glyph = step.reverse ? "<H" : "H"; break;
            }

            g.setColour (isCurrent ? MauriLookAndFeel::spark() : MauriLookAndFeel::ink().withAlpha (0.8f));
            g.drawText (glyph, cell.toNearestInt().reduced (3, 2).withHeight (9),
                        juce::Justification::topRight, false);
        }
    }
}

void MauriModeAudioProcessorEditor::resized()
{
    bakeWordmarks();

    headerArea = { kMargin, 16, getWidth() - 2 * kMargin, kHeaderH };
    presetBarArea = { kMargin, headerArea.getBottom() + kGap, getWidth() - 2 * kMargin, kPresetBarH };

    presetPlateArea = { presetBarArea.getX() + 56, presetBarArea.getY(), 220, kPresetBarH };
    presetBox.setBounds (presetPlateArea.getRight() + 10, presetBarArea.getY(), 170, kPresetBarH);
    saveButton.setBounds (presetBox.getRight() + 10, presetBarArea.getY(), 64, kPresetBarH);
    deleteButton.setBounds (saveButton.getRight() + 8, presetBarArea.getY(), 56, kPresetBarH);

    const auto mainTop = presetBarArea.getBottom() + kGap;
    const auto colAW = 260;
    const auto colBW = 270;
    const auto colCW = getWidth() - 2 * kMargin - colAW - colBW - 2 * kGap;

    const auto colAX = kMargin;
    const auto colBX = colAX + colAW + kGap;
    const auto colCX = colBX + colBW + kGap;

    if (cards.size() >= 5)
    {
        cards[0].area = { colAX, mainTop, colAW, kMainH };                  // CAPTURE
        cards[1].area = { colBX, mainTop, colBW, kRow1H };                  // RHYTHM
        cards[2].area = { colBX, mainTop + kRow1H + kGap, colBW, kRow2H };  // BLADE
        cards[3].area = { colCX, mainTop, colCW, kRow1H };                  // DYNAMICS
        cards[4].area = { colCX, mainTop + kRow1H + kGap, colCW, kRow2H };  // OUTPUT
    }

    // Scope card internal layout
    if (! cards.empty())
    {
        const auto& sc = cards[0].area;
        screenArea = { sc.getX() + 8, sc.getY() + 30, sc.getWidth() - 16, 150 };
        meterArea = { sc.getX() + 8, screenArea.getBottom() + 8, sc.getWidth() - 16, 54 };
        readoutArea = { sc.getX() + 8, meterArea.getBottom() + 8, sc.getWidth() - 16,
                        sc.getBottom() - 8 - (meterArea.getBottom() + 8) };
    }

    // Place knobs inside each card
    for (auto& card : cards)
    {
        if (card.knobIndices.empty())
            continue;

        const auto contentTop = card.area.getY() + 28;
        const auto contentBottom = card.area.getBottom() - 6;
        const auto cols = juce::jmax (1, card.columns);
        const auto count = (int) card.knobIndices.size();
        const auto rows = (count + cols - 1) / cols;

        const auto cellW = (card.area.getWidth() - 12) / cols;
        const auto cellH = (contentBottom - contentTop) / juce::jmax (1, rows);

        for (int i = 0; i < count; ++i)
        {
            auto& knob = *knobs[(std::size_t) card.knobIndices[(std::size_t) i]];
            const auto col = i % cols;
            const auto row = i / cols;

            knob.cell = { card.area.getX() + 6 + col * cellW,
                          contentTop + row * cellH,
                          cellW, cellH };

            const auto boxW = juce::jmin (74, cellW - 4);
            knob.slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, boxW, 17);
            knob.slider->setBounds (knob.cell.getX() + 1, knob.cell.getY() + 11,
                                    knob.cell.getWidth() - 2, knob.cell.getHeight() - 13);
        }
    }

    gridArea = { kMargin, mainTop + kMainH + kGap, getWidth() - 2 * kMargin, kGridH };
}

//==============================================================================
void MauriModeAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");
    presetBox.addItem ("MAURI", kMauriItem);
    presetBox.addItem ("ODD 7/8", kOdd78Item);
    presetBox.addItem ("GHOST 5/4", kGhost54Item);
    presetBox.addItem ("SHRED 6/8", kShred68Item);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], kUserItemFirst + i);
    }

    presetBox.setSelectedId (0, juce::dontSendNotification);
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void MauriModeAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId <= 0)
        return;

    juce::Component::SafePointer<MauriModeAudioProcessorEditor> safe (this);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe != nullptr)
            safe->presetBox.setSelectedId (0, juce::dontSendNotification);
    });

    if (itemId == kMauriItem)
    {
        processor.applyPreset (MauriModeAudioProcessor::Preset::mauri);
        setPresetName ("MAURI");
    }
    else if (itemId == kOdd78Item)
    {
        processor.applyPreset (MauriModeAudioProcessor::Preset::odd78);
        setPresetName ("ODD 7/8");
    }
    else if (itemId == kGhost54Item)
    {
        processor.applyPreset (MauriModeAudioProcessor::Preset::ghost54);
        setPresetName ("GHOST 5/4");
    }
    else if (itemId == kShred68Item)
    {
        processor.applyPreset (MauriModeAudioProcessor::Preset::shred68);
        setPresetName ("SHRED 6/8");
    }
    else
    {
        const auto index = itemId - kUserItemFirst;

        if (juce::isPositiveAndBelow (index, userPresets.size()))
        {
            const auto name = userPresets[index];

            if (processor.loadUserPreset (name))
                setPresetName (name);
        }
    }
}

void MauriModeAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;
    edited = false;

    for (std::size_t i = 0; i < knobs.size(); ++i)
        referenceValues[i] = currentValue (i);

    deleteButton.setEnabled (userPresets.contains (name));
    repaint();
}

void MauriModeAudioProcessorEditor::updateEditedState()
{
    if (knobs.empty())
        return;

    bool dirty = false;

    for (std::size_t i = 0; i < knobs.size(); ++i)
        if (std::abs (currentValue (i) - referenceValues[i]) > 1.0e-4f)
        {
            dirty = true;
            break;
        }

    if (dirty != edited)
    {
        edited = dirty;
        repaint (presetBarArea);
    }
}

void MauriModeAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || userPresets.contains (suggested))
        suggested = suggested.isEmpty() ? "MY RHYTHM" : suggested;
    else
        suggested = suggested + " COPY";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + MauriModeAudioProcessor::getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<MauriModeAudioProcessorEditor> safe (this);

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
            }
        }), true);
}

void MauriModeAudioProcessorEditor::promptDeletePreset()
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

    juce::Component::SafePointer<MauriModeAudioProcessorEditor> safe (this);

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

float MauriModeAudioProcessorEditor::currentValue (std::size_t index) const
{
    if (auto* value = processor.apvts.getRawParameterValue (knobs[index]->paramId))
        return value->load();

    return 0.0f;
}

void MauriModeAudioProcessorEditor::parameterChanged (const juce::String&, float)
{
    // Timer updates edited state and grid on the message thread.
}

void MauriModeAudioProcessorEditor::timerCallback()
{
    pullLiveData();
}

void MauriModeAudioProcessorEditor::pullLiveData()
{
    processor.copyScope (scope.data(), (int) scope.size());

    const auto onsets = processor.getSnapshot().onsets.load (std::memory_order_relaxed);
    if (onsets != lastOnsets)
    {
        lastOnsets = onsets;
        onsetFlash = 1.0f;
    }
    else
    {
        onsetFlash = juce::jmax (0.0f, onsetFlash - 0.14f);
    }

    updateEditedState();

    if (! cards.empty())
        repaint (cards[0].area);

    repaint (gridArea);
}
