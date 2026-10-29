#include "Presets.h"
#include "Parameters.h"

#include <array>
#include <vector>

namespace synthwave
{
namespace
{
struct PresetValue
{
    const char* id;
    float value;
};

struct PresetDefinition
{
    const char* name;
    std::vector<PresetValue> values;
};

const std::vector<PresetDefinition>& factoryPresets()
{
    static const std::vector<PresetDefinition> presets {
        { "Init Patch", {} },

        { "Neon Runner", {
            { params::osc1Wave, 0 }, { params::osc1Octave, 2 }, { params::osc1Level, 0.85f },
            { params::osc1Sub, 0.20f }, { params::osc2Wave, 1 }, { params::osc2Octave, 1 },
            { params::osc2Semi, 0 }, { params::osc2Fine, 7.0f }, { params::osc2Level, 0.55f },
            { params::osc2PW, 0.42f }, { params::filterMode, 1 }, { params::filterCutoff, 5200.0f },
            { params::filterResonance, 0.35f }, { params::filterDrive, 0.28f },
            { params::filterEnvAmount, 0.40f }, { params::filterKeyTrack, 0.40f },
            { params::ampAttack, 0.004f }, { params::ampDecay, 0.40f }, { params::ampSustain, 0.65f },
            { params::ampRelease, 0.45f }, { params::filtAttack, 0.003f }, { params::filtDecay, 0.35f },
            { params::filtSustain, 0.20f }, { params::filtRelease, 0.35f },
            { params::lfo1Rate, 1.3f }, { params::lfo1Shape, 0 }, { params::lfo2Rate, 0.18f },
            { params::chorusOn, 1 }, { params::chorusMix, 0.32f }, { params::chorusRate, 0.45f },
            { params::chorusDepth, 0.45f }, { params::delayOn, 1 }, { params::delaySync, 1 },
            { params::delayDivision, 2 }, { params::delayFeedback, 0.34f }, { params::delayMix, 0.28f },
            { params::delayPingPong, 1 }, { params::reverbOn, 1 }, { params::reverbSize, 0.55f },
            { params::reverbDamping, 0.45f }, { params::reverbMix, 0.22f },
            { params::eqHigh, 2.0f }, { params::masterGain, -7.0f },
            { params::matrixSource1, 1 }, { params::matrixDest1, 6 }, { params::matrixAmount1, 0.12f },
            { params::matrixSource2, 7 }, { params::matrixDest2, 4 }, { params::matrixAmount2, 0.15f }
        } },

        { "Midnight Chase", {
            { params::osc1Wave, 1 }, { params::osc1PW, 0.32f }, { params::osc1Level, 0.75f },
            { params::osc2Wave, 0 }, { params::osc2Octave, 1 }, { params::osc2Fine, -4.0f },
            { params::osc2Level, 0.60f }, { params::osc1Sub, 0.35f }, { params::filterMode, 1 },
            { params::filterCutoff, 900.0f }, { params::filterResonance, 0.48f },
            { params::filterDrive, 0.40f }, { params::filterEnvAmount, 0.55f },
            { params::filterKeyTrack, 0.60f }, { params::ampAttack, 0.002f },
            { params::ampDecay, 0.18f }, { params::ampSustain, 0.40f }, { params::ampRelease, 0.25f },
            { params::filtAttack, 0.001f }, { params::filtDecay, 0.22f }, { params::filtSustain, 0.0f },
            { params::filtRelease, 0.20f }, { params::lfo2Rate, 0.10f }, { params::lfo2Shape, 1 },
            { params::chorusOn, 1 }, { params::chorusMix, 0.15f }, { params::delayOn, 1 },
            { params::delaySync, 1 }, { params::delayDivision, 3 }, { params::delayFeedback, 0.45f },
            { params::delayMix, 0.35f }, { params::delayPingPong, 1 }, { params::reverbMix, 0.15f },
            { params::reverbSize, 0.35f }, { params::masterGain, -7.0f },
            { params::matrixSource1, 2 }, { params::matrixDest1, 6 }, { params::matrixAmount1, 0.10f },
            { params::matrixSource2, 7 }, { params::matrixDest2, 7 }, { params::matrixAmount2, 0.12f }
        } },

        { "Blade Horizon", {
            { params::osc1Wave, 0 }, { params::osc1Level, 0.60f }, { params::osc2Wave, 0 },
            { params::osc2Octave, 2 }, { params::osc2Fine, 12.0f }, { params::osc2Level, 0.60f },
            { params::osc1Sub, 0.10f }, { params::noise, 0.02f }, { params::filterMode, 1 },
            { params::filterCutoff, 1800.0f }, { params::filterResonance, 0.20f },
            { params::filterDrive, 0.10f }, { params::filterEnvAmount, 0.15f },
            { params::filterKeyTrack, 0.60f }, { params::ampAttack, 0.80f }, { params::ampDecay, 1.20f },
            { params::ampSustain, 0.85f }, { params::ampRelease, 2.50f }, { params::filtAttack, 0.90f },
            { params::filtDecay, 1.50f }, { params::filtSustain, 0.60f }, { params::filtRelease, 2.0f },
            { params::lfo1Rate, 0.12f }, { params::lfo1Shape, 0 }, { params::lfo2Rate, 0.07f },
            { params::lfo2Shape, 1 }, { params::chorusOn, 1 }, { params::chorusRate, 0.18f },
            { params::chorusDepth, 0.70f }, { params::chorusMix, 0.48f }, { params::chorusFeedback, 0.12f },
            { params::delayOn, 0 }, { params::reverbOn, 1 }, { params::reverbSize, 0.80f },
            { params::reverbDamping, 0.35f }, { params::reverbWidth, 1.0f }, { params::reverbMix, 0.42f },
            { params::eqLow, 1.0f }, { params::eqHigh, 1.0f }, { params::masterGain, -8.0f },
            { params::matrixSource1, 1 }, { params::matrixDest1, 3 }, { params::matrixAmount1, 0.04f },
            { params::matrixSource2, 7 }, { params::matrixDest2, 6 }, { params::matrixAmount2, 0.20f }
        } },

        { "Outrun Sunset", {
            { params::osc1Wave, 0 }, { params::osc1Level, 0.80f }, { params::osc2Wave, 0 },
            { params::osc2Octave, 3 }, { params::osc2Level, 0.35f }, { params::osc1Sub, 0.05f },
            { params::oscSync, 1 }, { params::fmAmount, 0.12f }, { params::fmRatio, 2.0f },
            { params::filterMode, 1 }, { params::filterCutoff, 3800.0f }, { params::filterResonance, 0.30f },
            { params::filterDrive, 0.35f }, { params::filterEnvAmount, 0.45f }, { params::filterKeyTrack, 0.30f },
            { params::ampAttack, 0.003f }, { params::ampDecay, 0.35f }, { params::ampSustain, 0.72f },
            { params::ampRelease, 0.35f }, { params::filtAttack, 0.002f }, { params::filtDecay, 0.30f },
            { params::filtSustain, 0.25f }, { params::filtRelease, 0.30f },
            { params::lfo1Rate, 5.2f }, { params::lfo1Shape, 0 }, { params::chorusOn, 1 },
            { params::chorusMix, 0.30f }, { params::delayOn, 1 }, { params::delaySync, 1 },
            { params::delayDivision, 4 }, { params::delayFeedback, 0.40f }, { params::delayMix, 0.30f },
            { params::delayPingPong, 1 }, { params::reverbMix, 0.20f }, { params::reverbSize, 0.45f },
            { params::masterGain, -7.0f }, { params::matrixSource1, 1 }, { params::matrixDest1, 3 },
            { params::matrixAmount1, 0.025f }, { params::matrixSource2, 7 }, { params::matrixDest2, 6 },
            { params::matrixAmount2, 0.35f }
        } },

        { "Glass City", {
            { params::osc1Wave, 3 }, { params::osc1Level, 0.70f }, { params::osc2Wave, 2 },
            { params::osc2Octave, 3 }, { params::osc2Semi, 7 }, { params::osc2Level, 0.42f },
            { params::ringMod, 0.35f }, { params::fmAmount, 0.22f }, { params::fmRatio, 3.5f },
            { params::filterMode, 0 }, { params::filterCutoff, 9000.0f }, { params::filterResonance, 0.12f },
            { params::filterDrive, 0.05f }, { params::filterEnvAmount, 0.10f }, { params::filterKeyTrack, 0.80f },
            { params::ampAttack, 0.002f }, { params::ampDecay, 1.40f }, { params::ampSustain, 0.0f },
            { params::ampRelease, 1.60f }, { params::filtAttack, 0.002f }, { params::filtDecay, 0.90f },
            { params::filtSustain, 0.0f }, { params::filtRelease, 1.20f },
            { params::modAttack, 0.002f }, { params::modDecay, 0.5f }, { params::modSustain, 0.0f },
            { params::modRelease, 0.6f }, { params::lfo1Rate, 0.14f }, { params::lfo2Rate, 3.1f },
            { params::chorusOn, 1 }, { params::chorusMix, 0.20f }, { params::delayOn, 1 },
            { params::delaySync, 1 }, { params::delayDivision, 6 }, { params::delayFeedback, 0.30f },
            { params::delayMix, 0.28f }, { params::delayPingPong, 1 }, { params::reverbOn, 1 },
            { params::reverbSize, 0.70f }, { params::reverbDamping, 0.25f }, { params::reverbWidth, 1.0f },
            { params::reverbMix, 0.45f }, { params::eqHigh, 3.0f }, { params::masterGain, -8.0f },
            { params::matrixSource1, 3 }, { params::matrixDest1, 5 }, { params::matrixAmount1, 0.10f },
            { params::matrixSource2, 1 }, { params::matrixDest2, 3 }, { params::matrixAmount2, 0.02f }
        } },

        { "Analog Dream", {
            { params::osc1Wave, 0 }, { params::osc1Level, 0.72f }, { params::osc2Wave, 0 },
            { params::osc2Octave, 2 }, { params::osc2Fine, -9.0f }, { params::osc2Level, 0.55f },
            { params::osc1Sub, 0.12f }, { params::filterMode, 1 }, { params::filterCutoff, 2400.0f },
            { params::filterResonance, 0.18f }, { params::filterDrive, 0.18f },
            { params::filterEnvAmount, 0.18f }, { params::filterKeyTrack, 0.45f },
            { params::ampAttack, 0.02f }, { params::ampDecay, 0.5f }, { params::ampSustain, 0.75f },
            { params::ampRelease, 1.1f }, { params::filtAttack, 0.02f }, { params::filtDecay, 0.8f },
            { params::filtSustain, 0.4f }, { params::filtRelease, 0.8f },
            { params::lfo1Rate, 0.22f }, { params::lfo1Shape, 0 }, { params::lfo2Rate, 0.09f },
            { params::lfo2Shape, 1 }, { params::chorusOn, 1 }, { params::chorusRate, 0.28f },
            { params::chorusDepth, 0.55f }, { params::chorusMix, 0.35f }, { params::delayOn, 0 },
            { params::reverbOn, 1 }, { params::reverbSize, 0.50f }, { params::reverbDamping, 0.55f },
            { params::reverbMix, 0.25f }, { params::eqLow, 1.5f }, { params::eqHigh, -0.5f },
            { params::masterGain, -7.0f }, { params::matrixSource1, 1 }, { params::matrixDest1, 3 },
            { params::matrixAmount1, 0.02f }
        } },

        { "Sub Zero", {
            { params::osc1Wave, 0 }, { params::osc1Level, 0.65f }, { params::osc2Wave, 1 },
            { params::osc2Octave, 1 }, { params::osc2Level, 0.50f }, { params::osc1Sub, 0.65f },
            { params::filterMode, 1 }, { params::filterCutoff, 600.0f }, { params::filterResonance, 0.30f },
            { params::filterDrive, 0.50f }, { params::filterEnvAmount, 0.55f }, { params::filterKeyTrack, 0.50f },
            { params::ampAttack, 0.002f }, { params::ampDecay, 0.22f }, { params::ampSustain, 0.55f },
            { params::ampRelease, 0.18f }, { params::filtAttack, 0.001f }, { params::filtDecay, 0.20f },
            { params::filtSustain, 0.10f }, { params::filtRelease, 0.20f },
            { params::chorusOn, 0 }, { params::delayOn, 0 }, { params::reverbOn, 1 },
            { params::reverbSize, 0.22f }, { params::reverbMix, 0.08f }, { params::masterDrive, 0.15f },
            { params::masterGain, -5.0f }, { params::matrixSource1, 7 }, { params::matrixDest1, 6 },
            { params::matrixAmount1, 0.10f }
        } }
    };

    return presets;
}

void setParameter (juce::AudioProcessorValueTreeState& state, const char* id, float value)
{
    if (auto* parameter = state.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    else
        jassertfalse;
}
} // namespace

//==============================================================================
juce::StringArray presetNames()
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets())
        names.add (preset.name);
    return names;
}

void applyFactoryPreset (juce::AudioProcessorValueTreeState& state, int presetIndex)
{
    const auto& presets = factoryPresets();

    // Start from a known state so partial presets remain deterministic.
    for (auto* parameter : state.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            if (ranged->paramID != params::preset)
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

    if (! juce::isPositiveAndBelow (presetIndex, (int) presets.size()))
        return;

    for (const auto& value : presets[(size_t) presetIndex].values)
        setParameter (state, value.id, value.value);

    setParameter (state, params::preset, (float) presetIndex);
}

} // namespace synthwave
