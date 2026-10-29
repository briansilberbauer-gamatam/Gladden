#include "PluginEditor.h"
#include "Presets.h"

#include <cmath>

namespace synthwave
{
namespace
{
constexpr int kTopBarHeight = 84;
constexpr int kKeyboardHeight = 94;
constexpr int kBottomBarHeight = 150;
constexpr int kOuterMargin = 12;
constexpr int kColumnGap = 10;

const juce::Colour groupAccents[] {
    Theme::teal,
    Theme::amber,
    Theme::green,
    juce::Colour (0xffff5a5f)
};

void addToParent (juce::Component& parent, juce::Component& child)
{
    parent.addAndMakeVisible (child);
}

} // namespace

//==============================================================================
SynthKeyboard::SynthKeyboard (juce::MidiKeyboardState& keyboardState)
    : juce::MidiKeyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setAvailableRange (36, 96);
    setLowestVisibleKey (36);
    setScrollButtonsVisible (false);
    setOctaveForMiddleC (4);
    setKeyPressBaseOctave (4);
    setMouseClickGrabsKeyboardFocus (false);
    setVelocity (0.82f, true);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd4e1e6));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0b1116));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff1a242b));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::teal.withAlpha (0.20f));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::teal.withAlpha (0.55f));
    setColour (juce::MidiKeyboardComponent::textLabelColourId, Theme::textDim);
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.30f));
}

void SynthKeyboard::drawWhiteNote (int midiNoteNumber, juce::Graphics& g,
                                   juce::Rectangle<float> area, bool isDown, bool isOver,
                                   juce::Colour lineColour, juce::Colour textColour)
{
    juce::ignoreUnused (lineColour);

    const auto top = isDown ? Theme::teal.withMultipliedBrightness (0.85f)
                            : (isOver ? juce::Colour (0xffe2f0f3) : juce::Colour (0xffcbd9de));
    const auto bottom = isDown ? Theme::teal.darker (0.45f)
                               : juce::Colour (0xff9fb0b7);

    juce::ColourGradient gradient (top, area.getX(), area.getY(),
                                   bottom, area.getX(), area.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRect (area);
    g.setColour (juce::Colour (0xff18232a));
    g.drawRect (area, 0.6f);

    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawHorizontalLine ((int) area.getY() + 1, area.getX() + 1.0f, area.getRight() - 1.0f);

    if (isDown)
    {
        g.setColour (Theme::teal.withAlpha (0.30f));
        g.fillRect (area.reduced (1.0f));
    }

    if (midiNoteNumber % 12 == 0)
    {
        const auto label = juce::MidiMessage::getMidiNoteName (midiNoteNumber, true,
                                                               true, 4);
        g.setColour (isDown ? Theme::background.withAlpha (0.8f) : textColour);
        g.setFont (CustomLookAndFeel::monoFont (7.5f, juce::Font::bold));
        g.drawText (label, area.removeFromBottom (12.0f), juce::Justification::centred, false);
    }
}

void SynthKeyboard::drawBlackNote (int midiNoteNumber, juce::Graphics& g,
                                   juce::Rectangle<float> area, bool isDown, bool isOver,
                                   juce::Colour noteFillColour)
{
    juce::ignoreUnused (noteFillColour, midiNoteNumber);

    const auto top = isDown ? Theme::teal.darker (0.20f)
                            : (isOver ? juce::Colour (0xff283842) : juce::Colour (0xff121b21));
    const auto bottom = juce::Colour (0xff05090c);
    juce::ColourGradient gradient (top, area.getX(), area.getY(),
                                   bottom, area.getX(), area.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (area.reduced (0.4f), 2.0f);

    g.setColour (isDown ? Theme::teal : Theme::lineBright.withAlpha (0.35f));
    g.drawRoundedRectangle (area.reduced (0.5f), 2.0f, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) area.getY() + 2, area.getX() + 3.0f, area.getRight() - 3.0f);

    if (isDown)
    {
        g.setColour (Theme::teal.withAlpha (0.22f));
        g.fillRoundedRectangle (area.reduced (1.0f), 1.5f);
    }
}

//==============================================================================
void PresetComboBox::showPopup()
{
    userSelectionWindow = true;
    popupClosedAtMs = 0;
    juce::ComboBox::showPopup();
    startTimer (80);
}

bool PresetComboBox::consumeUserSelection() noexcept
{
    if (! userSelectionWindow)
        return false;

    userSelectionWindow = false;
    stopTimer();
    return true;
}

void PresetComboBox::timerCallback()
{
    if (isPopupActive())
    {
        popupClosedAtMs = 0;
        return;
    }

    const auto now = juce::Time::getMillisecondCounter();
    if (popupClosedAtMs == 0)
        popupClosedAtMs = now;

    // Give the async ComboBox change message a generous window to be consumed
    // after the popup closes. This avoids treating programmatic state restores
    // as user selections.
    if (now - popupClosedAtMs > 500)
    {
        userSelectionWindow = false;
        stopTimer();
    }
}

//==============================================================================
void BrandPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Angular mark.
    const auto markX = bounds.getX() + 2.0f;
    const auto markY = bounds.getCentreY() - 17.0f;
    juce::Path mark;
    mark.startNewSubPath (markX, markY + 34.0f);
    mark.lineTo (markX + 7.0f, markY);
    mark.lineTo (markX + 23.0f, markY);
    mark.lineTo (markX + 16.0f, markY + 34.0f);
    mark.closeSubPath();
    g.setColour (Theme::teal.withAlpha (0.09f));
    g.fillPath (mark);
    g.setColour (Theme::teal.withAlpha (0.22f));
    g.fillPath (mark);
    g.setColour (Theme::teal);
    g.strokePath (mark, juce::PathStrokeType (1.0f));

    auto textArea = bounds.withTrimmedLeft (34.0f);
    auto titleRow = textArea.removeFromTop (27.0f);

    g.setFont (CustomLookAndFeel::monoFont (19.0f, juce::Font::bold));
    const auto firstWidth = g.getCurrentFont().getStringWidthFloat ("SYNTHWAVE");
    g.setColour (Theme::text);
    g.drawText ("SYNTHWAVE", titleRow.removeFromLeft (firstWidth + 6.0f),
                juce::Justification::centredLeft, false);
    g.setColour (Theme::amber);
    g.drawText ("LEGACY", titleRow, juce::Justification::centredLeft, false);

    g.setColour (Theme::textDim);
    g.setFont (CustomLookAndFeel::monoFont (7.4f));
    g.drawText ("16-VOICE VIRTUAL ANALOG ENGINE  /  GLADDEN AUDIO",
                textArea.removeFromTop (12.0f), juce::Justification::centredLeft, false);

    g.setColour (Theme::line);
    g.fillRect (bounds.getX() + 34.0f, bounds.getBottom() - 5.0f, 108.0f, 1.0f);
    g.setColour (Theme::amber.withAlpha (0.75f));
    g.fillRect (bounds.getX() + 144.0f, bounds.getBottom() - 5.0f, 28.0f, 1.0f);
}

