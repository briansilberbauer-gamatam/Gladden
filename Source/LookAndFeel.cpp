#include "LookAndFeel.h"

#include <cmath>

namespace synthwave
{
namespace
{
float effectiveKnobHeight (const juce::Slider& slider, int height)
{
    const auto pos = slider.getTextBoxPosition();
    if (pos == juce::Slider::TextBoxBelow || pos == juce::Slider::TextBoxAbove)
        return (float) juce::jmax (1, height - slider.getTextBoxHeight());

    return (float) height;
}

juce::Colour accentFor (const juce::Slider& slider)
{
    auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    if (accent.isTransparent())
        accent = Theme::teal;

    if (! slider.isEnabled())
        accent = accent.withSaturation (0.15f).withBrightness (0.35f);

    return accent;
}
} // namespace

//==============================================================================
CustomLookAndFeel::CustomLookAndFeel()
{
    setColourScheme ({ Theme::background, Theme::panel, Theme::panel,
                       Theme::line, Theme::text, Theme::tealDim,
                       Theme::teal, Theme::panelRaised, Theme::text });

    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::DocumentWindow::backgroundColourId, Theme::background);
    setColour (juce::Label::textColourId, Theme::text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textWhenEditingColourId, Theme::teal);
    setColour (juce::Label::backgroundWhenEditingColourId, Theme::panelInset);

    setColour (juce::TextButton::buttonColourId, Theme::panelRaised);
    setColour (juce::TextButton::buttonOnColourId, Theme::teal.withAlpha (0.20f));
    setColour (juce::TextButton::textColourOffId, Theme::textMuted);
    setColour (juce::TextButton::textColourOnId, Theme::teal);

    setColour (juce::ToggleButton::textColourId, Theme::textMuted);
    setColour (juce::ToggleButton::tickColourId, Theme::teal);
    setColour (juce::ToggleButton::tickDisabledColourId, Theme::textDim);

    setColour (juce::Slider::thumbColourId, Theme::teal);
    setColour (juce::Slider::trackColourId, Theme::tealDim);
    setColour (juce::Slider::backgroundColourId, Theme::panelInset);
    setColour (juce::Slider::rotarySliderFillColourId, Theme::teal);
    setColour (juce::Slider::rotarySliderOutlineColourId, Theme::line);
    setColour (juce::Slider::textBoxTextColourId, Theme::textMuted);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Theme::teal.withAlpha (0.25f));

    setColour (juce::ComboBox::backgroundColourId, Theme::panelInset);
    setColour (juce::ComboBox::textColourId, Theme::text);
    setColour (juce::ComboBox::outlineColourId, Theme::line);
    setColour (juce::ComboBox::buttonColourId, Theme::panelInset);
    setColour (juce::ComboBox::arrowColourId, Theme::teal);
    setColour (juce::ComboBox::focusedOutlineColourId, Theme::teal);

    setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::headerTextColourId, Theme::textMuted);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::teal.withAlpha (0.13f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::teal);

    setColour (juce::TabbedButtonBar::tabTextColourId, Theme::textMuted);
    setColour (juce::TabbedButtonBar::frontTextColourId, Theme::teal);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::frontOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedComponent::backgroundColourId, Theme::panel);
    setColour (juce::TabbedComponent::outlineColourId, Theme::line);

    setColour (juce::TextEditor::backgroundColourId, Theme::panelInset);
    setColour (juce::TextEditor::textColourId, Theme::text);
    setColour (juce::TextEditor::outlineColourId, Theme::line);
    setColour (juce::TextEditor::focusedOutlineColourId, Theme::teal);
    setColour (juce::TextEditor::highlightColourId, Theme::teal.withAlpha (0.22f));

    setColour (juce::ScrollBar::thumbColourId, Theme::lineBright);
    setColour (juce::ScrollBar::trackColourId, Theme::panelInset);

}

//==============================================================================
juce::Font CustomLookAndFeel::sansFont (float height, int style)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), height, style);
}

juce::Font CustomLookAndFeel::monoFont (float height, int style)
{
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), height, style);
}

