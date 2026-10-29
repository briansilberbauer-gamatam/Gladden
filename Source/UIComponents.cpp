#include "UIComponents.h"
#include "PluginProcessor.h"

#include <cmath>

namespace synthwave
{
//==============================================================================
void AttachedKnob::configure (juce::AudioProcessorValueTreeState& state,
                              const juce::String& parameterId,
                              const juce::String& displayName,
                              juce::Colour accent,
                              bool showValue)
{
    accentColour = accent;

    nameLabel.setText (displayName.toUpperCase(), juce::dontSendNotification);
    nameLabel.setFont (CustomLookAndFeel::monoFont (8.8f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Theme::textMuted);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                juce::MathConstants<float>::pi * 2.80f, true);
    slider.setTextBoxStyle (showValue ? juce::Slider::TextBoxBelow
                                      : juce::Slider::NoTextBox,
                            false, showValue ? 66 : 0, showValue ? 12 : 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId,
                      showValue ? Theme::textMuted : juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setPopupDisplayEnabled (true, false, this);
    slider.setMouseDragSensitivity (160);
    slider.setRepaintsOnMouseActivity (true);
    slider.setTooltip (displayName);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, parameterId, slider);
}

void AttachedKnob::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds (bounds.removeFromTop (juce::jmin (12, bounds.getHeight() / 4)));
    slider.setBounds (bounds);
}

void AttachedKnob::paint (juce::Graphics& g)
{
    g.setColour (accentColour.withAlpha (0.8f));
    g.fillEllipse (2.0f, 5.0f, 2.0f, 2.0f);
}

//==============================================================================
void AttachedChoice::configure (juce::AudioProcessorValueTreeState& state,
                                const juce::String& parameterId,
                                const juce::String& displayName,
                                juce::Colour accent)
{
    nameLabel.setText (displayName.toUpperCase(), juce::dontSendNotification);
    nameLabel.setFont (CustomLookAndFeel::monoFont (8.8f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Theme::textMuted);
    nameLabel.setJustificationType (juce::Justification::centredLeft);
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    combo.setJustificationType (juce::Justification::centredLeft);
    combo.setColour (juce::ComboBox::arrowColourId, accent);
    combo.setColour (juce::ComboBox::focusedOutlineColourId, accent);
    combo.setTooltip (displayName);
    addAndMakeVisible (combo);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (parameterId)))
        combo.addItemList (parameter->choices, 1);
    else
        jassertfalse;

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        state, parameterId, combo);
}

void AttachedChoice::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds (bounds.removeFromTop (11));
    combo.setBounds (bounds.removeFromTop (juce::jmin (23, bounds.getHeight())));
}

//==============================================================================
void AttachedFader::configure (juce::AudioProcessorValueTreeState& state,
                               const juce::String& parameterId,
                               const juce::String& displayName,
                               juce::Colour accent)
{
    nameLabel.setText (displayName.toUpperCase(), juce::dontSendNotification);
    nameLabel.setFont (CustomLookAndFeel::monoFont (8.4f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Theme::textMuted);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.7f));
    slider.setPopupDisplayEnabled (true, false, this);
    slider.setRepaintsOnMouseActivity (true);
    slider.setTooltip (displayName);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, parameterId, slider);
}

void AttachedFader::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds (bounds.removeFromTop (13));
    slider.setBounds (bounds);
}

//==============================================================================
void AttachedToggle::configure (juce::AudioProcessorValueTreeState& state,
                                const juce::String& parameterId,
                                const juce::String& displayName,
                                juce::Colour accent)
{
    toggle.setButtonText (displayName);
    toggle.setColour (juce::ToggleButton::textColourId, Theme::textMuted);
    toggle.setColour (juce::ToggleButton::tickColourId, accent);
    toggle.setTooltip (displayName);
    addAndMakeVisible (toggle);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        state, parameterId, toggle);
}

void AttachedToggle::resized()
{
    toggle.setBounds (getLocalBounds());
}

//==============================================================================
SectionPanel::SectionPanel (juce::String titleText, juce::String subtitleText,
                            juce::Colour accentColour)
    : title (std::move (titleText)),
      subtitle (std::move (subtitleText)),
      accent (accentColour)
{
    setInterceptsMouseClicks (false, true);
}

void SectionPanel::paint (juce::Graphics& g)
{
    CustomLookAndFeel::drawGlassPanel (g, getLocalBounds().toFloat().reduced (1.0f), accent);
    auto titleArea = getLocalBounds().reduced (10).removeFromTop (18);
    CustomLookAndFeel::drawPanelTitle (g, titleArea, title, subtitle, accent);
}

