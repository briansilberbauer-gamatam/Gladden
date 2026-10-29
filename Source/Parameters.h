#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <array>

namespace synthwave
{

//==============================================================================
/** Stable parameter identifiers. Keeping these in one place avoids typos and
    gives the preset system a single source of truth. */
namespace params
{
inline constexpr auto preset = "preset";

inline constexpr auto macro1 = "macro1";
inline constexpr auto macro2 = "macro2";
inline constexpr auto macro3 = "macro3";
inline constexpr auto macro4 = "macro4";

inline constexpr auto osc1Wave   = "osc1Wave";
inline constexpr auto osc1Octave = "osc1Octave";
inline constexpr auto osc1Semi   = "osc1Semi";
inline constexpr auto osc1Fine   = "osc1Fine";
inline constexpr auto osc1Level  = "osc1Level";
inline constexpr auto osc1PW     = "osc1PW";
inline constexpr auto osc1Sub    = "osc1Sub";

inline constexpr auto osc2Wave   = "osc2Wave";
inline constexpr auto osc2Octave = "osc2Octave";
inline constexpr auto osc2Semi   = "osc2Semi";
inline constexpr auto osc2Fine   = "osc2Fine";
inline constexpr auto osc2Level  = "osc2Level";
inline constexpr auto osc2PW     = "osc2PW";
inline constexpr auto osc2Sub    = "osc2Sub";

inline constexpr auto oscSync = "oscSync";
inline constexpr auto ringMod = "ringMod";
inline constexpr auto fmAmount = "fmAmount";
inline constexpr auto fmRatio  = "fmRatio";
inline constexpr auto noise    = "noise";

inline constexpr auto filterMode      = "filterMode";
inline constexpr auto filterCutoff    = "filterCutoff";
inline constexpr auto filterResonance = "filterResonance";
inline constexpr auto filterDrive     = "filterDrive";
inline constexpr auto filterEnvAmount = "filterEnvAmount";
inline constexpr auto filterKeyTrack  = "filterKeyTrack";
inline constexpr auto filterLfo1      = "filterLfo1";
inline constexpr auto filterLfo2      = "filterLfo2";

inline constexpr auto ampAttack  = "ampAttack";
inline constexpr auto ampDecay   = "ampDecay";
inline constexpr auto ampSustain = "ampSustain";
inline constexpr auto ampRelease = "ampRelease";

inline constexpr auto filtAttack  = "filtAttack";
inline constexpr auto filtDecay   = "filtDecay";
inline constexpr auto filtSustain = "filtSustain";
inline constexpr auto filtRelease = "filtRelease";

inline constexpr auto modAttack  = "modAttack";
inline constexpr auto modDecay   = "modDecay";
inline constexpr auto modSustain = "modSustain";
inline constexpr auto modRelease = "modRelease";

inline constexpr auto lfo1Rate     = "lfo1Rate";
inline constexpr auto lfo1Shape    = "lfo1Shape";
inline constexpr auto lfo1Sync     = "lfo1Sync";
inline constexpr auto lfo1Division = "lfo1Division";

inline constexpr auto lfo2Rate     = "lfo2Rate";
inline constexpr auto lfo2Shape    = "lfo2Shape";
inline constexpr auto lfo2Sync     = "lfo2Sync";
inline constexpr auto lfo2Division = "lfo2Division";

inline constexpr auto chorusOn       = "chorusOn";
inline constexpr auto chorusRate      = "chorusRate";
inline constexpr auto chorusDepth     = "chorusDepth";
inline constexpr auto chorusMix       = "chorusMix";
inline constexpr auto chorusFeedback  = "chorusFeedback";

inline constexpr auto delayOn       = "delayOn";
inline constexpr auto delaySync     = "delaySync";
inline constexpr auto delayDivision = "delayDivision";
inline constexpr auto delayTime     = "delayTime";
inline constexpr auto delayFeedback = "delayFeedback";
inline constexpr auto delayMix      = "delayMix";
inline constexpr auto delayPingPong = "delayPingPong";
inline constexpr auto delayTone     = "delayTone";

inline constexpr auto reverbOn      = "reverbOn";
inline constexpr auto reverbSize    = "reverbSize";
inline constexpr auto reverbDamping = "reverbDamping";
inline constexpr auto reverbWidth   = "reverbWidth";
inline constexpr auto reverbMix     = "reverbMix";

inline constexpr auto masterDrive = "masterDrive";
inline constexpr auto eqLow       = "eqLow";
inline constexpr auto eqMid       = "eqMid";
inline constexpr auto eqHigh      = "eqHigh";
inline constexpr auto masterGain  = "masterGain";

inline constexpr auto matrixSource1 = "matrixSource1";
inline constexpr auto matrixSource2 = "matrixSource2";
inline constexpr auto matrixSource3 = "matrixSource3";
inline constexpr auto matrixSource4 = "matrixSource4";
inline constexpr auto matrixSource5 = "matrixSource5";
inline constexpr auto matrixSource6 = "matrixSource6";
inline constexpr auto matrixSource7 = "matrixSource7";
inline constexpr auto matrixSource8 = "matrixSource8";

inline constexpr auto matrixDest1 = "matrixDest1";
inline constexpr auto matrixDest2 = "matrixDest2";
inline constexpr auto matrixDest3 = "matrixDest3";
inline constexpr auto matrixDest4 = "matrixDest4";
inline constexpr auto matrixDest5 = "matrixDest5";
inline constexpr auto matrixDest6 = "matrixDest6";
inline constexpr auto matrixDest7 = "matrixDest7";
inline constexpr auto matrixDest8 = "matrixDest8";

inline constexpr auto matrixAmount1 = "matrixAmount1";
inline constexpr auto matrixAmount2 = "matrixAmount2";
inline constexpr auto matrixAmount3 = "matrixAmount3";
inline constexpr auto matrixAmount4 = "matrixAmount4";
inline constexpr auto matrixAmount5 = "matrixAmount5";
inline constexpr auto matrixAmount6 = "matrixAmount6";
inline constexpr auto matrixAmount7 = "matrixAmount7";
inline constexpr auto matrixAmount8 = "matrixAmount8";

inline constexpr std::array<const char*, 8> matrixSourceIds {
    matrixSource1, matrixSource2, matrixSource3, matrixSource4,
    matrixSource5, matrixSource6, matrixSource7, matrixSource8
};

inline constexpr std::array<const char*, 8> matrixDestIds {
    matrixDest1, matrixDest2, matrixDest3, matrixDest4,
    matrixDest5, matrixDest6, matrixDest7, matrixDest8
};

inline constexpr std::array<const char*, 8> matrixAmountIds {
    matrixAmount1, matrixAmount2, matrixAmount3, matrixAmount4,
    matrixAmount5, matrixAmount6, matrixAmount7, matrixAmount8
};
} // namespace params

//==============================================================================
enum class Waveform : int
{
    saw = 0,
    square,
    triangle,
    sine
};

enum class FilterMode : int
{
    lowPass12 = 0,
    lowPass24,
    highPass12,
    highPass24,
    bandPass12,
    bandPass24,
    notch12,
    notch24
};

enum class LfoShape : int
{
    sine = 0,
    triangle,
    saw,
    square,
    sampleAndHold
};

enum class ModSource : int
{
    none = 0,
    lfo1,
    lfo2,
    ampEnv,
    filterEnv,
    modEnv,
    velocity,
    modWheel,
    aftertouch,
    keytrack,
    macro1,
    macro2,
    macro3,
    macro4,
    count
};

enum class ModDestination : int
{
    none = 0,
    osc1Pitch,
    osc2Pitch,
    bothPitch,
    osc1Shape,
    osc2Shape,
    filterCutoff,
    filterResonance,
    ampLevel,
    pan,
    chorusMix,
    delayMix,
    reverbMix,
    masterDrive,
    count
};

constexpr int numModSlots = 8;
constexpr int numVoices = 16;

//==============================================================================
juce::StringArray waveformChoices();
juce::StringArray octaveChoices();
juce::StringArray filterModeChoices();
juce::StringArray lfoShapeChoices();
juce::StringArray syncDivisionChoices();
juce::StringArray modSourceChoices();
juce::StringArray modDestinationChoices();
juce::StringArray presetChoices();

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

/** Beats (quarter notes) represented by one cycle of a sync division. */
double beatsPerCycleForDivision(int divisionIndex) noexcept;

/** Human readable delay time for the sync division at a given tempo. */
double syncedDelayMs(int divisionIndex, double bpm) noexcept;

} // namespace synthwave