//==============================================================================
EnvelopePreview::EnvelopePreview (juce::AudioProcessorValueTreeState& state,
                                  juce::String attackId, juce::String decayId,
                                  juce::String sustainId, juce::String releaseId,
                                  juce::Colour accent)
    : attack (state.getRawParameterValue (attackId)),
      decay (state.getRawParameterValue (decayId)),
      sustain (state.getRawParameterValue (sustainId)),
      release (state.getRawParameterValue (releaseId)),
      accentColour (accent)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (15);
}

void EnvelopePreview::timerCallback()
{
    repaint();
}

void EnvelopePreview::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Theme::panelInset.withAlpha (0.75f));
    g.fillRoundedRectangle (bounds, 2.5f);
    g.setColour (Theme::line.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds, 2.5f, 0.7f);

    if (attack == nullptr || decay == nullptr || sustain == nullptr || release == nullptr)
        return;

    const auto a = attack->load();
    const auto d = decay->load();
    const auto s = juce::jlimit (0.0f, 1.0f, sustain->load());
    const auto r = release->load();

    auto plot = bounds.reduced (4.0f, 3.0f);
    const auto wa = std::log1p ((double) a * 35.0);
    const auto wd = std::log1p ((double) d * 18.0);
    const auto wh = 0.55;
    const auto wr = std::log1p ((double) r * 18.0);
    const auto total = juce::jmax (1.0e-3, wa + wd + wh + wr);

    const auto x0 = plot.getX();
    const auto x1 = x0 + plot.getWidth() * (float) (wa / total);
    const auto x2 = x1 + plot.getWidth() * (float) (wd / total);
    const auto x3 = x2 + plot.getWidth() * (float) (wh / total);
    const auto x4 = plot.getRight();
    const auto y0 = plot.getBottom();
    const auto yPeak = plot.getY() + 2.0f;
    const auto ySustain = juce::jmap (s, y0, yPeak);

    juce::Path curve;
    curve.startNewSubPath (x0, y0);
    curve.lineTo (x1, yPeak);
    curve.lineTo (x2, ySustain);
    curve.lineTo (x3, ySustain);
    curve.lineTo (x4, y0);

    juce::Path fill (curve);
    fill.lineTo (x0, y0);
    fill.closeSubPath();

    juce::ColourGradient gradient (accentColour.withAlpha (0.25f),
                                   plot.getCentreX(), plot.getY(),
                                   accentColour.withAlpha (0.01f),
                                   plot.getCentreX(), plot.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillPath (fill);

    g.setColour (accentColour.withAlpha (0.18f));
    g.strokePath (curve, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (accentColour);
    g.strokePath (curve, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (accentColour.withAlpha (0.9f));
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre ({ x1, yPeak }));
}

//==============================================================================
OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& state, int oscillatorIndex)
    : SectionPanel (oscillatorIndex == 0 ? "Oscillator 1" : "Oscillator 2",
                    oscillatorIndex == 0 ? "POLYBLEP / SUB" : "POLYBLEP / SUB",
                    oscillatorIndex == 0 ? Theme::teal : Theme::amber)
{
    const auto first = oscillatorIndex == 0;
    const auto accentColour = first ? Theme::teal : Theme::amber;

    waveChoice.configure (state, first ? params::osc1Wave : params::osc2Wave, "Wave", accentColour);
    octaveChoice.configure (state, first ? params::osc1Octave : params::osc2Octave, "Octave", accentColour);
    semitoneKnob.configure (state, first ? params::osc1Semi : params::osc2Semi, "Semi", accentColour, true);
    fineKnob.configure (state, first ? params::osc1Fine : params::osc2Fine, "Fine", accentColour, true);
    pulseKnob.configure (state, first ? params::osc1PW : params::osc2PW, "Pulse", accentColour, true);
    levelKnob.configure (state, first ? params::osc1Level : params::osc2Level, "Level", accentColour, true);
    subKnob.configure (state, first ? params::osc1Sub : params::osc2Sub, "Sub", accentColour, true);

    addToParent (*this, waveChoice);
    addToParent (*this, octaveChoice);
    addToParent (*this, semitoneKnob);
    addToParent (*this, fineKnob);
    addToParent (*this, pulseKnob);
    addToParent (*this, levelKnob);
    addToParent (*this, subKnob);
}

void OscillatorPanel::resized()
{
    auto content = getContentBounds();

    auto selectorRow = content.removeFromTop (juce::jlimit (30, 38, content.getHeight() / 3));
    waveChoice.setBounds (selectorRow.removeFromLeft (juce::jmax (92, selectorRow.getWidth() * 42 / 100)));
    selectorRow.removeFromLeft (6);
    octaveChoice.setBounds (selectorRow.removeFromLeft (juce::jmin (112, selectorRow.getWidth())));

    content.removeFromTop (4);

    const int knobCount = 5;
    const int knobWidth = content.getWidth() / knobCount;
    AttachedKnob* knobs[] { &semitoneKnob, &fineKnob, &pulseKnob, &levelKnob, &subKnob };
    for (auto* knob : knobs)
        knob->setBounds (content.removeFromLeft (knobWidth));
}

