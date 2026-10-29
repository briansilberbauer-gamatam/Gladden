#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace synthwave
{
//==============================================================================
namespace Theme
{
inline const juce::Colour background   { 0xff090d12 };
inline const juce::Colour backgroundAlt{ 0xff0d1218 };
inline const juce::Colour panel        { 0xff121920 };
inline const juce::Colour panelRaised  { 0xff17212a };
inline const juce::Colour panelInset   { 0xff0b1117 };
inline const juce::Colour line         { 0xff22313b };
inline const juce::Colour lineBright   { 0xff33515f };
inline const juce::Colour text         { 0xffd9e8ee };
inline const juce::Colour textMuted    { 0xff7d909b };
inline const juce::Colour textDim      { 0xff4e6069 };
inline const juce::Colour teal         { 0xff00e5ff };
inline const juce::Colour tealDim      { 0xff007f91 };
inline const juce::Colour amber        { 0xffff9100 };
inline const juce::Colour amberDim     { 0xff9d5b00 };
inline const juce::Colour red          { 0xffff4d5e };
inline const juce::Colour green        { 0xff36e6a0 };
} // namespace Theme

//==============================================================================
class CustomLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawTabButton (juce::TabBarButton&, juce::Graphics&,
                        bool isMouseOver, bool isMouseDown) override;
    void drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    static void drawGlassPanel (juce::Graphics&, juce::Rectangle<float> area,
                                juce::Colour accent, bool raised = false,
                                float cornerSize = 7.0f);

    static void drawPanelTitle (juce::Graphics&, juce::Rectangle<int> area,
                                const juce::String& title,
                                const juce::String& subtitle,
                                juce::Colour accent);

    static juce::Font sansFont (float height, int style = juce::Font::plain);
    static juce::Font monoFont (float height, int style = juce::Font::plain);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLookAndFeel)
};

} // namespace synthwave
