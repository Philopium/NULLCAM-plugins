#include "PluginEditor.h"

#include <PierAssets.h>

namespace
{
// The panel is a fixed card, like a pedal: everything is placed from these numbers.
constexpr int kWidth = 700;
constexpr int kHeight = 552;
constexpr int kMargin = 20;
constexpr int kHeaderH = 82;
constexpr int kPresetBarH = 32;
constexpr int kGap = 12;
constexpr int kStageGap = 10;
constexpr int kStageTitleH = 26;
constexpr int kKnobCellH = 96;
constexpr int kStageCaptionH = 20;
constexpr int kBaseH = 46;

constexpr int kMotionItem = 1;
constexpr int kSortItem = 2;
constexpr int kCleanItem = 3;
constexpr int kUserItemFirst = 100;

juce::Typeface::Ptr loadFace (const void* data, std::size_t size)
{
    return juce::Typeface::createSystemTypefaceFor (data, size);
}

const juce::Typeface::Ptr& bungee()
{
    static const juce::Typeface::Ptr face = loadFace (PierAssets::BungeeRegular_ttf,
                                                      (std::size_t) PierAssets::BungeeRegular_ttfSize);
    return face;
}

const juce::Typeface::Ptr& bungeeShade()
{
    static const juce::Typeface::Ptr face = loadFace (PierAssets::BungeeShadeRegular_ttf,
                                                      (std::size_t) PierAssets::BungeeShadeRegular_ttfSize);
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
juce::Colour PierLookAndFeel::paper() { return juce::Colour (0xff0b0b0d); }
juce::Colour PierLookAndFeel::panel() { return juce::Colour (0xff14141a); }
juce::Colour PierLookAndFeel::line()  { return juce::Colour (0xff272731); }
juce::Colour PierLookAndFeel::ink()   { return juce::Colour (0xffe9e9ef); }
juce::Colour PierLookAndFeel::cream() { return juce::Colour (0xfff3ecd7); }
juce::Colour PierLookAndFeel::dim()   { return juce::Colour (0xff8b8b99); }
juce::Colour PierLookAndFeel::acid()  { return juce::Colour (0xffcdf24b); }

PierLookAndFeel::PierLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101014));
    setColour (juce::PopupMenu::textColourId, ink());
    setColour (juce::PopupMenu::headerTextColourId, acid().withAlpha (0.75f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, acid());
    setColour (juce::PopupMenu::highlightedTextColourId, paper());

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff101014));
    setColour (juce::AlertWindow::textColourId, ink());
    setColour (juce::AlertWindow::outlineColourId, line());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0b0e));
    setColour (juce::TextEditor::textColourId, ink());
    setColour (juce::TextEditor::highlightColourId, acid().withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, line());
    setColour (juce::TextEditor::focusedOutlineColourId, acid());

    setColour (juce::Label::textColourId, ink());
}

juce::Font PierLookAndFeel::displayFont (float height) const
{
    return juce::Font (juce::FontOptions (bungee()).withHeight (height));
}

juce::Font PierLookAndFeel::shadeFont (float height) const
{
    return juce::Font (juce::FontOptions (bungeeShade()).withHeight (height));
}

juce::Font PierLookAndFeel::monoFont (float height, bool bold) const
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

void PierLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.isEnabled() ? acid() : acid().withAlpha (0.3f);

    // scale ticks, longer every five
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = rotaryStartAngle + (float) i / 10.0f * (rotaryEndAngle - rotaryStartAngle);
        g.setColour (i % 5 == 0 ? line().brighter (0.45f) : line().brighter (0.15f));
        g.drawLine ({ centre.getPointOnCircumference (radius + 2.5f, a),
                      centre.getPointOnCircumference (radius + (i % 5 == 0 ? 6.0f : 4.0f), a) }, 1.0f);
    }

    const auto arcRadius = radius - 3.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff2b2b35));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, juce::jmax (rotaryStartAngle, angle), true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // the knob itself: a turned metal skirt
    const auto body = radius - 7.5f;

    juce::ColourGradient metal (juce::Colour (0xff4c4c5a), centre.x - body * 0.45f,
                                centre.y - body * 1.05f,
                                juce::Colour (0xff0c0c10), centre.x + body * 0.35f,
                                centre.y + body * 1.1f, false);
    metal.addColour (0.42, juce::Colour (0xff2a2a34));
    metal.addColour (0.72, juce::Colour (0xff181820));
    g.setGradientFill (metal);
    g.fillEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f);

    // knurling: fine radial teeth around the skirt
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    for (int i = 0; i < 36; ++i)
    {
        const auto a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
        g.drawLine ({ centre.getPointOnCircumference (body * 0.86f, a),
                      centre.getPointOnCircumference (body * 0.995f, a) }, 0.7f);
    }

    // the top cap, catching the light
    const auto cap = body * 0.72f;
    juce::ColourGradient capFill (juce::Colour (0xff33333f), centre.x, centre.y - cap,
                                  juce::Colour (0xff16161d), centre.x, centre.y + cap, false);
    g.setGradientFill (capFill);
    g.fillEllipse (centre.x - cap, centre.y - cap, cap * 2.0f, cap * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawEllipse (centre.x - cap, centre.y - cap, cap * 2.0f, cap * 2.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f, 1.0f);

    // pointer
    const auto tip = centre.getPointOnCircumference (body * 0.92f, angle);
    const auto tail = centre.getPointOnCircumference (body * 0.3f, angle);
    g.setColour (accent);
    g.drawLine ({ tail, tip }, 2.4f);
    g.fillEllipse (tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
}

juce::Label* PierLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);

    label->setFont (monoFont (11.5f, true));
    label->setJustificationType (juce::Justification::centred);
    label->setColour (juce::Label::textColourId, juce::Colour (0xffe4e4ec));
    label->setColour (juce::Label::textWhenEditingColourId, acid());
    label->setColour (juce::Label::backgroundColourId, juce::Colour (0xcc0a0a0e));
    label->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff17171d));
    label->setColour (juce::Label::outlineColourId, line());
    label->setColour (juce::Label::outlineWhenEditingColourId, acid().withAlpha (0.7f));

    return label;
}

void PierLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto fill = down ? acid()
                           : (highlighted ? juce::Colour (0xff1d1d24) : juce::Colour (0xff131318));

    g.setColour (button.isEnabled() ? fill : fill.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (down ? acid()
                      : (highlighted ? acid().withAlpha (0.8f)
                                     : line().withAlpha (button.isEnabled() ? 1.0f : 0.5f)));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);
}

void PierLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                      bool highlighted, bool down)
{
    g.setFont (monoFont (9.5f, true).withExtraKerningFactor (0.18f));
    g.setColour (! button.isEnabled() ? dim().withAlpha (0.5f)
                                      : (down ? paper() : (highlighted ? acid() : juce::Colour (0xffd7d7e0))));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void PierLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);

    g.setColour (juce::Colour (0xff0e0e12));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (box.isMouseOver (true) ? acid().withAlpha (0.7f) : line());
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    juce::Path arrow;
    const auto cx = (float) width - 13.0f;
    const auto cy = (float) height * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);

    g.setColour (acid());
    g.fillPath (arrow);
}

juce::Font PierLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return monoFont (10.0f, true).withExtraKerningFactor (0.16f);
}

juce::Font PierLookAndFeel::getPopupMenuFont()
{
    return monoFont (11.0f);
}

void PierLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xff101014));
    g.setColour (line());
    g.drawRect (0, 0, width, height, 1);
}

void PierLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // Only the knob value boxes are little screens: a combo box owns a label too, and
    // that one must stay transparent so the box can draw its own text.
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) == nullptr)
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }

    const auto r = label.getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (juce::Colour (0xff07070a));
    g.fillRoundedRectangle (r, 2.0f);

    g.setColour (label.isBeingEdited() ? acid() : line().brighter (0.22f));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    if (label.isBeingEdited())
        return;   // the editor draws its own text

    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));
    g.drawFittedText (label.getText(), label.getLocalBounds().reduced (4, 0),
                      label.getJustificationType(), 1, 1.0f);
}