//==============================================================================
MixerPanel::MixerPanel (juce::AudioProcessorValueTreeState& state)
    : SectionPanel ("Mixer", "OSC / SUB / NOISE", Theme::teal)
{
    const std::array<const char*, 5> ids {
        params::osc1Level, params::osc2Level, params::osc1Sub, params::osc2Sub, params::noise
    };
    const std::array<juce::String, 5> names { "OSC 1", "OSC 2", "SUB 1", "SUB 2", "NOISE" };

    for (size_t i = 0; i < faders.size(); ++i)
    {
        faders[i] = std::make_unique<AttachedFader>();
        faders[i]->configure (state, ids[i], names[i],
                              i < 2 ? Theme::teal : (i < 4 ? Theme::amber : Theme::green));
        addAndMakeVisible (*faders[i]);
    }

    ringKnob.configure (state, params::ringMod, "Ring", Theme::amber, false);
    fmKnob.configure (state, params::fmAmount, "FM", Theme::teal, false);
    ratioKnob.configure (state, params::fmRatio, "Ratio", Theme::teal, false);
    syncToggle.configure (state, params::oscSync, "Sync", Theme::amber);

    addAndMakeVisible (ringKnob);
    addAndMakeVisible (fmKnob);
    addAndMakeVisible (ratioKnob);
    addAndMakeVisible (syncToggle);
}

void MixerPanel::resized()
{
    auto content = getContentBounds();
    auto controls = content.removeFromRight (juce::jlimit (126, 164, content.getWidth() / 3));
    content.removeFromRight (6);

    const int faderWidth = content.getWidth() / (int) faders.size();
    for (auto& fader : faders)
        fader->setBounds (content.removeFromLeft (faderWidth));

    auto syncRow = controls.removeFromBottom (22);
    syncToggle.setBounds (syncRow.removeFromLeft (72));

    const int knobWidth = controls.getWidth() / 3;
    ringKnob.setBounds (controls.removeFromLeft (knobWidth));
    fmKnob.setBounds (controls.removeFromLeft (knobWidth));
    ratioKnob.setBounds (controls.removeFromLeft (knobWidth));
}

//==============================================================================
ModMatrixRow::ModMatrixRow (juce::AudioProcessorValueTreeState& state, int slotIndex)
    : rowIndex (slotIndex)
{
    sourceBox.setColour (juce::ComboBox::arrowColourId, Theme::teal);
    destinationBox.setColour (juce::ComboBox::arrowColourId, Theme::amber);

    if (auto* sourceParameter = dynamic_cast<juce::AudioParameterChoice*> (
            state.getParameter (params::matrixSourceIds[(size_t) slotIndex])))
        sourceBox.addItemList (sourceParameter->choices, 1);
    else
        jassertfalse;

    if (auto* destParameter = dynamic_cast<juce::AudioParameterChoice*> (
            state.getParameter (params::matrixDestIds[(size_t) slotIndex])))
        destinationBox.addItemList (destParameter->choices, 1);
    else
        jassertfalse;

    sourceBox.setTooltip ("Modulation source for route " + juce::String (slotIndex + 1));
    destinationBox.setTooltip ("Modulation destination for route " + juce::String (slotIndex + 1));

    amountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    amountSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
    amountSlider.setColour (juce::Slider::thumbColourId, Theme::amber);
    amountSlider.setColour (juce::Slider::textBoxTextColourId, Theme::textMuted);
    amountSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    amountSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    amountSlider.setPopupDisplayEnabled (true, false, this);
    amountSlider.setTooltip ("Route amount for route " + juce::String (slotIndex + 1));

    addAndMakeVisible (sourceBox);
    addAndMakeVisible (destinationBox);
    addAndMakeVisible (amountSlider);

    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        state, params::matrixSourceIds[(size_t) slotIndex], sourceBox);
    destAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        state, params::matrixDestIds[(size_t) slotIndex], destinationBox);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, params::matrixAmountIds[(size_t) slotIndex], amountSlider);
}

void ModMatrixRow::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (rowIndex % 2 == 0)
    {
        g.setColour (Theme::panelInset.withAlpha (0.22f));
        g.fillRoundedRectangle (bounds.toFloat().reduced (1.0f, 0.0f), 2.0f);
    }

    g.setColour (Theme::textDim);
    g.setFont (CustomLookAndFeel::monoFont (8.5f, juce::Font::bold));
    g.drawText (juce::String (rowIndex + 1), bounds.removeFromLeft (18),
                juce::Justification::centred, false);

    g.setColour (Theme::line.withAlpha (0.45f));
    g.drawHorizontalLine (getHeight() - 1, 4.0f, (float) getWidth() - 4.0f);
}

void ModMatrixRow::resized()
{
    auto bounds = getLocalBounds().reduced (2, 1);
    bounds.removeFromLeft (18);

    const auto amountWidth = juce::jmax (104, bounds.getWidth() * 38 / 100);
    auto amountArea = bounds.removeFromRight (amountWidth);
    bounds.removeFromRight (4);

    auto destinationArea = bounds.removeFromRight (bounds.getWidth() * 52 / 100);
    bounds.removeFromRight (4);

    sourceBox.setBounds (bounds.reduced (0, 1));
    destinationBox.setBounds (destinationArea.reduced (0, 1));
    amountSlider.setBounds (amountArea.reduced (0, 1));
}

//==============================================================================
ModMatrixPanel::ModMatrixPanel (juce::AudioProcessorValueTreeState& state)
    : SectionPanel ("Modulation Matrix", "8 ROUTES", Theme::amber)
{
    for (int i = 0; i < numModSlots; ++i)
    {
        rows[(size_t) i] = std::make_unique<ModMatrixRow> (state, i);
        addAndMakeVisible (*rows[(size_t) i]);
    }
}

