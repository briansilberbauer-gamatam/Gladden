#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <atomic>
#include <memory>

#include "LookAndFeel.h"
#include "PluginProcessor.h"
#include "UIComponents.h"

namespace synthwave
{
//==============================================================================
class SynthKeyboard final : public juce::MidiKeyboardComponent
{
public:
    explicit SynthKeyboard (juce::MidiKeyboardState& keyboardState);
    ~SynthKeyboard() override = default;

    void drawWhiteNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour,
                        juce::Colour textColour) override;

    void drawBlackNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override;
};

//==============================================================================
class PresetComboBox final : public juce::ComboBox,
                             private juce::Timer
{
public:
    PresetComboBox() = default;
    ~PresetComboBox() override = default;

    void showPopup() override;
    bool consumeUserSelection() noexcept;

private:
    void timerCallback() override;

    bool userSelectionWindow = false;
    juce::uint32 popupClosedAtMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetComboBox)
};

//==============================================================================
class BrandPanel final : public juce::Component
{
public:
    BrandPanel() = default;
    void paint (juce::Graphics&) override;
};

//==============================================================================
class EnvelopePreview final : public juce::Component,
                              private juce::Timer
{
public:
    EnvelopePreview (juce::AudioProcessorValueTreeState& state,
                     juce::String attackId, juce::String decayId,
                     juce::String sustainId, juce::String releaseId,
                     juce::Colour accent);
    ~EnvelopePreview() override = default;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    const std::atomic<float>* attack = nullptr;
    const std::atomic<float>* decay = nullptr;
    const std::atomic<float>* sustain = nullptr;
    const std::atomic<float>* release = nullptr;
    juce::Colour accentColour;
};

//==============================================================================
class OscillatorPanel final : public SectionPanel
{
public:
    OscillatorPanel (juce::AudioProcessorValueTreeState& state, int oscillatorIndex);
    void resized() override;

private:
    AttachedChoice waveChoice;
    AttachedChoice octaveChoice;
    AttachedKnob semitoneKnob;
    AttachedKnob fineKnob;
    AttachedKnob pulseKnob;
    AttachedKnob levelKnob;
    AttachedKnob subKnob;
};

//==============================================================================
class MixerPanel final : public SectionPanel
{
public:
    explicit MixerPanel (juce::AudioProcessorValueTreeState& state);
    void resized() override;

private:
    std::array<std::unique_ptr<AttachedFader>, 5> faders;
    AttachedKnob ringKnob;
    AttachedKnob fmKnob;
    AttachedKnob ratioKnob;
    AttachedToggle syncToggle;
};

//==============================================================================
class ModMatrixRow final : public juce::Component
{
public:
    ModMatrixRow (juce::AudioProcessorValueTreeState& state, int slotIndex);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::ComboBox sourceBox;
    juce::ComboBox destinationBox;
    juce::Slider amountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    int rowIndex = 0;
};

//==============================================================================
class ModMatrixPanel final : public SectionPanel
{
public:
    explicit ModMatrixPanel (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::array<std::unique_ptr<ModMatrixRow>, numModSlots> rows;
};

//==============================================================================
class FilterPanel final : public SectionPanel
{
public:
    explicit FilterPanel (juce::AudioProcessorValueTreeState& state);
    void resized() override;

private:
    AttachedChoice modeChoice;
    AttachedKnob cutoffKnob;
    AttachedKnob resonanceKnob;
    AttachedKnob driveKnob;
    AttachedKnob envAmountKnob;
    AttachedKnob keytrackKnob;
    AttachedKnob lfo1Knob;
    AttachedKnob lfo2Knob;
};

//==============================================================================
class EnvelopePanel final : public SectionPanel
{
public:
    explicit EnvelopePanel (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::array<std::array<std::unique_ptr<AttachedFader>, 4>, 3> faders;
    std::array<std::unique_ptr<EnvelopePreview>, 3> previews;
};

//==============================================================================
class ModTabContent final : public juce::Component
{
public:
    explicit ModTabContent (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct LfoGroup
    {
        AttachedChoice shapeChoice;
        AttachedChoice divisionChoice;
        AttachedKnob rateKnob;
        AttachedToggle syncToggle;
    };

    std::array<LfoGroup, 2> groups;
    std::array<juce::Rectangle<int>, 2> groupBounds;
};

//==============================================================================
class FxTabContent final : public juce::Component
{
public:
    explicit FxTabContent (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AttachedToggle chorusOn;
    AttachedKnob chorusRate;
    AttachedKnob chorusDepth;
    AttachedKnob chorusMix;
    AttachedKnob chorusFeedback;

    AttachedToggle delayOn;
    AttachedToggle delaySync;
    AttachedToggle delayPingPong;
    AttachedChoice delayDivision;
    AttachedKnob delayTime;
    AttachedKnob delayFeedback;
    AttachedKnob delayMix;
    AttachedKnob delayTone;

    AttachedToggle reverbOn;
    AttachedKnob reverbSize;
    AttachedKnob reverbDamping;
    AttachedKnob reverbWidth;
    AttachedKnob reverbMix;

    AttachedKnob masterDrive;
    AttachedKnob eqLow;
    AttachedKnob eqMid;
    AttachedKnob eqHigh;

    std::array<juce::Rectangle<int>, 4> groupBounds;
};

//==============================================================================
class MacroPanel final : public SectionPanel
{
public:
    explicit MacroPanel (juce::AudioProcessorValueTreeState& state);
    void resized() override;

private:
    std::array<std::unique_ptr<AttachedKnob>, 4> knobs;
};

//==============================================================================
class SynthwaveLegacyEditor final : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit SynthwaveLegacyEditor (SynthwaveLegacyProcessor&);
    ~SynthwaveLegacyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configurePresetBrowser();
    void stepPreset (int delta);

    SynthwaveLegacyProcessor& processor;
    juce::AudioProcessorValueTreeState& state;
    CustomLookAndFeel lookAndFeel;

    BrandPanel brand;
    PresetComboBox presetBox;
    juce::TextButton previousPreset { "<" };
    juce::TextButton nextPreset { ">" };
    juce::Label presetLabel;
    juce::Label statusLabel;
    AttachedKnob masterGain;
    LevelMeter levelMeter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;
    bool suppressPresetCallback = false;

    ScopeDisplay scope;
    OscillatorPanel oscillator1;
    OscillatorPanel oscillator2;

    MixerPanel mixer;
    ModMatrixPanel modulationMatrix;

    FilterPanel filterPanel;
    EnvelopePanel envelopePanel;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    ModTabContent modTab;
    FxTabContent fxTab;
    MacroPanel macroPanel;

    std::unique_ptr<SynthKeyboard> keyboard;
    juce::TooltipWindow tooltipWindow { this, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthwaveLegacyEditor)
};

} // namespace synthwave