void CustomLookAndFeel::drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> area,
                                        juce::Colour accent, bool raised, float cornerSize)
{
    if (area.isEmpty())
        return;

    const auto top = (raised ? Theme::panelRaised : Theme::panel)
                         .interpolatedWith (accent, 0.035f);
    const auto bottom = Theme::panelInset.interpolatedWith (accent, 0.012f);

    juce::ColourGradient gradient (top, area.getX(), area.getY(),
                                   bottom, area.getX(), area.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (area, cornerSize);

    // Subtle interior top highlight.
    g.setColour (juce::Colours::white.withAlpha (raised ? 0.06f : 0.035f));
    g.drawHorizontalLine ((int) area.getY() + 1, area.getX() + 9.0f, area.getRight() - 9.0f);

    g.setColour (accent.withAlpha (raised ? 0.34f : 0.20f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.0f);

    // A faint inner hairline adds the glassy edge without a heavy border.
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawRoundedRectangle (area.reduced (2.5f), juce::jmax (2.0f, cornerSize - 1.0f), 0.6f);
}

void CustomLookAndFeel::drawPanelTitle (juce::Graphics& g, juce::Rectangle<int> area,
                                        const juce::String& title,
                                        const juce::String& subtitle,
                                        juce::Colour accent)
{
    auto bar = juce::Rectangle<float> ((float) area.getX() + 1.0f,
                                      (float) area.getY() + area.getHeight() * 0.28f,
                                       2.0f,
                                       area.getHeight() * 0.44f);
    g.setColour (accent.withAlpha (0.20f));
    g.fillRoundedRectangle (bar.expanded (2.0f, 1.0f), 2.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (bar, 1.0f);

    g.setColour (Theme::text);
    g.setFont (monoFont (11.5f, juce::Font::bold));
    const auto textArea = area.withTrimmedLeft (9);
    g.drawText (title.toUpperCase(), textArea, juce::Justification::centredLeft, false);

    if (subtitle.isNotEmpty())
    {
        g.setColour (Theme::textDim);
        g.setFont (monoFont (8.5f));
        g.drawText (subtitle.toUpperCase(), textArea, juce::Justification::centredRight, false);
    }
}

//==============================================================================
juce::Font CustomLookAndFeel::getLabelFont (juce::Label& label)
{
    return sansFont (juce::jlimit (9.0f, 18.0f, (float) label.getHeight() * 0.58f));
}

juce::Font CustomLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return monoFont (juce::jlimit (9.0f, 13.0f, (float) box.getHeight() * 0.38f));
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return monoFont (11.5f);
}

juce::Font CustomLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return monoFont (juce::jlimit (9.0f, 13.0f, (float) buttonHeight * 0.40f), juce::Font::bold);
}

void CustomLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 0, box.getWidth() - 26, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

//==============================================================================
void CustomLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                      bool isButtonDown, int buttonX, int buttonY,
                                      int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH);

    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    const auto accent = box.findColour (juce::ComboBox::arrowColourId);

    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (bounds, 3.5f);

    g.setColour (box.isMouseOver() || isButtonDown
                     ? accent.withAlpha (0.65f)
                     : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 3.5f, 1.0f);

    auto arrowArea = juce::Rectangle<float> ((float) width - 19.0f, 0.0f, 12.0f, (float) height);
    juce::Path arrow;
    arrow.startNewSubPath (arrowArea.getCentreX() - 4.0f, arrowArea.getCentreY() - 2.0f);
    arrow.lineTo (arrowArea.getCentreX(), arrowArea.getCentreY() + 3.0f);
    arrow.lineTo (arrowArea.getCentreX() + 4.0f, arrowArea.getCentreY() - 2.0f);
    g.setColour (accent.withAlpha (box.isEnabled() ? 0.90f : 0.35f));
    g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