void ModMatrixPanel::paint (juce::Graphics& g)
{
    SectionPanel::paint (g);

    auto content = getContentBounds();
    auto headers = content.removeFromTop (12);

    headers.removeFromLeft (20);
    const auto amountWidth = juce::jmax (104, headers.getWidth() * 38 / 100);
    headers.removeFromRight (amountWidth);
    headers.removeFromRight (4);
    const auto destWidth = headers.getWidth() * 52 / 100;
    const auto sourceWidth = headers.getWidth() - destWidth - 4;

    g.setColour (Theme::textDim);
    g.setFont (CustomLookAndFeel::monoFont (7.0f, juce::Font::bold));
    g.drawText ("SOURCE", headers.removeFromLeft (sourceWidth), juce::Justification::centredLeft, false);
    headers.removeFromLeft (4);
    g.drawText ("DESTINATION", headers.removeFromLeft (destWidth), juce::Justification::centredLeft, false);
    g.drawText ("AMOUNT", headers, juce::Justification::centredRight, false);
}

void ModMatrixPanel::resized()
{
    auto content = getContentBounds();
    content.removeFromTop (12);

    const int rowHeight = juce::jmax (22, content.getHeight() / numModSlots);
    for (auto& row : rows)
        row->setBounds (content.removeFromTop (rowHeight));
}

//==============================================================================
FilterPanel::FilterPanel (juce::AudioProcessorValueTreeState& state)
    : SectionPanel ("Filter", "SVF / LADDER", Theme::amber)
{
    modeChoice.configure (state, params::filterMode, "Mode", Theme::amber);
    cutoffKnob.configure (state, params::filterCutoff, "Cutoff", Theme::amber, true);
    resonanceKnob.configure (state, params::filterResonance, "Resonance", Theme::teal, true);
    driveKnob.configure (state, params::filterDrive, "Drive", Theme::amber, true);
    envAmountKnob.configure (state, params::filterEnvAmount, "Env Amt", Theme::teal, false);
    keytrackKnob.configure (state, params::filterKeyTrack, "Keytrack", Theme::teal, false);
    lfo1Knob.configure (state, params::filterLfo1, "LFO 1", Theme::teal, false);
    lfo2Knob.configure (state, params::filterLfo2, "LFO 2", Theme::teal, false);

    addAndMakeVisible (modeChoice);
    addAndMakeVisible (cutoffKnob);
    addAndMakeVisible (resonanceKnob);
    addAndMakeVisible (driveKnob);
    addAndMakeVisible (envAmountKnob);
    addAndMakeVisible (keytrackKnob);
    addAndMakeVisible (lfo1Knob);
    addAndMakeVisible (lfo2Knob);
}

void FilterPanel::resized()
{
    auto content = getContentBounds();
    auto topRow = content.removeFromTop (juce::jmax (72, content.getHeight() * 55 / 100));
    content.removeFromTop (4);

    const auto modeWidth = juce::jmin (132, topRow.getWidth() * 33 / 100);
    modeChoice.setBounds (topRow.removeFromLeft (modeWidth).withSizeKeepingCentre (modeWidth - 4, 37));
    topRow.removeFromLeft (4);

    const int topKnobWidth = topRow.getWidth() / 3;
    cutoffKnob.setBounds (topRow.removeFromLeft (topKnobWidth));
    resonanceKnob.setBounds (topRow.removeFromLeft (topKnobWidth));
    driveKnob.setBounds (topRow);

    const int bottomKnobWidth = content.getWidth() / 4;
    envAmountKnob.setBounds (content.removeFromLeft (bottomKnobWidth));
    keytrackKnob.setBounds (content.removeFromLeft (bottomKnobWidth));
    lfo1Knob.setBounds (content.removeFromLeft (bottomKnobWidth));
    lfo2Knob.setBounds (content.removeFromLeft (bottomKnobWidth));
}

//==============================================================================
EnvelopePanel::EnvelopePanel (juce::AudioProcessorValueTreeState& state)
    : SectionPanel ("Envelopes", "AMP / FILTER / MOD", Theme::teal)
{
    constexpr std::array<const char*, 3> names { "AMP", "FILTER", "MOD" };
    const std::array<juce::Colour, 3> accents { Theme::teal, Theme::amber, Theme::green };

    constexpr std::array<std::array<const char*, 4>, 3> ids {{
        { params::ampAttack, params::ampDecay, params::ampSustain, params::ampRelease },
        { params::filtAttack, params::filtDecay, params::filtSustain, params::filtRelease },
        { params::modAttack, params::modDecay, params::modSustain, params::modRelease }
    }};

    constexpr std::array<const char*, 4> stageNames { "A", "D", "S", "R" };

    for (size_t row = 0; row < 3; ++row)
    {
        previews[row] = std::make_unique<EnvelopePreview> (
            state, ids[row][0], ids[row][1], ids[row][2], ids[row][3], accents[row]);
        addAndMakeVisible (*previews[row]);

        for (size_t stage = 0; stage < 4; ++stage)
        {
            auto fader = std::make_unique<AttachedFader>();
            fader->configure (state, ids[row][stage], stageNames[stage], accents[row]);
            addAndMakeVisible (*fader);
            faders[row][stage] = std::move (fader);
        }
    }

    juce::ignoreUnused (names);
}

void EnvelopePanel::paint (juce::Graphics& g)
{
    SectionPanel::paint (g);

    auto content = getContentBounds();
    const auto rowHeight = (float) content.getHeight() / 3.0f;
    constexpr std::array<const char*, 3> names { "AMP", "FILTER", "MOD" };
    const std::array<juce::Colour, 3> accents { Theme::teal, Theme::amber, Theme::green };

    for (int i = 0; i < 3; ++i)
    {
        auto row = content.toFloat().removeFromTop (rowHeight);
        g.setColour (accents[(size_t) i].withAlpha (0.85f));
        g.fillRoundedRectangle (row.getX(), row.getCentreY() - 4.0f, 2.0f, 8.0f, 1.0f);
        g.setColour (Theme::textMuted);
        g.setFont (CustomLookAndFeel::monoFont (8.0f, juce::Font::bold));
        g.drawText (names[(size_t) i],
                    row.withTrimmedLeft (7.0f).withWidth (38.0f),
                    juce::Justification::centredLeft, false);

        if (i > 0)
        {
            g.setColour (Theme::line.withAlpha (0.5f));
            g.drawHorizontalLine ((int) row.getY(), row.getX() + 4.0f, row.getRight() - 2.0f);
        }
    }
}