//==============================================================================
NullcamChainAudioProcessorEditor::NullcamChainAudioProcessorEditor (NullcamChainAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    // The chain, left to right, in the order the signal runs through it.
    addStage ("01", "BAND", "band pass", { { ids::hpf, "HPF", Unit::hz },
                                           { ids::lpf, "LPF", Unit::khz } });

    addStage ("02", "CRUSH", "10 to 2 bit", { { ids::crush, "CRUSH", Unit::percent } }, true);

    addStage ("03", "ECHO", "feed the tail", { { ids::delayMs, "DELAY", Unit::ms },
                                               { ids::echo, "ECHO", Unit::percent },
                                               { ids::feedback, "FEEDBACK", Unit::percent } });

    addStage ("04", "RING", "117 Hz carrier", { { ids::ringHz, "RING Hz", Unit::hzTenth },
                                                { ids::ring, "RING", Unit::percent } });

    addStage ("05", "OUT", "dry + trim", { { ids::dry, "DRY", Unit::percent },
                                           { ids::output, "OUTPUT", Unit::percent } });

    presetBox.setTextWhenNothingSelected ("LOAD PRESET");
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    saveButton.onClick = [this] { promptSavePreset(); };
    deleteButton.onClick = [this] { promptDeletePreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);

    makePaperTexture();
    refreshPresetList();

    // The host restores the knobs, not the preset name, so read it back off the values.
    const auto current = processor.matchFactoryPreset();
    setPresetName (current.isNotEmpty() ? current : "CUSTOM");

    // Last, so the first resized() already sees every child and can place it.
    setSize (kWidth, kHeight);
}

NullcamChainAudioProcessorEditor::~NullcamChainAudioProcessorEditor()
{
    cancelPendingUpdate();

    for (auto& knob : knobs)
        processor.apvts.removeParameterListener (knob->paramId, this);

    if (activeDialog != nullptr)
    {
        activeDialog->setLookAndFeel (nullptr);
        activeDialog->exitModalState (0);
    }

    setLookAndFeel (nullptr);
}

void NullcamChainAudioProcessorEditor::addStage (const char* number, const char* name,
                                                 const char* caption,
                                                 std::initializer_list<KnobSpec> specs,
                                                 bool showsBitDepth)
{
    Stage stage;
    stage.number = number;
    stage.name = name;
    stage.caption = caption;
    stage.showsBitDepth = showsBitDepth;

    for (const auto& spec : specs)
    {
        auto knob = std::make_unique<Knob>();
        knob->paramId = spec.id;
        knob->label = spec.label;
        knob->unit = spec.unit;

        knob->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                       juce::Slider::TextBoxBelow);
        knob->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 19);
        knob->slider->setMouseDragSensitivity (450);
        knob->slider->setScrollWheelEnabled (true);
        addAndMakeVisible (*knob->slider);

        knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, spec.id, *knob->slider);

        // After the attachment, which installs the parameter's own text conversion:
        // these are the ones that print "330 Hz" and "98%" and read them back, so a
        // value can be typed exactly instead of hunted with the mouse.
        const auto unit = knob->unit;
        knob->slider->textFromValueFunction = [this, unit] (double value)
        { return valueText (value, unit); };
        knob->slider->valueFromTextFunction = [this, unit] (const juce::String& text)
        { return valueFromText (text, unit); };

        // The attachment refreshed the label with the parameter's raw text before the
        // two functions above existed; nudge it once so the box reads "330 Hz".
        knob->slider->updateText();

        processor.apvts.addParameterListener (spec.id, this);

        stage.knobIndices.push_back ((int) knobs.size());
        knobs.push_back (std::move (knob));
    }

    stages.push_back (std::move (stage));
}

juce::String NullcamChainAudioProcessorEditor::valueText (double value, Unit unit) const
{
    switch (unit)
    {
        case Unit::hz:      return juce::String (juce::roundToInt (value)) + " Hz";
        case Unit::hzTenth: return juce::String (value, 1) + " Hz";
        case Unit::khz:     return value >= 1000.0 ? juce::String (value / 1000.0, 1) + " kHz"
                                                   : juce::String (juce::roundToInt (value)) + " Hz";
        case Unit::ms:      return juce::String (juce::roundToInt (value)) + " ms";
        case Unit::percent: return juce::String (juce::roundToInt (value * 100.0)) + "%";
    }

    return {};
}