//==============================================================================
void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                          int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto knobHeight = effectiveKnobHeight (slider, height);
    auto knobY = (float) y;
    if (slider.getTextBoxPosition() == juce::Slider::TextBoxAbove)
        knobY += (float) slider.getTextBoxHeight();

    auto area = juce::Rectangle<float> ((float) x, knobY, (float) width, knobHeight).reduced (3.0f);
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 5.0f;
    if (radius <= 2.0f)
        return;

    const auto centre = area.getCentre();
    const auto accent = accentFor (slider);
    const auto angle = rotaryStartAngle
                     + juce::jlimit (0.0f, 1.0f, sliderPosProportional)
                       * (rotaryEndAngle - rotaryStartAngle);
    const auto ringRadius = radius - 2.0f;

    // Outer ambience.
    g.setColour (accent.withAlpha (slider.isMouseOver() ? 0.10f : 0.045f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f + 12.0f, radius * 2.0f + 12.0f)
                       .withCentre (centre));

    // Main knob body.
    juce::ColourGradient body (Theme::panelRaised.brighter (0.06f),
                               centre.x - radius * 0.5f, centre.y - radius * 0.65f,
                               Theme::panelInset.darker (0.20f),
                               centre.x + radius * 0.35f, centre.y + radius * 0.8f, false);
    body.addColour (0.55, Theme::panel.brighter (0.03f));
    g.setGradientFill (body);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (Theme::lineBright.withAlpha (0.45f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.8f);

    // Background value arc.
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Theme::panelInset);
    g.strokePath (backgroundArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

    // Tick marks around the arc.
    constexpr int tickCount = 9;
    for (int i = 0; i < tickCount; ++i)
    {
        const auto tickAngle = rotaryStartAngle
                             + (rotaryEndAngle - rotaryStartAngle)
                               * (float) i / (float) (tickCount - 1);
        const auto inner = centre.getPointOnCircumference (ringRadius - 5.0f, tickAngle);
        const auto outer = centre.getPointOnCircumference (ringRadius - 1.5f, tickAngle);
        g.setColour (Theme::lineBright.withAlpha (i == 0 ? 0.25f : 0.40f));
        g.drawLine ({ inner, outer }, 0.8f);
    }

    if (angle > rotaryStartAngle + 0.005f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                                rotaryStartAngle, angle, true);

        g.setColour (accent.withAlpha (0.10f));
        g.strokePath (valueArc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (valueArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Pointer.
    const auto pointerInner = centre.getPointOnCircumference (radius * 0.28f, angle);
    const auto pointerOuter = centre.getPointOnCircumference (radius * 0.72f, angle);
    g.setColour (accent.withAlpha (0.18f));
    g.drawLine ({ pointerInner, pointerOuter }, 5.0f);
    g.setColour (accent);
    g.drawLine ({ pointerInner, pointerOuter }, 1.8f);

    // Hub and highlight.
    g.setColour (Theme::panelInset);
    g.fillEllipse (juce::Rectangle<float> (radius * 0.38f, radius * 0.38f).withCentre (centre));
    g.setColour (accent.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (radius * 0.38f, radius * 0.38f).withCentre (centre), 0.8f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.75f, radius * 0.34f)
                       .withCentre ({ centre.x, centre.y - radius * 0.48f }));
}

void CustomLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y,
                                          int width, int height,
                                          float sliderPos, float minSliderPos,
                                          float maxSliderPos,
                                          juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    const auto accent = slider.findColour (juce::Slider::thumbColourId);
    const auto isEnabled = slider.isEnabled();

    if (style == juce::Slider::LinearVertical)
    {
        const auto centreX = (float) x + (float) width * 0.5f;
        const auto top = (float) y + 7.0f;
        const auto bottom = (float) (y + height) - 7.0f;
        if (bottom <= top)
            return;

        const auto trackWidth = 4.0f;
        auto track = juce::Rectangle<float> (trackWidth, bottom - top)
                         .withCentre ({ centreX, (top + bottom) * 0.5f });
        g.setColour (Theme::panelInset);
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (Theme::line.withAlpha (0.75f));
        g.drawRoundedRectangle (track.reduced (0.5f), 2.0f, 0.8f);

        const auto filledTop = juce::jlimit (top, bottom, sliderPos);
        if (bottom - filledTop > 0.5f)
        {
            auto fill = juce::Rectangle<float> (trackWidth, bottom - filledTop)
                            .withCentre ({ centreX, (filledTop + bottom) * 0.5f });
            juce::ColourGradient glow (accent.withAlpha (0.35f), centreX, bottom,
                                       accent, centreX, filledTop, false);
            g.setGradientFill (glow);
            g.fillRoundedRectangle (fill, 2.0f);

            g.setColour (accent.withAlpha (0.10f));
            g.fillRoundedRectangle (fill.expanded (3.0f, 0.0f), 4.0f);
        }

        // Scale ticks.
        for (int i = 0; i <= 4; ++i)
        {
            const auto ty = juce::jmap ((float) i, 0.0f, 4.0f, top, bottom);
            g.setColour (Theme::lineBright.withAlpha (0.30f));
            g.drawHorizontalLine ((int) ty, centreX - 8.0f, centreX - 5.0f);
            g.drawHorizontalLine ((int) ty, centreX + 5.0f, centreX + 8.0f);
        }

        const auto thumbY = juce::jlimit (top, bottom, sliderPos);
        auto thumb = juce::Rectangle<float> ((float) width * 0.68f, 10.0f)
                         .withCentre ({ centreX, thumbY });
        g.setColour (accent.withAlpha (isEnabled ? 0.14f : 0.05f));
        g.fillRoundedRectangle (thumb.expanded (2.5f, 3.5f), 5.0f);

        juce::ColourGradient thumbFill (Theme::panelRaised, thumb.getX(), thumb.getY(),
                                        Theme::panelInset, thumb.getX(), thumb.getBottom(), false);
        g.setGradientFill (thumbFill);
        g.fillRoundedRectangle (thumb, 3.5f);
        g.setColour (accent.withAlpha (isEnabled ? 0.95f : 0.35f));
        g.drawRoundedRectangle (thumb, 3.5f, 1.2f);
        g.drawHorizontalLine ((int) thumbY, thumb.getX() + 3.0f, thumb.getRight() - 3.0f);
        return;
    }

    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        const auto centreY = (float) y + (float) height * 0.5f;
        auto drawableWidth = width;
        auto drawableX = x;

        if (slider.getTextBoxPosition() == juce::Slider::TextBoxRight)
            drawableWidth = juce::jmax (0, width - slider.getTextBoxWidth() - 3);
        else if (slider.getTextBoxPosition() == juce::Slider::TextBoxLeft)
        {
            drawableX += slider.getTextBoxWidth() + 3;
            drawableWidth = juce::jmax (0, width - slider.getTextBoxWidth() - 3);
        }

        const auto left = (float) drawableX + 5.0f;
        const auto right = (float) (drawableX + drawableWidth) - 5.0f;
        if (right <= left)
            return;

        const auto trackHeight = 3.0f;
        auto track = juce::Rectangle<float> (right - left, trackHeight)
                         .withCentre ({ (left + right) * 0.5f, centreY });
        g.setColour (Theme::panelInset);
        g.fillRoundedRectangle (track, 1.5f);

        const auto filledRight = juce::jlimit (left, right, sliderPos);
        auto fill = juce::Rectangle<float> (juce::jmax (0.0f, filledRight - left), trackHeight)
                        .withCentre ({ (left + filledRight) * 0.5f, centreY });
        g.setColour (accent.withAlpha (isEnabled ? 0.85f : 0.30f));
        g.fillRoundedRectangle (fill, 1.5f);
        g.setColour (accent.withAlpha (0.12f));
        g.fillRoundedRectangle (fill.expanded (1.0f, 3.0f), 3.0f);

        const auto thumbX = juce::jlimit (left, right, sliderPos);
        g.setColour (accent.withAlpha (isEnabled ? 0.16f : 0.05f));
        g.fillEllipse (juce::Rectangle<float> (13.0f, 13.0f).withCentre ({ thumbX, centreY }));
        g.setColour (Theme::panelRaised);
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ thumbX, centreY }));
        g.setColour (accent.withAlpha (isEnabled ? 0.95f : 0.35f));
        g.drawEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ thumbX, centreY }), 1.0f);
        return;
    }

    LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                      minSliderPos, maxSliderPos, style, slider);
}

