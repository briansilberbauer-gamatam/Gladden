#include "Parameters.h"
#include "Presets.h"

#include <array>
#include <cmath>

namespace synthwave
{
namespace
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using Attributes = juce::AudioParameterFloatAttributes;

juce::String formatPercent (float value, int)
{
    return juce::String (juce::roundToInt (value * 100.0f)) + "%";
}

juce::String formatSignedPercent (float value, int)
{
    return (value >= 0.0f ? "+" : "") + juce::String (juce::roundToInt (value * 100.0f)) + "%";
}

juce::String formatDb (float value, int)
{
    return (value >= 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
}

juce::String formatHz (float value, int)
{
    if (value >= 1000.0f)
        return juce::String (value / 1000.0f, value >= 10000.0f ? 1 : 2) + " kHz";
    if (value >= 100.0f)
        return juce::String (juce::roundToInt (value)) + " Hz";
    if (value >= 10.0f)
        return juce::String (value, 1) + " Hz";
    return juce::String (value, 2) + " Hz";
}

juce::String formatMs (float value, int)
{
    return juce::String (juce::roundToInt (value)) + " ms";
}

juce::String formatCents (float value, int)
{
    return (value >= 0.0f ? "+" : "") + juce::String (juce::roundToInt (value)) + " ct";
}

juce::String formatSeconds (float value, int)
{
    if (value < 1.0f)
        return juce::String (juce::roundToInt (value * 1000.0f)) + " ms";
    return juce::String (value, 2) + " s";
}

juce::String formatRatio (float value, int)
{
    return juce::String (value, 2) + "x";
}

std::unique_ptr<juce::RangedAudioParameter> makeFloat (const char* id,
                                                       const juce::String& name,
                                                       float minValue,
                                                       float maxValue,
                                                       float defaultValue,
                                                       float centreValue,
                                                       const juce::String& label,
                                                       std::function<juce::String (float, int)> formatter,
                                                       float interval = 0.0f)
{
    juce::NormalisableRange<float> range (minValue, maxValue, interval);
    if (centreValue > minValue && centreValue < maxValue)
        range.setSkewForCentre (centreValue);

    auto attributes = Attributes().withLabel (label)
                                  .withStringFromValueFunction (std::move (formatter));
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                        range, defaultValue, attributes);
}

std::unique_ptr<juce::RangedAudioParameter> makeChoice (const char* id,
                                                        const juce::String& name,
                                                        const juce::StringArray& choices,
                                                        int defaultIndex,
                                                        bool automatable = true)
{
    auto attributes = juce::AudioParameterChoiceAttributes().withAutomatable (automatable);
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name,
                                                         choices, defaultIndex, attributes);
}

std::unique_ptr<juce::RangedAudioParameter> makeBool (const char* id,
                                                      const juce::String& name,
                                                      bool defaultValue)
{
    // JUCE's AudioParameterBool deliberately stores arbitrary normalised host
    // values, which makes state-restoration validation ambiguous for discrete
    // toggles. A two-choice parameter behaves identically in the UI/DSP (raw
    // values are 0 or 1) while keeping deterministic host automation semantics.
    auto attributes = juce::AudioParameterChoiceAttributes().withAutomatable (true);
    return std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id, 1 }, name, juce::StringArray { "Off", "On" },
        defaultValue ? 1 : 0, attributes);
}
} // namespace

//==============================================================================
juce::StringArray waveformChoices()
{
    return { "Saw", "Square / Pulse", "Triangle", "Sine" };
}

juce::StringArray octaveChoices()
{
    return { "-2 Oct", "-1 Oct", "0 Oct", "+1 Oct", "+2 Oct" };
}

juce::StringArray filterModeChoices()
{
    return { "LP 12 dB", "LP 24 dB",
             "HP 12 dB", "HP 24 dB",
             "BP 12 dB", "BP 24 dB",
             "Notch 12 dB", "Notch 24 dB" };
}

juce::StringArray lfoShapeChoices()
{
    return { "Sine", "Triangle", "Saw", "Square", "Sample & Hold" };
}