double NullcamChainAudioProcessorEditor::valueFromText (const juce::String& text, Unit unit) const
{
    auto cleaned = text.toLowerCase();
    const auto kilo = cleaned.contains ("k");
    cleaned = cleaned.retainCharacters ("0123456789.-");

    auto value = cleaned.getDoubleValue();

    if (unit == Unit::khz && kilo)
        value *= 1000.0;
    else if (unit == Unit::percent)
        value /= 100.0;

    return value;
}

void NullcamChainAudioProcessorEditor::makePaperTexture()
{
    constexpr int size = 256;
    paperTexture = juce::Image (juce::Image::ARGB, size, size, true);

    juce::Image::BitmapData data (paperTexture, juce::Image::BitmapData::writeOnly);
    juce::Random rng (0x9e3779b9);

    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const auto grain = (juce::uint8) (rng.nextInt (24) + rng.nextInt (8));
            data.setPixelColour (x, y, juce::Colours::white.withAlpha ((float) grain / 255.0f));
        }
}

void NullcamChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (PierLookAndFeel::paper());

    if (paperTexture.isValid())
    {
        g.setTiledImageFill (paperTexture, 0, 0, 1.0f);
        g.fillAll();
    }

    paintWordmarkGhost (g);

    // the acid edge, like the colour bar on a printed pedal box
    g.setColour (PierLookAndFeel::acid());
    g.fillRect (0, 0, getWidth(), 3);

    paintHeader (g);
    paintPresetBar (g);

    for (const auto& stage : stages)
        paintStage (g, stage);

    // the chain reads left to right, so mark the hops between the cards
    for (std::size_t i = 0; i + 1 < stages.size(); ++i)
    {
        const auto x = (float) stages[i].area.getRight() + (float) kStageGap * 0.5f;
        const auto y = (float) stages[i].area.getCentreY();

        juce::Path chevron;
        chevron.addTriangle (x - 1.5f, y - 3.0f, x + 1.5f, y, x - 1.5f, y + 3.0f);

        g.setColour (PierLookAndFeel::acid().withAlpha (0.5f));
        g.fillPath (chevron);
    }

    paintBaseplate (g);
    paintScrews (g);

    g.setColour (PierLookAndFeel::line());
    g.drawRect (getLocalBounds(), 1);
}

void NullcamChainAudioProcessorEditor::paintWordmarkGhost (juce::Graphics& g)
{
    const juce::String mark ("PIER-MODE");
    const juce::Rectangle<int> box (-520, -150, 1040, 300);

    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (-8.0f))
                        .translated ((float) getWidth() * 0.5f, (float) getHeight() * 0.58f));

    g.setFont (lookAndFeel.shadeFont (188.0f));

    g.setColour (PierLookAndFeel::acid().withAlpha (0.055f));
    g.drawText (mark, box.translated (-7, -9), juce::Justification::centred, false);

    g.setColour (PierLookAndFeel::cream().withAlpha (0.045f));
    g.drawText (mark, box, juce::Justification::centred, false);

    g.restoreState();
}

void NullcamChainAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    const auto markArea = headerArea.withHeight (68).translated (0, 8);

    g.setFont (lookAndFeel.monoFont (9.0f, true));
    g.setColour (PierLookAndFeel::dim());
    drawTracked (g, "NULLCAM", headerArea.withHeight (12).translated (0, 1), 0.42f,
                 juce::Justification::topLeft);

    // The goofy one: Bungee Shade, one ink only so the inline detail stays crisp.
    g.setFont (lookAndFeel.shadeFont (58.0f));
    g.setColour (PierLookAndFeel::acid());
    g.drawText ("PIER-MODE", markArea, juce::Justification::topLeft, false);

    const auto right = headerArea.withTrimmedLeft (headerArea.getWidth() / 2);

    g.setFont (lookAndFeel.monoFont (9.0f, true));
    g.setColour (PierLookAndFeel::acid().withAlpha (0.9f));
    drawTracked (g, "AUDIO DAMAGE CHAIN", right.withHeight (12).translated (0, 26), 0.3f,
                 juce::Justification::topRight);

    g.setColour (PierLookAndFeel::dim());
    drawTracked (g, "BAND - CRUSH - ECHO - RING - TRIM",
                 right.withHeight (12).translated (0, 42), 0.22f, juce::Justification::topRight);

    g.setColour (PierLookAndFeel::dim().withAlpha (0.7f));
    drawTracked (g, "STEREO / 48K / NO MERCY", right.withHeight (12).translated (0, 56), 0.2f,
                 juce::Justification::topRight);

    // the rule between header and controls, with a printed registration mark
    g.setColour (PierLookAndFeel::line());
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 1, headerArea.getWidth(), 1);
    g.setColour (PierLookAndFeel::acid());
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 2, 46, 2);
}

