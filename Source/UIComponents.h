#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

#include "LookAndFeel.h"

namespace synthwave
{
class SynthwaveLegacyProcessor;

//==============================================================================
class AttachedKnob final : public juce::Component
{
public:
    AttachedKnob() = default;

    void configure (juce::AudioProcessorValueTreeState& state,
                    const juce::String& parameterId,
                    const juce::String& displayName,
                    juce::Colour accent,
                    bool showValue = true);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider slider;
    juce::Label nameLabel;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Colour accentColour = Theme::teal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttachedKnob)
};

//==============================================================================
class AttachedChoice final : public juce::Component
{
public:
    AttachedChoice() = default;

    void configure (juce::AudioProcessorValueTreeState& state,
                    const juce::String& parameterId,
                    const juce::String& displayName,
                    juce::Colour accent);

    void resized() override;

    juce::ComboBox combo;
    juce::Label nameLabel;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttachedChoice)
};

//==============================================================================
class AttachedFader final : public juce::Component
{
public:
    AttachedFader() = default;

    void configure (juce::AudioProcessorValueTreeState& state,
                    const juce::String& parameterId,
                    const juce::String& displayName,
                    juce::Colour accent);

    void resized() override;

    juce::Slider slider;
    juce::Label nameLabel;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttachedFader)
};

//==============================================================================
class AttachedToggle final : public juce::Component
{
public:
    AttachedToggle() = default;

    void configure (juce::AudioProcessorValueTreeState& state,
                    const juce::String& parameterId,
                    const juce::String& displayName,
                    juce::Colour accent);

    void resized() override;

    juce::ToggleButton toggle;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttachedToggle)
};

//==============================================================================
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::String titleText, juce::String subtitleText, juce::Colour accentColour);

    void paint (juce::Graphics&) override;
    juce::Rectangle<int> getContentBounds() const;

    void setTitle (juce::String newTitle)    { title = std::move (newTitle); repaint(); }
    void setSubtitle (juce::String value)    { subtitle = std::move (value); repaint(); }
    void setAccent (juce::Colour value)      { accent = value; repaint(); }

protected:
    juce::String title;
    juce::String subtitle;
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionPanel)
};

//==============================================================================
class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    explicit LevelMeter (SynthwaveLegacyProcessor& processor);
    ~LevelMeter() override = default;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    SynthwaveLegacyProcessor& audioProcessor;
    float displayedLeft = 0.0f;
    float displayedRight = 0.0f;
    float holdLeft = 0.0f;
    float holdRight = 0.0f;
    int holdCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

//==============================================================================
class ScopeDisplay final : public juce::Component,
                           private juce::Timer
{
public:
    explicit ScopeDisplay (SynthwaveLegacyProcessor& processor);
    ~ScopeDisplay() override = default;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    SynthwaveLegacyProcessor& audioProcessor;
    std::array<float, 512> samples {};
    int numSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeDisplay)
};

} // namespace synthwave