juce::StringArray syncDivisionChoices()
{
    return { "1/32", "1/16T", "1/16", "1/8T", "1/8",
             "1/4T", "1/4", "1/2", "1 Bar", "2 Bars" };
}

juce::StringArray modSourceChoices()
{
    return { "None", "LFO 1", "LFO 2", "Amp Env", "Filter Env", "Mod Env",
             "Velocity", "Mod Wheel", "Aftertouch", "Keytrack",
             "Macro 1", "Macro 2", "Macro 3", "Macro 4" };
}

juce::StringArray modDestinationChoices()
{
    return { "None", "Osc 1 Pitch", "Osc 2 Pitch", "Both Pitch",
             "Osc 1 Shape", "Osc 2 Shape", "Filter Cutoff", "Filter Resonance",
             "Mixer Volume", "Pan", "Chorus Mix", "Delay Mix", "Reverb Mix",
             "Master Drive" };
}

juce::StringArray presetChoices()
{
    return presetNames();
}

double beatsPerCycleForDivision (int divisionIndex) noexcept
{
    // Values are in quarter-note beats.
    static constexpr std::array<double, 10> beats {
        0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.5,
        2.0 / 3.0, 1.0, 2.0, 4.0, 8.0
    };

    const auto index = juce::jlimit (0, (int) beats.size() - 1, divisionIndex);
    return beats[(size_t) index];
}

double syncedDelayMs (int divisionIndex, double bpm) noexcept
{
    const auto safeBpm = juce::jlimit (20.0, 400.0, bpm);
    return beatsPerCycleForDivision (divisionIndex) * 60000.0 / safeBpm;
}