void EnvelopePanel::resized()
{
    auto content = getContentBounds();
    const int rowHeight = content.getHeight() / 3;

    for (int row = 0; row < 3; ++row)
    {
        auto rowArea = content.removeFromTop (rowHeight);
        auto previewArea = rowArea.removeFromRight (juce::jmin (146, rowArea.getWidth() / 3));
        rowArea.removeFromRight (4);
        rowArea.removeFromLeft (44);

        const int faderWidth = rowArea.getWidth() / 4;
        for (int stage = 0; stage < 4; ++stage)
            faders[(size_t) row][(size_t) stage]->setBounds (rowArea.removeFromLeft (faderWidth));

        previews[(size_t) row]->setBounds (previewArea.reduced (2, 5));
    }
}

//==============================================================================
ModTabContent::ModTabContent (juce::AudioProcessorValueTreeState& state)
{
    for (int i = 0; i < 2; ++i)
    {
        auto& group = groups[(size_t) i];
        const auto accent = i == 0 ? Theme::teal : Theme::amber;

        group.shapeChoice.configure (state,
            i == 0 ? params::lfo1Shape : params::lfo2Shape, "Shape", accent);
        group.divisionChoice.configure (state,
            i == 0 ? params::lfo1Division : params::lfo2Division, "Division", accent);
        group.rateKnob.configure (state,
            i == 0 ? params::lfo1Rate : params::lfo2Rate, "Rate", accent, true);
        group.syncToggle.configure (state,
            i == 0 ? params::lfo1Sync : params::lfo2Sync, "Host Sync", accent);

        addAndMakeVisible (group.shapeChoice);
        addAndMakeVisible (group.divisionChoice);
        addAndMakeVisible (group.rateKnob);
        addAndMakeVisible (group.syncToggle);
    }
}

void ModTabContent::paint (juce::Graphics& g)
{
    for (int i = 0; i < 2; ++i)
    {
        auto groupArea = groupBounds[(size_t) i].toFloat();
        if (groupArea.isEmpty())
            continue;

        const auto accent = i == 0 ? Theme::teal : Theme::amber;
        CustomLookAndFeel::drawGlassPanel (g, groupArea, accent);
        CustomLookAndFeel::drawPanelTitle (g, groupBounds[(size_t) i].reduced (8).removeFromTop (15),
                                           i == 0 ? "LFO 1" : "LFO 2",
                                           i == 0 ? "GLOBAL MODULATOR" : "GLOBAL MODULATOR",
                                           accent);
    }
}

void ModTabContent::resized()
{
    auto area = getLocalBounds().reduced (4, 3);
    const int gap = 8;
    const int groupWidth = (area.getWidth() - gap) / 2;

    for (int i = 0; i < 2; ++i)
    {
        auto groupArea = area.removeFromLeft (groupWidth);
        groupBounds[(size_t) i] = groupArea;
        if (i == 0)
            area.removeFromLeft (gap);

        auto inner = groupArea.reduced (9);
        inner.removeFromTop (18);

        auto rateArea = inner.removeFromRight (70);
        groups[(size_t) i].rateKnob.setBounds (rateArea);

        auto choiceArea = inner;
        groups[(size_t) i].shapeChoice.setBounds (
            choiceArea.removeFromTop (34).removeFromLeft (juce::jmin (132, choiceArea.getWidth())));
        choiceArea.removeFromTop (6);
        groups[(size_t) i].divisionChoice.setBounds (
            choiceArea.removeFromTop (34).removeFromLeft (juce::jmin (112, choiceArea.getWidth())));
        choiceArea.removeFromTop (5);
        groups[(size_t) i].syncToggle.setBounds (
            choiceArea.removeFromTop (20).removeFromLeft (88));
    }
}

//==============================================================================
FxTabContent::FxTabContent (juce::AudioProcessorValueTreeState& state)
{
    chorusOn.configure (state, params::chorusOn, "On", Theme::teal);
    chorusRate.configure (state, params::chorusRate, "Rate", Theme::teal, false);
    chorusDepth.configure (state, params::chorusDepth, "Depth", Theme::teal, false);
    chorusMix.configure (state, params::chorusMix, "Mix", Theme::teal, false);
    chorusFeedback.configure (state, params::chorusFeedback, "Feedback", Theme::teal, false);

    delayOn.configure (state, params::delayOn, "On", Theme::amber);
    delaySync.configure (state, params::delaySync, "Sync", Theme::amber);
    delayPingPong.configure (state, params::delayPingPong, "Ping", Theme::amber);
    delayDivision.configure (state, params::delayDivision, "Division", Theme::amber);
    delayTime.configure (state, params::delayTime, "Time", Theme::amber, false);
    delayFeedback.configure (state, params::delayFeedback, "Feedback", Theme::amber, false);
    delayMix.configure (state, params::delayMix, "Mix", Theme::amber, false);
    delayTone.configure (state, params::delayTone, "Tone", Theme::amber, false);

    reverbOn.configure (state, params::reverbOn, "On", Theme::green);
    reverbSize.configure (state, params::reverbSize, "Size", Theme::green, false);
    reverbDamping.configure (state, params::reverbDamping, "Damp", Theme::green, false);
    reverbWidth.configure (state, params::reverbWidth, "Width", Theme::green, false);
    reverbMix.configure (state, params::reverbMix, "Mix", Theme::green, false);

    masterDrive.configure (state, params::masterDrive, "Drive", Theme::amber, false);
    eqLow.configure (state, params::eqLow, "Low", Theme::teal, false);
    eqMid.configure (state, params::eqMid, "Mid", Theme::teal, false);
    eqHigh.configure (state, params::eqHigh, "High", Theme::teal, false);

    juce::Component* all[] {
        &chorusOn, &chorusRate, &chorusDepth, &chorusMix, &chorusFeedback,
        &delayOn, &delaySync, &delayPingPong, &delayDivision,
        &delayTime, &delayFeedback, &delayMix, &delayTone,
        &reverbOn, &reverbSize, &reverbDamping, &reverbWidth, &reverbMix,
        &masterDrive, &eqLow, &eqMid, &eqHigh
    };

    for (auto* component : all)
        addAndMakeVisible (*component);
}