juce::Rectangle<int> SectionPanel::getContentBounds() const
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (20);
    return bounds;
}

//==============================================================================
LevelMeter::LevelMeter (SynthwaveLegacyProcessor& processor)
    : audioProcessor (processor)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    const auto incomingL = juce::jlimit (0.0f, 2.0f, audioProcessor.getPeakLevel (0));
    const auto incomingR = juce::jlimit (0.0f, 2.0f, audioProcessor.getPeakLevel (1));

    displayedLeft = incomingL > displayedLeft ? incomingL
                                              : displayedLeft * 0.87f;
    displayedRight = incomingR > displayedRight ? incomingR
                                                : displayedRight * 0.87f;

    if (incomingL > holdLeft)
    {
        holdLeft = incomingL;
        holdCounter = 30;
    }
    if (incomingR > holdRight)
    {
        holdRight = incomingR;
        holdCounter = 30;
    }

    if (--holdCounter <= 0)
    {
        holdLeft *= 0.985f;
        holdRight *= 0.985f;
    }

    if (displayedLeft < 1.0e-5f) displayedLeft = 0.0f;
    if (displayedRight < 1.0e-5f) displayedRight = 0.0f;

    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty())
        return;

    const auto drawRow = [&g] (float x, float y, float width, float height,
                               const juce::String& label, float level, float hold)
    {
        g.setColour (Theme::textDim);
        g.setFont (CustomLookAndFeel::monoFont (8.0f, juce::Font::bold));
        g.drawText (label, juce::Rectangle<float> (x, y - 2.0f, 8.0f, height + 4.0f),
                    juce::Justification::centredLeft, false);

        auto bar = juce::Rectangle<float> (x + 10.0f, y, width - 18.0f, height);
        g.setColour (Theme::panelInset);
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (Theme::line.withAlpha (0.6f));
        g.drawRoundedRectangle (bar, 2.0f, 0.7f);

        const auto db = juce::Decibels::gainToDecibels (juce::jmax (1.0e-5f, level), -60.0f);
        const auto normalised = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
        constexpr int numSegments = 32;
        const auto segmentGap = 1.0f;
        const auto segmentWidth = (bar.getWidth() - 2.0f) / (float) numSegments;

        for (int i = 0; i < numSegments; ++i)
        {
            const auto segmentNorm = (float) (i + 1) / (float) numSegments;
            auto segment = juce::Rectangle<float> (
                bar.getX() + 1.0f + (float) i * segmentWidth,
                bar.getY() + 1.0f,
                juce::jmax (1.0f, segmentWidth - segmentGap),
                bar.getHeight() - 2.0f);

            juce::Colour colour = Theme::line.withAlpha (0.34f);
            if (segmentNorm <= normalised)
            {
                if (segmentNorm > 0.94f)
                    colour = Theme::red;
                else if (segmentNorm > 0.78f)
                    colour = Theme::amber;
                else
                    colour = Theme::teal;
            }

            g.setColour (colour);
            g.fillRect (segment);
        }

        if (level >= 0.999f)
        {
            g.setColour (Theme::red);
            g.fillEllipse (bar.getRight() - 2.0f, bar.getY() + 1.0f, 4.0f, 4.0f);
        }

        const auto holdNorm = juce::jlimit (0.0f, 1.0f, juce::jmap (
            juce::Decibels::gainToDecibels (juce::jmax (1.0e-5f, hold), -60.0f),
            -60.0f, 0.0f, 0.0f, 1.0f));
        if (holdNorm > 0.01f)
        {
            g.setColour (Theme::text.withAlpha (0.45f));
            g.fillRect (bar.getX() + 1.0f + holdNorm * (bar.getWidth() - 2.0f),
                        bar.getY() + 1.0f, 1.0f, bar.getHeight() - 2.0f);
        }
    };

    const auto rowHeight = (bounds.getHeight() - 6.0f) * 0.5f;
    drawRow (bounds.getX(), bounds.getY() + 2.0f, bounds.getWidth(), rowHeight, "L",
             displayedLeft, holdLeft);
    drawRow (bounds.getX(), bounds.getY() + 5.0f + rowHeight, bounds.getWidth(), rowHeight, "R",
             displayedRight, holdRight);
}