//==============================================================================
void CustomLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto base = backgroundColour;
    if (base.isTransparent())
        base = Theme::panelRaised;

    if (button.getToggleState())
        base = Theme::teal.withAlpha (0.13f);

    if (isHighlighted)
        base = base.brighter (0.10f);
    if (isDown)
        base = base.darker (0.15f);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, 4.0f);

    auto outline = button.getToggleState() ? Theme::teal.withAlpha (0.75f)
                                           : Theme::line;
    if (isHighlighted)
        outline = Theme::teal.withAlpha (0.65f);

    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void CustomLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const auto isOn = button.getToggleState();
    auto accent = button.findColour (juce::ToggleButton::tickColourId);
    if (accent.isTransparent())
        accent = Theme::teal;

    auto switchArea = juce::Rectangle<float> (juce::jmax (0.0f, bounds.getRight() - 28.0f),
                                             bounds.getCentreY() - 7.0f, 26.0f, 14.0f);
    if (button.getButtonText().isEmpty())
        switchArea = switchArea.withCentre (bounds.getCentre());

    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (switchArea, 7.0f);
    g.setColour (isOn ? accent.withAlpha (0.85f) : Theme::lineBright.withAlpha (0.55f));
    g.drawRoundedRectangle (switchArea, 7.0f, 0.9f);

    const auto thumb = juce::Rectangle<float> (10.0f, 10.0f)
                           .withCentre ({ (isOn ? switchArea.getRight() - 7.0f
                                                : switchArea.getX() + 7.0f),
                                          switchArea.getCentreY() });
    g.setColour (isOn ? accent.withAlpha (0.18f) : juce::Colours::transparentBlack);
    g.fillEllipse (thumb.expanded (3.0f));
    g.setColour (isOn ? accent : Theme::textMuted);
    g.fillEllipse (thumb);
    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.drawEllipse (thumb.reduced (0.3f), 0.6f);

    if (button.getButtonText().isNotEmpty())
    {
        g.setColour (isOn ? Theme::text : Theme::textMuted);
        g.setFont (CustomLookAndFeel::monoFont (9.5f, juce::Font::bold));
        g.drawText (button.getButtonText().toUpperCase(),
                    bounds.withTrimmedRight (32.0f).reduced (2.0f, 0.0f),
                    juce::Justification::centredLeft, false);
    }

    if (isHighlighted || isDown)
    {
        g.setColour (accent.withAlpha (isDown ? 0.10f : 0.045f));
        g.fillRoundedRectangle (bounds.reduced (0.5f), 3.0f);
    }
}