//==============================================================================
Layout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.reserve (112);

    auto add = [&parameters] (std::unique_ptr<juce::RangedAudioParameter> p)
    {
        parameters.push_back (std::move (p));
    };

    // Performance / global -----------------------------------------------------
    add (makeChoice (params::preset, "Preset", presetChoices(), 0, false));

    for (int i = 1; i <= 4; ++i)
        add (makeFloat (i == 1 ? params::macro1 : i == 2 ? params::macro2
                                                         : i == 3 ? params::macro3 : params::macro4,
                        "Macro " + juce::String (i), 0.0f, 1.0f, 0.0f, 0.0f,
                        "%", formatPercent));

    // Oscillator 1 -------------------------------------------------------------
    add (makeChoice (params::osc1Wave, "Osc 1 Wave", waveformChoices(), 0));
    add (makeChoice (params::osc1Octave, "Osc 1 Octave", octaveChoices(), 2));
    add (makeFloat (params::osc1Semi, "Osc 1 Semitone", -12.0f, 12.0f, 0.0f, 0.0f,
                    "st", [] (float v, int) { return juce::String ((int) std::round (v)) + " st"; }, 1.0f));
    add (makeFloat (params::osc1Fine, "Osc 1 Fine", -100.0f, 100.0f, 0.0f, 0.0f,
                    "ct", formatCents));
    add (makeFloat (params::osc1Level, "Osc 1 Level", 0.0f, 1.0f, 0.78f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::osc1PW, "Osc 1 Pulse Width", 0.05f, 0.95f, 0.5f, 0.5f,
                    "%", formatPercent));
    add (makeFloat (params::osc1Sub, "Osc 1 Sub", 0.0f, 1.0f, 0.15f, 0.0f,
                    "%", formatPercent));

    // Oscillator 2 -------------------------------------------------------------
    add (makeChoice (params::osc2Wave, "Osc 2 Wave", waveformChoices(), 0));
    add (makeChoice (params::osc2Octave, "Osc 2 Octave", octaveChoices(), 1));
    add (makeFloat (params::osc2Semi, "Osc 2 Semitone", -12.0f, 12.0f, 0.0f, 0.0f,
                    "st", [] (float v, int) { return juce::String ((int) std::round (v)) + " st"; }, 1.0f));
    add (makeFloat (params::osc2Fine, "Osc 2 Fine", -100.0f, 100.0f, 6.0f, 0.0f,
                    "ct", formatCents));
    add (makeFloat (params::osc2Level, "Osc 2 Level", 0.0f, 1.0f, 0.42f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::osc2PW, "Osc 2 Pulse Width", 0.05f, 0.95f, 0.5f, 0.5f,
                    "%", formatPercent));
    add (makeFloat (params::osc2Sub, "Osc 2 Sub", 0.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatPercent));

    // Oscillator interaction ---------------------------------------------------
    add (makeBool (params::oscSync, "Hard Sync", false));
    add (makeFloat (params::ringMod, "Ring Modulation", 0.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::fmAmount, "FM Amount", 0.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::fmRatio, "FM Ratio", 0.25f, 8.0f, 2.0f, 2.0f,
                    "x", formatRatio));
    add (makeFloat (params::noise, "Noise", 0.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatPercent));

    // Filter -------------------------------------------------------------------
    add (makeChoice (params::filterMode, "Filter Mode", filterModeChoices(), 1));
    add (makeFloat (params::filterCutoff, "Filter Cutoff", 20.0f, 20000.0f, 2800.0f, 900.0f,
                    "Hz", formatHz));
    add (makeFloat (params::filterResonance, "Filter Resonance", 0.0f, 1.0f, 0.22f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::filterDrive, "Filter Drive", 0.0f, 1.0f, 0.15f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::filterEnvAmount, "Filter Env Amount", -1.0f, 1.0f, 0.25f, 0.0f,
                    "%", formatSignedPercent));
    add (makeFloat (params::filterKeyTrack, "Filter Keytrack", 0.0f, 1.0f, 0.35f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::filterLfo1, "Filter LFO 1", -1.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatSignedPercent));
    add (makeFloat (params::filterLfo2, "Filter LFO 2", -1.0f, 1.0f, 0.0f, 0.0f,
                    "%", formatSignedPercent));

    // Amp envelope -------------------------------------------------------------
    add (makeFloat (params::ampAttack, "Amp Attack", 0.001f, 10.0f, 0.008f, 0.08f,
                    "s", formatSeconds));
    add (makeFloat (params::ampDecay, "Amp Decay", 0.001f, 10.0f, 0.3f, 0.3f,
                    "s", formatSeconds));
    add (makeFloat (params::ampSustain, "Amp Sustain", 0.0f, 1.0f, 0.8f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::ampRelease, "Amp Release", 0.003f, 15.0f, 0.45f, 0.35f,
                    "s", formatSeconds));

    // Filter envelope ----------------------------------------------------------
    add (makeFloat (params::filtAttack, "Filter Attack", 0.001f, 10.0f, 0.005f, 0.08f,
                    "s", formatSeconds));
    add (makeFloat (params::filtDecay, "Filter Decay", 0.001f, 10.0f, 0.5f, 0.3f,
                    "s", formatSeconds));
    add (makeFloat (params::filtSustain, "Filter Sustain", 0.0f, 1.0f, 0.35f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::filtRelease, "Filter Release", 0.003f, 15.0f, 0.4f, 0.35f,
                    "s", formatSeconds));

    // Mod envelope -------------------------------------------------------------
    add (makeFloat (params::modAttack, "Mod Attack", 0.001f, 10.0f, 0.01f, 0.08f,
                    "s", formatSeconds));
    add (makeFloat (params::modDecay, "Mod Decay", 0.001f, 10.0f, 0.4f, 0.3f,
                    "s", formatSeconds));
    add (makeFloat (params::modSustain, "Mod Sustain", 0.0f, 1.0f, 0.2f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::modRelease, "Mod Release", 0.003f, 15.0f, 0.4f, 0.35f,
                    "s", formatSeconds));

    // LFOs ---------------------------------------------------------------------
    add (makeFloat (params::lfo1Rate, "LFO 1 Rate", 0.01f, 20.0f, 0.7f, 1.0f,
                    "Hz", formatHz));
    add (makeChoice (params::lfo1Shape, "LFO 1 Shape", lfoShapeChoices(), 0));
    add (makeBool (params::lfo1Sync, "LFO 1 Sync", false));
    add (makeChoice (params::lfo1Division, "LFO 1 Division", syncDivisionChoices(), 4));

    add (makeFloat (params::lfo2Rate, "LFO 2 Rate", 0.01f, 20.0f, 0.25f, 1.0f,
                    "Hz", formatHz));
    add (makeChoice (params::lfo2Shape, "LFO 2 Shape", lfoShapeChoices(), 1));
    add (makeBool (params::lfo2Sync, "LFO 2 Sync", false));
    add (makeChoice (params::lfo2Division, "LFO 2 Division", syncDivisionChoices(), 4));

    // Chorus -------------------------------------------------------------------
    add (makeBool (params::chorusOn, "Chorus On", true));
    add (makeFloat (params::chorusRate, "Chorus Rate", 0.05f, 8.0f, 0.35f, 0.5f,
                    "Hz", formatHz));
    add (makeFloat (params::chorusDepth, "Chorus Depth", 0.0f, 1.0f, 0.4f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::chorusMix, "Chorus Mix", 0.0f, 1.0f, 0.28f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::chorusFeedback, "Chorus Feedback", -0.95f, 0.95f, 0.0f, 0.0f,
                    "%", formatSignedPercent));

    // Delay --------------------------------------------------------------------
    add (makeBool (params::delayOn, "Delay On", false));
    add (makeBool (params::delaySync, "Delay Sync", true));
    add (makeChoice (params::delayDivision, "Delay Division", syncDivisionChoices(), 4));
    add (makeFloat (params::delayTime, "Delay Time", 10.0f, 2000.0f, 375.0f, 350.0f,
                    "ms", formatMs));
    add (makeFloat (params::delayFeedback, "Delay Feedback", 0.0f, 0.95f, 0.32f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::delayMix, "Delay Mix", 0.0f, 1.0f, 0.22f, 0.0f,
                    "%", formatPercent));
    add (makeBool (params::delayPingPong, "Delay Ping-Pong", true));
    add (makeFloat (params::delayTone, "Delay Tone", 200.0f, 18000.0f, 5500.0f, 3000.0f,
                    "Hz", formatHz));

    // Reverb -------------------------------------------------------------------
    add (makeBool (params::reverbOn, "Reverb On", true));
    add (makeFloat (params::reverbSize, "Reverb Size", 0.0f, 1.0f, 0.45f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::reverbDamping, "Reverb Damping", 0.0f, 1.0f, 0.5f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::reverbWidth, "Reverb Width", 0.0f, 1.0f, 1.0f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.18f, 0.0f,
                    "%", formatPercent));

    // Master -------------------------------------------------------------------
    add (makeFloat (params::masterDrive, "Master Drive", 0.0f, 1.0f, 0.05f, 0.0f,
                    "%", formatPercent));
    add (makeFloat (params::eqLow, "EQ Low", -12.0f, 12.0f, 0.0f, 0.0f,
                    "dB", formatDb));
    add (makeFloat (params::eqMid, "EQ Mid", -12.0f, 12.0f, 0.0f, 0.0f,
                    "dB", formatDb));
    add (makeFloat (params::eqHigh, "EQ High", -12.0f, 12.0f, 0.0f, 0.0f,
                    "dB", formatDb));
    add (makeFloat (params::masterGain, "Master Gain", -24.0f, 6.0f, -6.0f, 0.0f,
                    "dB", formatDb));

    // Modulation matrix --------------------------------------------------------
    for (int i = 0; i < numModSlots; ++i)
    {
        const auto slot = juce::String (i + 1);
        add (makeChoice (params::matrixSourceIds[(size_t) i], "Matrix " + slot + " Source",
                         modSourceChoices(), 0));
        add (makeChoice (params::matrixDestIds[(size_t) i], "Matrix " + slot + " Destination",
                         modDestinationChoices(), 0));
        add (makeFloat (params::matrixAmountIds[(size_t) i], "Matrix " + slot + " Amount",
                        -1.0f, 1.0f, 0.0f, 0.0f, "%", formatSignedPercent));
    }

    return Layout (parameters.begin(), parameters.end());
}

} // namespace synthwave