void FxTabContent::paint (juce::Graphics& g)
{
    constexpr std::array<const char*, 4> titles { "CHORUS", "DELAY", "REVERB", "MASTER" };

    for (int i = 0; i < 4; ++i)
    {
        auto area = groupBounds[(size_t) i].toFloat();
        if (area.isEmpty())
            continue;

        const auto accent = groupAccents[(size_t) i];
        CustomLookAndFeel::drawGlassPanel (g, area, accent);
        CustomLookAndFeel::drawPanelTitle (g, groupBounds[(size_t) i].reduced (8).removeFromTop (14),
                                           titles[(size_t) i], "", accent);
    }
}

void FxTabContent::resized()
{
    auto area = getLocalBounds().reduced (4, 3);
    const int gap = 7;
    const int groupWidth = (area.getWidth() - gap * 3) / 4;

    for (int i = 0; i < 4; ++i)
    {
        groupBounds[(size_t) i] = area.removeFromLeft (groupWidth);
        if (i < 3)
            area.removeFromLeft (gap);
    }

    // Chorus.
    {
        auto inner = groupBounds[0].reduced (8);
        inner.removeFromTop (17);
        chorusOn.setBounds (inner.removeFromTop (18).removeFromLeft (58));
        inner.removeFromTop (3);
        const int w = inner.getWidth() / 4;
        chorusRate.setBounds (inner.removeFromLeft (w));
        chorusDepth.setBounds (inner.removeFromLeft (w));
        chorusMix.setBounds (inner.removeFromLeft (w));
        chorusFeedback.setBounds (inner);
    }

    // Delay.
    {
        auto inner = groupBounds[1].reduced (8);
        inner.removeFromTop (17);

        auto topRow = inner.removeFromTop (33);
        const auto toggleWidth = juce::jmax (34, (topRow.getWidth() - 74 - 8) / 3);
        delayOn.setBounds (topRow.removeFromLeft (toggleWidth).withSizeKeepingCentre (toggleWidth, 18));
        delaySync.setBounds (topRow.removeFromLeft (toggleWidth).withSizeKeepingCentre (toggleWidth, 18));
        delayPingPong.setBounds (topRow.removeFromLeft (toggleWidth).withSizeKeepingCentre (toggleWidth, 18));
        topRow.removeFromLeft (4);
        delayDivision.setBounds (topRow);

        inner.removeFromTop (4);
        const int w = inner.getWidth() / 4;
        delayTime.setBounds (inner.removeFromLeft (w));
        delayFeedback.setBounds (inner.removeFromLeft (w));
        delayMix.setBounds (inner.removeFromLeft (w));
        delayTone.setBounds (inner);
    }

    // Reverb.
    {
        auto inner = groupBounds[2].reduced (8);
        inner.removeFromTop (17);
        reverbOn.setBounds (inner.removeFromTop (18).removeFromLeft (58));
        inner.removeFromTop (3);
        const int w = inner.getWidth() / 4;
        reverbSize.setBounds (inner.removeFromLeft (w));
        reverbDamping.setBounds (inner.removeFromLeft (w));
        reverbWidth.setBounds (inner.removeFromLeft (w));
        reverbMix.setBounds (inner);
    }

    // Master.
    {
        auto inner = groupBounds[3].reduced (8);
        inner.removeFromTop (22);
        const int w = inner.getWidth() / 4;
        masterDrive.setBounds (inner.removeFromLeft (w));
        eqLow.setBounds (inner.removeFromLeft (w));
        eqMid.setBounds (inner.removeFromLeft (w));
        eqHigh.setBounds (inner);
    }
}

//==============================================================================
MacroPanel::MacroPanel (juce::AudioProcessorValueTreeState& state)
    : SectionPanel ("Performance Macros", "LIVE CONTROLS", Theme::amber)
{
    constexpr std::array<const char*, 4> ids {
        params::macro1, params::macro2, params::macro3, params::macro4
    };
    constexpr std::array<const char*, 4> names { "Macro 1", "Macro 2", "Macro 3", "Macro 4" };
    const std::array<juce::Colour, 4> accents {
        Theme::teal, Theme::green, Theme::teal, Theme::amber
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i] = std::make_unique<AttachedKnob>();
        knobs[i]->configure (state, ids[i], names[i], accents[i], true);
        const std::array<const char*, 4> descriptions {
            "Macro 1 - filter cutoff (+3 octaves)",
            "Macro 2 - reverb mix",
            "Macro 3 - chorus mix and depth",
            "Macro 4 - master drive"
        };
        knobs[i]->slider.setTooltip (descriptions[i]);
        addAndMakeVisible (*knobs[i]);
    }
}

void MacroPanel::resized()
{
    auto content = getContentBounds();
    const int knobWidth = content.getWidth() / 4;
    for (auto& knob : knobs)
        knob->setBounds (content.removeFromLeft (knobWidth));
}