//==============================================================================
void CustomLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                       bool isMouseOver, bool isMouseDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const auto isFront = button.isFrontTab();
    auto accent = button.findColour (juce::TabbedButtonBar::frontOutlineColourId);
    if (accent.isTransparent())
        accent = Theme::teal;

    if (isFront)
    {
        g.setColour (Theme::panelRaised);
        g.fillRoundedRectangle (bounds.reduced (2.0f, 2.0f), 3.0f);
        g.setColour (accent.withAlpha (0.16f));
        g.fillRoundedRectangle (bounds.reduced (2.0f, 2.0f), 3.0f);
    }
    else if (isMouseOver || isMouseDown)
    {
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.fillRoundedRectangle (bounds.reduced (2.0f, 2.0f), 3.0f);
    }

    g.setColour (isFront ? accent : Theme::textMuted);
    g.setFont (monoFont (10.5f, juce::Font::bold));
    g.drawText (button.getButtonText().toUpperCase(), bounds, juce::Justification::centred, false);

    if (isFront)
    {
        auto underline = bounds.removeFromBottom (3.0f).reduced (9.0f, 0.0f);
        g.setColour (accent.withAlpha (0.24f));
        g.fillRoundedRectangle (underline.expanded (0.0f, 1.5f), 1.5f);
        g.setColour (accent);
        g.fillRoundedRectangle (underline, 1.3f);
    }
}

void CustomLookAndFeel::drawTabbedButtonBarBackground (juce::TabbedButtonBar& bar,
                                                       juce::Graphics& g)
{
    g.setColour (Theme::panelInset);
    g.fillRect (bar.getLocalBounds());
    g.setColour (Theme::line);
    g.drawHorizontalLine (bar.getHeight() - 1, 0.0f, (float) bar.getWidth());
}

//==============================================================================
void CustomLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (Theme::lineBright.withAlpha (0.65f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem (juce::Graphics& g,
                                           const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive,
                                           bool isHighlighted, bool isTicked,
                                           bool hasSubMenu, const juce::String& text,
                                           const juce::String& shortcutKeyText,
                                           const juce::Drawable* icon,
                                           const juce::Colour* textColour)
{
    juce::ignoreUnused (icon);
    auto bounds = area.toFloat().reduced (2.0f, 1.0f);

    if (isSeparator)
    {
        g.setColour (Theme::line);
        g.drawHorizontalLine (area.getCentreY(), (float) area.getX() + 8.0f,
                              (float) area.getRight() - 8.0f);
        return;
    }

    if (isHighlighted)
    {
        g.setColour (Theme::teal.withAlpha (0.10f));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (Theme::teal.withAlpha (0.20f));
        g.drawRoundedRectangle (bounds, 3.0f, 0.7f);
    }

    auto base = textColour != nullptr ? *textColour : Theme::text;
    if (! isActive)
        base = Theme::textDim;
    else if (isHighlighted)
        base = Theme::teal;

    auto textArea = area.withTrimmedLeft (isTicked ? 28 : 12)
                        .withTrimmedRight (hasSubMenu ? 20 : 8);

    g.setColour (base);
    g.setFont (getPopupMenuFont());
    g.drawText (text, textArea, juce::Justification::centredLeft, true);

    if (isTicked)
    {
        g.setColour (Theme::teal);
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f)
                           .withCentre ({ (float) area.getX() + 13.0f,
                                         (float) area.getCentreY() }));
    }

    if (hasSubMenu)
    {
        juce::Path arrow;
        arrow.startNewSubPath ((float) area.getRight() - 14.0f, (float) area.getCentreY() - 4.0f);
        arrow.lineTo ((float) area.getRight() - 9.0f, (float) area.getCentreY());
        arrow.lineTo ((float) area.getRight() - 14.0f, (float) area.getCentreY() + 4.0f);
        g.setColour (base.withAlpha (0.75f));
        g.strokePath (arrow, juce::PathStrokeType (1.1f));
    }

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (Theme::textDim);
        g.setFont (monoFont (9.0f));
        g.drawText (shortcutKeyText, area.withTrimmedRight (8),
                    juce::Justification::centredRight, false);
    }
}

} // namespace synthwave