void NullcamChainAudioProcessorEditor::paintPresetBar (juce::Graphics& g)
{
    g.setFont (lookAndFeel.monoFont (8.5f, true));
    g.setColour (PierLookAndFeel::dim());
    drawTracked (g, "PRESET", { presetBarArea.getX(), presetBarArea.getY(), 50, kPresetBarH }, 0.3f,
                 juce::Justification::centredLeft);

    const auto plate = presetPlateArea.toFloat();

    g.setColour (juce::Colour (0xff0e0e12));
    g.fillRoundedRectangle (plate, 2.0f);
    g.setColour (PierLookAndFeel::line());
    g.drawRoundedRectangle (plate.reduced (0.5f), 2.0f, 1.0f);

    g.setColour (PierLookAndFeel::acid());
    g.fillRect (presetPlateArea.getX() + 1, presetPlateArea.getY() + 7, 2,
                presetPlateArea.getHeight() - 14);

    g.setFont (lookAndFeel.displayFont (15.0f));
    g.setColour (PierLookAndFeel::acid());
    g.drawText (presetName.isEmpty() ? "CUSTOM" : presetName.toUpperCase(),
                presetPlateArea.withTrimmedLeft (12).withTrimmedRight (22),
                juce::Justification::centredLeft, false);

    if (edited)
    {
        g.setFont (lookAndFeel.displayFont (15.0f));
        g.setColour (PierLookAndFeel::acid().withAlpha (0.85f));
        g.drawText ("*", presetPlateArea.withTrimmedRight (9), juce::Justification::centredRight, false);
    }
}

void NullcamChainAudioProcessorEditor::paintStage (juce::Graphics& g, const Stage& stage)
{
    const auto r = stage.area.toFloat();

    juce::ColourGradient card (PierLookAndFeel::panel().brighter (0.035f), r.getX(), r.getY(),
                               PierLookAndFeel::panel().darker (0.25f), r.getX(), r.getBottom(), false);
    g.setGradientFill (card);
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (PierLookAndFeel::line());
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

    g.setFont (lookAndFeel.monoFont (8.5f, true));
    g.setColour (PierLookAndFeel::acid().withAlpha (0.85f));
    drawTracked (g, stage.number, { stage.area.getX() + 9, stage.area.getY() + 8, 18, 11 }, 0.1f,
                 juce::Justification::topLeft);

    g.setColour (PierLookAndFeel::ink());
    drawTracked (g, stage.name, { stage.area.getX() + 26, stage.area.getY() + 8,
                                  stage.area.getWidth() - 34, 11 }, 0.24f,
                 juce::Justification::topLeft);

    g.setColour (PierLookAndFeel::line());
    g.fillRect (stage.area.getX() + 9, stage.area.getY() + 23, stage.area.getWidth() - 18, 1);

    for (int index : stage.knobIndices)
    {
        const auto& knob = *knobs[(std::size_t) index];

        g.setFont (lookAndFeel.monoFont (8.5f, true));
        g.setColour (PierLookAndFeel::dim());
        drawTracked (g, knob.label,
                     { knob.cell.getX(), knob.cell.getY() + 1, knob.cell.getWidth(), 10 },
                     0.2f, juce::Justification::centred);
    }

    // A stage with two knobs has a gap between them: print its number in the gap,
    // large and faint, so the space reads as artwork instead of a missing control.
    if (stage.knobIndices.size() == 2)
    {
        const auto gapTop = knobs[(std::size_t) stage.knobIndices[0]]->cell.getBottom();
        const auto gapBottom = knobs[(std::size_t) stage.knobIndices[1]]->cell.getY();

        if (gapBottom - gapTop > 56)
        {
            const juce::Rectangle<int> gapArea (stage.area.getX(), gapTop,
                                                stage.area.getWidth(), gapBottom - gapTop);

            g.setFont (lookAndFeel.shadeFont (60.0f));
            g.setColour (PierLookAndFeel::acid().withAlpha (0.09f));
            g.drawText (stage.number, gapArea, juce::Justification::centred, false);
        }
    }

    g.setFont (lookAndFeel.monoFont (7.5f));
    g.setColour (PierLookAndFeel::dim().withAlpha (0.7f));
    drawTracked (g, stage.caption,
                 { stage.area.getX() + 8, stage.area.getBottom() - kStageCaptionH + 1,
                   stage.area.getWidth() - 16, 12 }, 0.16f, juce::Justification::centred);

    if (stage.showsBitDepth)
        paintBitReadout (g, stage);
}