//==============================================================================
SynthwaveLegacyEditor::SynthwaveLegacyEditor (SynthwaveLegacyProcessor& processorToUse)
    : AudioProcessorEditor (&processorToUse),
      processor (processorToUse),
      state (processorToUse.getValueTreeState()),
      levelMeter (processorToUse),
      scope (processorToUse),
      oscillator1 (processorToUse.getValueTreeState(), 0),
      oscillator2 (processorToUse.getValueTreeState(), 1),
      mixer (processorToUse.getValueTreeState()),
      modulationMatrix (processorToUse.getValueTreeState()),
      filterPanel (processorToUse.getValueTreeState()),
      envelopePanel (processorToUse.getValueTreeState()),
      modTab (processorToUse.getValueTreeState()),
      fxTab (processorToUse.getValueTreeState()),
      macroPanel (processorToUse.getValueTreeState()),
      keyboard (std::make_unique<SynthKeyboard> (processorToUse.getKeyboardState()))
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    addAndMakeVisible (brand);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (previousPreset);
    addAndMakeVisible (nextPreset);
    addAndMakeVisible (presetLabel);
    addAndMakeVisible (statusLabel);

    previousPreset.setColour (juce::TextButton::textColourOffId, Theme::teal);
    nextPreset.setColour (juce::TextButton::textColourOffId, Theme::teal);
    previousPreset.setColour (juce::TextButton::buttonColourId, Theme::panelRaised);
    nextPreset.setColour (juce::TextButton::buttonColourId, Theme::panelRaised);
    previousPreset.setTooltip ("Previous factory preset");
    nextPreset.setTooltip ("Next factory preset");

    previousPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setFont (CustomLookAndFeel::monoFont (7.5f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, Theme::textDim);
    presetLabel.setJustificationType (juce::Justification::centredRight);

    statusLabel.setFont (CustomLookAndFeel::monoFont (8.0f, juce::Font::bold));
    statusLabel.setColour (juce::Label::textColourId, Theme::textDim);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    statusLabel.setText ("120.0 BPM  /  STOP", juce::dontSendNotification);

    masterGain.configure (state, params::masterGain, "Master", Theme::amber, true);
    addAndMakeVisible (masterGain);
    addAndMakeVisible (levelMeter);

    addAndMakeVisible (scope);
    addAndMakeVisible (oscillator1);
    addAndMakeVisible (oscillator2);
    addAndMakeVisible (mixer);
    addAndMakeVisible (modulationMatrix);
    addAndMakeVisible (filterPanel);
    addAndMakeVisible (envelopePanel);

    tabs.setTabBarDepth (25);
    tabs.setOutline (0);
    tabs.setIndent (0);
    tabs.setColour (juce::TabbedComponent::backgroundColourId, Theme::panelInset);
    tabs.setColour (juce::TabbedComponent::outlineColourId, Theme::line);
    tabs.addTab ("MOD", Theme::panelInset, &modTab, false);
    tabs.addTab ("FX", Theme::panelInset, &fxTab, false);
    tabs.setCurrentTabIndex (0);
    addAndMakeVisible (tabs);

    addAndMakeVisible (macroPanel);
    addAndMakeVisible (*keyboard);

    configurePresetBrowser();

    setResizable (true, true);
    setResizeLimits (1180, 800, 2400, 1500);
    setSize (1520, 960);
    startTimerHz (10);
}

SynthwaveLegacyEditor::~SynthwaveLegacyEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SynthwaveLegacyEditor::configurePresetBrowser()
{
    presetBox.addItemList (presetChoices(), 1);
    presetBox.setSelectedItemIndex (processor.getCurrentProgram(), juce::dontSendNotification);
    presetBox.setColour (juce::ComboBox::arrowColourId, Theme::amber);
    presetBox.setTooltip ("Factory preset browser");

    presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        state, params::preset, presetBox);

    presetBox.onChange = [this]
    {
        if (suppressPresetCallback || ! presetBox.consumeUserSelection())
            return;

        const auto index = presetBox.getSelectedItemIndex();
        if (index < 0)
            return;

        const juce::ScopedValueSetter<bool> guard (suppressPresetCallback, true);
        processor.loadFactoryPreset (index);
        presetBox.setSelectedItemIndex (index, juce::dontSendNotification);
    };
}

void SynthwaveLegacyEditor::stepPreset (int delta)
{
    const auto count = processor.getNumPrograms();
    if (count <= 0)
        return;

    const auto current = processor.getCurrentProgram();
    const auto next = (current + delta + count) % count;

    const juce::ScopedValueSetter<bool> guard (suppressPresetCallback, true);
    processor.loadFactoryPreset (next);
    presetBox.setSelectedItemIndex (next, juce::dontSendNotification);
}

void SynthwaveLegacyEditor::timerCallback()
{
    const auto bpm = processor.getDisplayedBpm();
    const auto transport = processor.isTransportRunning() ? "RUN" : "STOP";
    statusLabel.setText (juce::String::formatted ("%.1f BPM  /  %s", bpm, transport),
                         juce::dontSendNotification);
}