//==============================================================================
ScopeDisplay::ScopeDisplay (SynthwaveLegacyProcessor& processor)
    : audioProcessor (processor)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ScopeDisplay::timerCallback()
{
    const auto newSamples = audioProcessor.readScopeSamples (samples.data(), (int) samples.size());
    if (newSamples > 0)
    {
        numSamples = newSamples;
        repaint();
    }
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat().reduced (1.0f);
    CustomLookAndFeel::drawGlassPanel (g, panel, Theme::teal, true, 6.0f);

    auto titleArea = getLocalBounds().reduced (10).removeFromTop (16);
    CustomLookAndFeel::drawPanelTitle (g, titleArea, "Signal Monitor", "", Theme::teal);

    auto content = getLocalBounds().toFloat().reduced (10.0f);
    content.removeFromTop (19.0f);

    g.setColour (Theme::panelInset.withAlpha (0.80f));
    g.fillRoundedRectangle (content, 3.0f);

    // Grid.
    g.setColour (Theme::line.withAlpha (0.45f));
    for (int i = 1; i < 8; ++i)
    {
        const auto x = content.getX() + content.getWidth() * (float) i / 8.0f;
        g.drawVerticalLine ((int) x, content.getY() + 3.0f, content.getBottom() - 3.0f);
    }
    for (int i = 1; i < 4; ++i)
    {
        const auto y = content.getY() + content.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) y, content.getX() + 3.0f, content.getRight() - 3.0f);
    }

    g.setColour (Theme::lineBright.withAlpha (0.28f));
    g.drawHorizontalLine ((int) content.getCentreY(), content.getX() + 3.0f,
                          content.getRight() - 3.0f);

    g.saveState();
    g.reduceClipRegion (content.toNearestInt());

    if (numSamples > 2)
    {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (samples[(size_t) i]));

        const auto gain = juce::jlimit (1.0f, 3.2f,
                                        0.46f * content.getHeight() / juce::jmax (0.04f, peak));

        juce::Path waveform;
        juce::Path filled;
        const auto xStep = content.getWidth() / (float) juce::jmax (1, numSamples - 1);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = content.getX() + xStep * (float) i;
            const auto sample = juce::jlimit (-1.0f, 1.0f, samples[(size_t) i] * gain);
            const auto y = content.getCentreY() - sample * content.getHeight() * 0.46f;

            if (i == 0)
            {
                waveform.startNewSubPath (x, y);
                filled.startNewSubPath (x, content.getCentreY());
                filled.lineTo (x, y);
            }
            else
            {
                waveform.lineTo (x, y);
                filled.lineTo (x, y);
            }
        }

        filled.lineTo (content.getRight(), content.getCentreY());
        filled.closeSubPath();

        juce::ColourGradient fillGradient (Theme::teal.withAlpha (0.16f),
                                           content.getCentreX(), content.getY(),
                                           Theme::teal.withAlpha (0.0f),
                                           content.getCentreX(), content.getBottom(), false);
        g.setGradientFill (fillGradient);
        g.fillPath (filled);

        g.setColour (Theme::teal.withAlpha (0.08f));
        g.strokePath (waveform, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        g.setColour (Theme::teal.withAlpha (0.22f));
        g.strokePath (waveform, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        g.setColour (Theme::teal);
        g.strokePath (waveform, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        const auto db = juce::Decibels::gainToDecibels (juce::jmax (1.0e-5f, peak), -60.0f);
        g.setColour (Theme::textDim);
        g.setFont (CustomLookAndFeel::monoFont (7.5f));
        g.drawText (juce::String (db, 1) + " dB", content.reduced (4.0f).removeFromBottom (10.0f),
                    juce::Justification::centredRight, false);
    }
    else
    {
        g.setColour (Theme::teal.withAlpha (0.22f));
        g.drawHorizontalLine ((int) content.getCentreY(),
                              content.getX() + 4.0f, content.getRight() - 4.0f);
    }

    g.restoreState();

    // Live indicator.
    const auto phase = (float) (juce::Time::getMillisecondCounterHiRes() * 0.002);
    const auto pulse = 0.45f + 0.55f * (0.5f + 0.5f * std::sin (phase));
    g.setColour (Theme::teal.withAlpha (0.15f * pulse));
    g.fillEllipse (panel.getRight() - 26.0f, panel.getY() + 10.0f, 10.0f, 10.0f);
    g.setColour (Theme::teal.withAlpha (0.75f * pulse));
    g.fillEllipse (panel.getRight() - 23.0f, panel.getY() + 13.0f, 4.0f, 4.0f);
    g.setColour (Theme::textDim);
    g.setFont (CustomLookAndFeel::monoFont (7.0f));
    g.drawText ("LIVE", juce::Rectangle<float> (panel.getRight() - 18.0f,
                                                panel.getY() + 7.0f, 16.0f, 12.0f),
                juce::Justification::centredLeft, false);
}

} // namespace synthwave