void NullcamChainAudioProcessorEditor::paintBitReadout (juce::Graphics& g, const Stage&)
{
    int crushIndex = 0;

    for (std::size_t i = 0; i < knobs.size(); ++i)
        if (knobs[i]->paramId == ids::crush)
            crushIndex = (int) i;

    const auto crush = juce::jlimit (0.0f, 1.0f, currentValue ((std::size_t) crushIndex));
    const auto index = juce::jlimit (0, 6, (int) std::floor (crush * 6.0f + 0.0001f));
    const auto bits = nullcam::Chain::kBitLadder[index];
    const auto hold = crush > 0.3f ? 1 + (int) std::lround (crush * 7.0f) : 1;

    const auto screen = bitReadoutArea.toFloat();

    g.setColour (juce::Colour (0xff08080b));
    g.fillRoundedRectangle (screen, 3.0f);
    g.setColour (PierLookAndFeel::acid().withAlpha (0.22f));
    g.drawRoundedRectangle (screen.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (PierLookAndFeel::acid().withAlpha (0.05f));
    g.drawRoundedRectangle (screen.reduced (3.5f), 2.0f, 1.0f);

    g.setFont (lookAndFeel.shadeFont (48.0f));
    g.setColour (PierLookAndFeel::acid().withAlpha (0.35f));
    g.drawText (juce::String (juce::roundToInt (bits)),
                bitReadoutArea.withTrimmedTop (22).withHeight (58).translated (2, 2),
                juce::Justification::centred, false);
    g.setColour (PierLookAndFeel::acid());
    g.drawText (juce::String (juce::roundToInt (bits)),
                bitReadoutArea.withTrimmedTop (22).withHeight (58),
                juce::Justification::centred, false);

    g.setFont (lookAndFeel.monoFont (8.0f, true));
    g.setColour (PierLookAndFeel::dim());
    drawTracked (g, "BIT", bitReadoutArea.withTrimmedTop (82).withHeight (10), 0.34f,
                 juce::Justification::centred);

    g.setColour (PierLookAndFeel::dim().withAlpha (0.8f));
    drawTracked (g, hold > 1 ? "HOLD x" + juce::String (hold) : "NO HOLD",
                 bitReadoutArea.withTrimmedTop (96).withHeight (10), 0.16f,
                 juce::Justification::centred);

    // the little level ladder along the bottom of the screen
    const auto bars = 7;
    const auto barW = 5;
    const auto gap = 3;
    const auto totalW = bars * barW + (bars - 1) * gap;
    auto bx = screen.getCentreX() - (float) totalW * 0.5f;
    const auto by = screen.getBottom() - 14.0f;

    for (int i = 0; i < bars; ++i)
    {
        g.setColour (i <= index ? PierLookAndFeel::acid() : PierLookAndFeel::line());
        g.fillRect (bx, by, (float) barW, 5.0f);
        bx += (float) (barW + gap);
    }
}

void NullcamChainAudioProcessorEditor::paintBaseplate (juce::Graphics& g)
{
    const auto area = juce::Rectangle<int> (0, getHeight() - kBaseH, getWidth(), kBaseH);

    if (area.isEmpty())
        return;

    g.setColour (PierLookAndFeel::paper().darker (0.35f));
    g.fillRect (area);
    g.setColour (PierLookAndFeel::line());
    g.fillRect (area.getX(), area.getY(), area.getWidth(), 1);

    // The wordmark as a printed band along the bottom: the same font as the header,
    // used as artwork rather than as a label.
    const juce::String mark ("PIER-MODE");

    juce::Graphics::ScopedSaveState clip (g);
    g.reduceClipRegion (area);

    g.setFont (lookAndFeel.shadeFont (48.0f));
    g.setColour (PierLookAndFeel::acid().withAlpha (0.42f));
    g.drawText (mark, area.translated (2, 2), juce::Justification::centred, false);
    g.setColour (PierLookAndFeel::cream().withAlpha (0.30f));
    g.drawText (mark, area, juce::Justification::centred, false);
}

void NullcamChainAudioProcessorEditor::paintScrews (juce::Graphics& g)
{
    const juce::Point<float> corners[] = {
        { 14.0f, 17.0f },
        { (float) getWidth() - 14.0f, 17.0f },
        { 14.0f, (float) getHeight() - 14.0f },
        { (float) getWidth() - 14.0f, (float) getHeight() - 14.0f }
    };

    for (auto p : corners)
    {
        juce::ColourGradient metal (juce::Colour (0xff3b3b47), p.x - 4.0f, p.y - 4.0f,
                                    juce::Colour (0xff14141a), p.x + 4.0f, p.y + 4.0f, false);
        g.setGradientFill (metal);
        g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);

        g.setColour (juce::Colour (0xff09090c));
        g.drawEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f, 1.0f);

        g.setColour (juce::Colour (0xff06060a));
        g.drawLine (p.x - 2.8f, p.y + 2.8f, p.x + 2.8f, p.y - 2.8f, 1.3f);
    }
}