//==============================================================================
void SynthwaveLegacyEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    juce::ColourGradient background (Theme::backgroundAlt, 0.0f, 0.0f,
                                     Theme::background, 0.0f, (float) getHeight(), false);
    g.setGradientFill (background);
    g.fillRect (bounds);

    // Fine engineering grid.
    g.setColour (juce::Colours::white.withAlpha (0.014f));
    for (int x = 0; x < getWidth(); x += 44)
        g.drawVerticalLine (x, 0.0f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 44)
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    // Top console plate.
    auto top = bounds.removeFromTop (kTopBarHeight).toFloat();
    juce::ColourGradient topGradient (Theme::panelRaised.withAlpha (0.78f),
                                      0.0f, 0.0f,
                                      Theme::backgroundAlt.withAlpha (0.30f),
                                      0.0f, top.getBottom(), false);
    g.setGradientFill (topGradient);
    g.fillRect (top);

    g.setColour (Theme::line.withAlpha (0.9f));
    g.drawHorizontalLine (kTopBarHeight - 1, 0.0f, (float) getWidth());

    juce::ColourGradient accentLine (Theme::teal.withAlpha (0.85f), 0.0f, 0.0f,
                                     Theme::amber.withAlpha (0.70f), (float) getWidth(), 0.0f,
                                     false);
    g.setGradientFill (accentLine);
    g.fillRect (0.0f, 0.0f, (float) getWidth(), 1.5f);

    // Footer console line.
    auto keyboardTop = getHeight() - kKeyboardHeight - kOuterMargin / 2;
    g.setColour (Theme::line.withAlpha (0.75f));
    g.drawHorizontalLine (keyboardTop, 0.0f, (float) getWidth());

    // Window edge and corner brackets.
    g.setColour (Theme::line.withAlpha (0.55f));
    g.drawRect (getLocalBounds(), 1);
    g.setColour (Theme::teal.withAlpha (0.55f));
    g.drawHorizontalLine (0, 0.0f, 18.0f);
    g.drawVerticalLine (0, 0.0f, 18.0f);
    g.setColour (Theme::amber.withAlpha (0.55f));
    g.drawHorizontalLine (getHeight() - 1, (float) getWidth() - 18.0f, (float) getWidth());
    g.drawVerticalLine (getWidth() - 1, (float) getHeight() - 18.0f, (float) getHeight());

    g.setColour (Theme::textDim.withAlpha (0.65f));
    g.setFont (CustomLookAndFeel::monoFont (7.0f));
    g.drawText ("SWL-01  //  VIRTUAL ANALOG INSTRUMENT  //  V" SYNTHWAVE_VERSION,
                getLocalBounds().reduced (14).removeFromBottom (10),
                juce::Justification::centredRight, false);
}

void SynthwaveLegacyEditor::resized()
{
    auto area = getLocalBounds().reduced (kOuterMargin);

    auto topArea = area.removeFromTop (kTopBarHeight);
    area.removeFromTop (6);

    auto keyboardArea = area.removeFromBottom (kKeyboardHeight);
    area.removeFromBottom (6);

    auto bottomArea = area.removeFromBottom (kBottomBarHeight);
    area.removeFromBottom (10);

    auto mainArea = area;

    // ---- Top console --------------------------------------------------------
    brand.setBounds (topArea.removeFromLeft (292));

    auto topRight = topArea.removeFromRight (juce::jmin (410, topArea.getWidth() / 2));
    statusLabel.setBounds (topRight.removeFromRight (144).reduced (0, 24));
    if (topRight.getWidth() > 0)
    {
        levelMeter.setBounds (topRight.removeFromRight (132).reduced (0, 13));
        topRight.removeFromRight (8);
        masterGain.setBounds (topRight.reduced (2, 1));
    }

    auto presetArea = topArea.reduced (8, 17);
    previousPreset.setBounds (presetArea.removeFromLeft (30).withSizeKeepingCentre (28, 30));
    presetArea.removeFromLeft (7);
    presetLabel.setBounds (presetArea.removeFromLeft (55).withSizeKeepingCentre (52, 15));
    presetArea.removeFromLeft (6);
    auto presetRect = presetArea.removeFromLeft (juce::jmin (290, presetArea.getWidth()));
    presetBox.setBounds (presetRect.withSizeKeepingCentre (juce::jmin (280, presetRect.getWidth()), 30));
    presetArea.removeFromLeft (7);
    nextPreset.setBounds (presetArea.removeFromLeft (30).withSizeKeepingCentre (28, 30));

    // ---- Main columns -------------------------------------------------------
    const int gap = kColumnGap;
    const int contentWidth = mainArea.getWidth();
    const int leftWidth = juce::roundToInt (contentWidth * 0.285f);
    const int rightWidth = juce::roundToInt (contentWidth * 0.36f);
    const int centreWidth = contentWidth - leftWidth - rightWidth - gap * 2;

    auto leftColumn = mainArea.removeFromLeft (leftWidth);
    mainArea.removeFromLeft (gap);
    auto centreColumn = mainArea.removeFromLeft (centreWidth);
    mainArea.removeFromLeft (gap);
    auto rightColumn = mainArea;

    {
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::column;
        flex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        flex.items.add (juce::FlexItem (scope).withFlex (0.29f)
                            .withMargin (juce::FlexItem::Margin (0.0f, 0.0f, (float) gap, 0.0f)));
        flex.items.add (juce::FlexItem (oscillator1).withFlex (0.355f)
                            .withMargin (juce::FlexItem::Margin (0.0f, 0.0f, (float) gap, 0.0f)));
        flex.items.add (juce::FlexItem (oscillator2).withFlex (0.355f));
        flex.performLayout (leftColumn);
    }

    {
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::column;
        flex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        flex.items.add (juce::FlexItem (mixer).withFlex (0.33f)
                            .withMargin (juce::FlexItem::Margin (0.0f, 0.0f, (float) gap, 0.0f)));
        flex.items.add (juce::FlexItem (modulationMatrix).withFlex (0.67f));
        flex.performLayout (centreColumn);
    }

    {
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::column;
        flex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        flex.items.add (juce::FlexItem (filterPanel).withFlex (0.46f)
                            .withMargin (juce::FlexItem::Margin (0.0f, 0.0f, (float) gap, 0.0f)));
        flex.items.add (juce::FlexItem (envelopePanel).withFlex (0.54f));
        flex.performLayout (rightColumn);
    }

    // ---- Bottom utility strip ----------------------------------------------
    {
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.items.add (juce::FlexItem (tabs).withFlex (3.4f)
                            .withMargin (juce::FlexItem::Margin (0.0f, 10.0f, 0.0f, 0.0f)));
        flex.items.add (juce::FlexItem (macroPanel).withFlex (1.0f));
        flex.performLayout (bottomArea);
    }

    // ---- Keyboard -----------------------------------------------------------
    auto keyboardBounds = keyboardArea.reduced (0, 3);
    keyboard->setBounds (keyboardBounds);
    keyboard->setKeyWidth (juce::jmax (16.0f, (float) keyboardBounds.getWidth() / 36.0f));
}

} // namespace synthwave