void NullcamChainAudioProcessorEditor::resized()
{
    headerArea = { kMargin, 18, getWidth() - 2 * kMargin, kHeaderH };
    presetBarArea = { kMargin, headerArea.getBottom() + kGap, getWidth() - 2 * kMargin, kPresetBarH };

    presetPlateArea = { presetBarArea.getX() + 54, presetBarArea.getY(), 200, kPresetBarH };
    presetBox.setBounds (presetPlateArea.getRight() + 10, presetBarArea.getY(), 154, kPresetBarH);
    saveButton.setBounds (presetBox.getRight() + 10, presetBarArea.getY(), 62, kPresetBarH);
    deleteButton.setBounds (saveButton.getRight() + 8, presetBarArea.getY(), 54, kPresetBarH);

    const auto stageTop = presetBarArea.getBottom() + kGap;
    const auto stageCount = (int) stages.size();

    if (stageCount == 0)
        return;

    const auto stageW = (presetBarArea.getWidth() - (stageCount - 1) * kStageGap) / stageCount;
    const auto stageH = kStageTitleH + 3 * kKnobCellH + kStageCaptionH + 6;

    for (int s = 0; s < stageCount; ++s)
    {
        auto& stage = stages[(std::size_t) s];
        stage.area = { presetBarArea.getX() + s * (stageW + kStageGap), stageTop, stageW, stageH };

        const auto top = stage.area.getY() + kStageTitleH;
        const auto areaH = stage.area.getHeight() - kStageTitleH - kStageCaptionH - 6;
        const auto count = (int) stage.knobIndices.size();

        // A stage with two knobs spreads them over the same three rows a stage with
        // three knobs uses, so the cards stay the same height and the rows still line
        // up between neighbours.
        const auto step = count > 1 ? (areaH - kKnobCellH) / (count - 1) : 0;

        auto y = top;

        for (int i = 0; i < count; ++i)
        {
            auto& knob = *knobs[(std::size_t) stage.knobIndices[(std::size_t) i]];

            knob.cell = { stage.area.getX() + 4, y, stage.area.getWidth() - 8, kKnobCellH - 2 };

            const auto sliderW = juce::jmin (knob.cell.getWidth() - 6, 84);
            knob.slider->setBounds (knob.cell.getCentreX() - sliderW / 2, knob.cell.getY() + 12,
                                    sliderW, kKnobCellH - 16);

            y += step;
        }

        // One cell further than the last knob, whatever the step was.
        const auto contentBottom = top + (count - 1) * step + kKnobCellH;

        if (stage.showsBitDepth)
        {
            // The screen takes whatever is left under the knob, centred in it.
            const auto freeTop = contentBottom + 8;
            const auto freeBottom = stage.area.getBottom() - kStageCaptionH - 8;
            const auto screenH = juce::jmin (140, freeBottom - freeTop);
            const auto screenY = freeTop + (freeBottom - freeTop - screenH) / 2;

            bitReadoutArea = { stage.area.getX() + 8, screenY, stage.area.getWidth() - 16, screenH };
        }
    }
}

//==============================================================================
void NullcamChainAudioProcessorEditor::refreshPresetList()
{
    userPresets = processor.getUserPresetNames();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");
    presetBox.addItem ("MOTION", kMotionItem);
    presetBox.addItem ("SORT", kSortItem);
    presetBox.addItem ("CLEAN", kCleanItem);

    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], kUserItemFirst + i);
    }

    presetBox.setSelectedId (0, juce::dontSendNotification);
    deleteButton.setEnabled (userPresets.contains (presetName));
}

void NullcamChainAudioProcessorEditor::choosePreset (int itemId)
{
    if (itemId <= 0)
        return;

    // Always go back to "LOAD PRESET", so choosing the same entry twice still works.
    juce::Component::SafePointer<NullcamChainAudioProcessorEditor> safe (this);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe != nullptr)
            safe->presetBox.setSelectedId (0, juce::dontSendNotification);
    });

    if (itemId == kMotionItem)
    {
        processor.applyPreset (NullcamChainAudioProcessor::Preset::motion);
        setPresetName ("MOTION");
    }
    else if (itemId == kSortItem)
    {
        processor.applyPreset (NullcamChainAudioProcessor::Preset::sort);
        setPresetName ("SORT");
    }
    else if (itemId == kCleanItem)
    {
        processor.applyPreset (NullcamChainAudioProcessor::Preset::clean);
        setPresetName ("CLEAN");
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

void NullcamChainAudioProcessorEditor::setPresetName (const juce::String& name)
{
    presetName = name;
    edited = false;

    for (std::size_t i = 0; i < knobs.size(); ++i)
        referenceValues[i] = currentValue (i);

    deleteButton.setEnabled (userPresets.contains (name));
    repaint();
}

void NullcamChainAudioProcessorEditor::updateEditedState()
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

void NullcamChainAudioProcessorEditor::promptSavePreset()
{
    if (activeDialog != nullptr)
        return;

    auto suggested = presetName;

    if (suggested.isEmpty() || userPresets.contains (suggested))
        suggested = suggested.isEmpty() ? "MY PRESET" : suggested;
    else
        suggested = suggested + " COPY";

    auto* dialog = new juce::AlertWindow ("SAVE PRESET",
                                          "The preset is written to\n"
                                              + NullcamChainAudioProcessor::getUserPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", suggested, "NAME");
    dialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    activeDialog = dialog;

    juce::Component::SafePointer<NullcamChainAudioProcessorEditor> safe (this);

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

void NullcamChainAudioProcessorEditor::promptDeletePreset()
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

    juce::Component::SafePointer<NullcamChainAudioProcessorEditor> safe (this);

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

float NullcamChainAudioProcessorEditor::currentValue (std::size_t index) const
{
    if (auto* value = processor.apvts.getRawParameterValue (knobs[index]->paramId))
        return value->load();

    return 0.0f;
}

void NullcamChainAudioProcessorEditor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate();
}

void NullcamChainAudioProcessorEditor::handleAsyncUpdate()
{
    updateEditedState();

    // the bit depth readout in the CRUSH stage follows the knob
    repaint (bitReadoutArea);
}
