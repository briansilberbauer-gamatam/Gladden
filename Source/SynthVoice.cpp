#include "SynthVoice.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace synthwave
{
namespace
{
constexpr float kVoiceHeadroom = 0.30f;
constexpr int kFilterControlInterval = 8;

juce::dsp::ProcessSpec monoSpec (double sampleRate, int blockSize)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) juce::jmax (1, blockSize);
    spec.numChannels = 1;
    return spec;
}
} // namespace

//==============================================================================
SynthVoice::SynthVoice (juce::AudioProcessorValueTreeState& state)
    : parameters (state)
{
    cacheParameterPointers();
}

void SynthVoice::cacheParameterPointers()
{
    auto cache = [this] (std::atomic<float>*& target, const char* id)
    {
        target = parameters.getRawParameterValue (id);
        jassert (target != nullptr);
    };

    cache (refs.osc1Wave, params::osc1Wave);
    cache (refs.osc1Octave, params::osc1Octave);
    cache (refs.osc1Semi, params::osc1Semi);
    cache (refs.osc1Fine, params::osc1Fine);
    cache (refs.osc1Level, params::osc1Level);
    cache (refs.osc1PW, params::osc1PW);
    cache (refs.osc1Sub, params::osc1Sub);

    cache (refs.osc2Wave, params::osc2Wave);
    cache (refs.osc2Octave, params::osc2Octave);
    cache (refs.osc2Semi, params::osc2Semi);
    cache (refs.osc2Fine, params::osc2Fine);
    cache (refs.osc2Level, params::osc2Level);
    cache (refs.osc2PW, params::osc2PW);
    cache (refs.osc2Sub, params::osc2Sub);

    cache (refs.oscSync, params::oscSync);
    cache (refs.ringMod, params::ringMod);
    cache (refs.fmAmount, params::fmAmount);
    cache (refs.fmRatio, params::fmRatio);
    cache (refs.noise, params::noise);

    cache (refs.filterMode, params::filterMode);
    cache (refs.filterCutoff, params::filterCutoff);
    cache (refs.filterResonance, params::filterResonance);
    cache (refs.filterDrive, params::filterDrive);
    cache (refs.filterEnvAmount, params::filterEnvAmount);
    cache (refs.filterKeyTrack, params::filterKeyTrack);
    cache (refs.filterLfo1, params::filterLfo1);
    cache (refs.filterLfo2, params::filterLfo2);

    cache (refs.ampAttack, params::ampAttack);
    cache (refs.ampDecay, params::ampDecay);
    cache (refs.ampSustain, params::ampSustain);
    cache (refs.ampRelease, params::ampRelease);

    cache (refs.filtAttack, params::filtAttack);
    cache (refs.filtDecay, params::filtDecay);
    cache (refs.filtSustain, params::filtSustain);
    cache (refs.filtRelease, params::filtRelease);

    cache (refs.modAttack, params::modAttack);
    cache (refs.modDecay, params::modDecay);
    cache (refs.modSustain, params::modSustain);
    cache (refs.modRelease, params::modRelease);

    for (int i = 0; i < numModSlots; ++i)
    {
        cache (refs.matrixSource[(size_t) i], params::matrixSourceIds[(size_t) i]);
        cache (refs.matrixDest[(size_t) i], params::matrixDestIds[(size_t) i]);
        cache (refs.matrixAmount[(size_t) i], params::matrixAmountIds[(size_t) i]);
    }
}

float SynthVoice::valueOf (const std::atomic<float>* parameter) noexcept
{
    return parameter != nullptr ? parameter->load (std::memory_order_relaxed) : 0.0f;
}

int SynthVoice::intValueOf (const std::atomic<float>* parameter) noexcept
{
    return juce::roundToInt (valueOf (parameter));
}

void SynthVoice::prepare (double newSampleRate, int maximumBlockSize, int)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);

    osc1.prepare (sampleRate);
    osc2.prepare (sampleRate);
    ampEnvelope.prepare (sampleRate);
    filterEnvelope.prepare (sampleRate);
    modEnvelope.prepare (sampleRate);

    const auto spec = monoSpec (sampleRate, maximumBlockSize);
    svfA.prepare (spec);
    svfB.prepare (spec);
    ladderA.prepareVoice (spec);
    ladderB.prepareVoice (spec);

    resetVoice();
}

void SynthVoice::resetVoice()
{
    osc1.reset (0.0);
    osc2.reset (0.25);
    subPhase1 = 0.0;
    subPhase2 = 0.0;
    ampEnvelope.reset();
    filterEnvelope.reset();
    modEnvelope.reset();
    svfA.reset();
    svfB.reset();
    ladderA.reset();
    ladderB.reset();

    filterConfigurationValid = false;
    cutoffInitialised = false;
    filterUpdateCountdown = 0;
    pitchBend = 0.0f;
    clearCurrentNote();
}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float incomingVelocity,
                            juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    if (! canPlaySound (sound))
        return;

    currentNote = juce::jlimit (0, 127, midiNoteNumber);
    noteVelocity = juce::jlimit (0.0f, 1.0f, incomingVelocity);
    velocityGain = 0.15f + 0.85f * std::pow (noteVelocity, 1.4f);
    pitchBend = juce::jlimit (-1.0f, 1.0f,
                             ((float) currentPitchWheelPosition - 8192.0f) / 8192.0f);

    osc1.reset (0.0);
    osc2.reset (0.25);
    subPhase1 = 0.0;
    subPhase2 = 0.0;

    // A small per-note offset avoids bit-identical noise from stacked voices.
    noiseState = 0x9e3779b9u ^ ((uint32_t) currentNote * 2654435761u);

    ampEnvelope.noteOn();
    filterEnvelope.noteOn();
    modEnvelope.noteOn();

    svfA.reset();
    svfB.reset();
    ladderA.reset();
    ladderB.reset();
    filterConfigurationValid = false;
    cutoffInitialised = false;
    filterUpdateCountdown = 0;
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (! allowTailOff)
    {
        ampEnvelope.reset();
        filterEnvelope.reset();
        modEnvelope.reset();
        clearCurrentNote();
        return;
    }

    ampEnvelope.noteOff();
    filterEnvelope.noteOff();
    modEnvelope.noteOff();

    if (! ampEnvelope.isActive())
        clearCurrentNote();
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchBend = juce::jlimit (-1.0f, 1.0f,
                             ((float) newPitchWheelValue - 8192.0f) / 8192.0f);
}

void SynthVoice::controllerMoved (int, int)
{
    // Mod wheel, aftertouch and sustain are aggregated by the processor so
    // every voice sees exactly the same performance state.
}

void SynthVoice::setGlobalModulation (const float* lfo1Samples,
                                      const float* lfo2Samples,
                                      int numGlobalSamples,
                                      float modWheelValue,
                                      float aftertouchValue,
                                      const float* macroValues)
{
    globalLfo1 = lfo1Samples;
    globalLfo2 = lfo2Samples;
    globalLfoSampleCount = numGlobalSamples;
    globalModWheel = juce::jlimit (0.0f, 1.0f, modWheelValue);
    globalAftertouch = juce::jlimit (0.0f, 1.0f, aftertouchValue);

    for (size_t i = 0; i < globalMacros.size(); ++i)
        globalMacros[i] = macroValues != nullptr ? juce::jlimit (0.0f, 1.0f, macroValues[i]) : 0.0f;
}

void SynthVoice::configureFilter (FilterMode mode, float drive) noexcept
{
    const auto driveChanged = std::abs (drive - configuredDrive) > 0.001f;
    if (filterConfigurationValid && mode == configuredFilterMode && ! driveChanged)
        return;

    const auto svfType = [] (FilterMode m) -> juce::dsp::StateVariableTPTFilterType
    {
        if (m == FilterMode::highPass12)
            return juce::dsp::StateVariableTPTFilterType::highpass;
        if (m == FilterMode::bandPass12)
            return juce::dsp::StateVariableTPTFilterType::bandpass;
        return juce::dsp::StateVariableTPTFilterType::lowpass;
    };

    switch (mode)
    {
        case FilterMode::lowPass12:
        case FilterMode::highPass12:
        case FilterMode::bandPass12:
            svfA.setType (svfType (mode));
            break;
        case FilterMode::notch12:
            svfA.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            svfB.setType (juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case FilterMode::lowPass24:
            ladderA.setMode (juce::dsp::LadderFilterMode::LPF24);
            break;
        case FilterMode::highPass24:
            ladderA.setMode (juce::dsp::LadderFilterMode::HPF24);
            break;
        case FilterMode::bandPass24:
            ladderA.setMode (juce::dsp::LadderFilterMode::BPF24);
            break;
        case FilterMode::notch24:
            ladderA.setMode (juce::dsp::LadderFilterMode::LPF24);
            ladderB.setMode (juce::dsp::LadderFilterMode::HPF24);
            break;
        default:
            break;
    }

    const auto ladderDrive = 1.0f + juce::jlimit (0.0f, 1.0f, drive) * 8.0f;
    ladderA.setDrive (ladderDrive);
    ladderB.setDrive (ladderDrive);

    configuredFilterMode = mode;
    configuredDrive = drive;
    filterConfigurationValid = true;
}

void SynthVoice::applyFilterCoefficients (float cutoffHz, float resonance) noexcept
{
    const auto safeCutoff = juce::jlimit (20.0f, (float) (sampleRate * 0.48), cutoffHz);
    const auto safeRes = juce::jlimit (0.0f, 1.0f, resonance);

    const auto svfQ = 0.707f + safeRes * 9.0f;

    if (configuredFilterMode == FilterMode::notch12)
    {
        svfA.setCutoffFrequency (safeCutoff);
        svfB.setCutoffFrequency (safeCutoff);
        svfA.setResonance (svfQ);
        svfB.setResonance (svfQ);
    }
    else if (configuredFilterMode == FilterMode::notch24)
    {
        ladderA.setCutoffFrequencyHz (safeCutoff);
        ladderB.setCutoffFrequencyHz (safeCutoff);
        ladderA.setResonance (safeRes);
        ladderB.setResonance (safeRes);
    }
    else if (configuredFilterMode == FilterMode::lowPass24
          || configuredFilterMode == FilterMode::highPass24
          || configuredFilterMode == FilterMode::bandPass24)
    {
        ladderA.setCutoffFrequencyHz (safeCutoff);
        ladderA.setResonance (safeRes);
    }
    else
    {
        svfA.setCutoffFrequency (safeCutoff);
        svfA.setResonance (svfQ);
    }
}

float SynthVoice::processFilterSample (float input) noexcept
{
    const bool isLadder = configuredFilterMode == FilterMode::lowPass24
                       || configuredFilterMode == FilterMode::highPass24
                       || configuredFilterMode == FilterMode::bandPass24
                       || configuredFilterMode == FilterMode::notch24;

    if (isLadder)
    {
        const auto low = ladderA.processVoiceSample (input);
        if (configuredFilterMode == FilterMode::notch24)
            return low + ladderB.processVoiceSample (input);
        return low;
    }

    const auto drive = juce::jlimit (0.0f, 1.0f, configuredDrive);
    const auto driven = softClip (input * (1.0f + drive * 8.0f)) / (1.0f + drive * 2.2f);
    const auto low = svfA.processSample (0, driven);

    if (configuredFilterMode == FilterMode::notch12)
        return low + svfB.processSample (0, driven);

    return low;
}

float SynthVoice::nextNoise() noexcept
{
    noiseState ^= noiseState << 13;
    noiseState ^= noiseState >> 17;
    noiseState ^= noiseState << 5;
    return (float) ((double) (noiseState & 0x00ffffffu) / 8388608.0 - 1.0);
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                  int startSample, int numSamples)
{
    if (! isVoiceActive() || numSamples <= 0)
        return;

    const int numChannels = outputBuffer.getNumChannels();
    if (numChannels <= 0)
        return;

    auto* left = outputBuffer.getWritePointer (0, startSample);
    auto* right = numChannels > 1 ? outputBuffer.getWritePointer (1, startSample) : nullptr;

    // Per-block parameter snapshot -------------------------------------------------
    const auto w1 = juce::jlimit (0, 3, intValueOf (refs.osc1Wave));
    const auto w2 = juce::jlimit (0, 3, intValueOf (refs.osc2Wave));

    const auto oct1 = intValueOf (refs.osc1Octave) - 2;
    const auto oct2 = intValueOf (refs.osc2Octave) - 2;
    const auto semis1 = valueOf (refs.osc1Semi) + valueOf (refs.osc1Fine) * 0.01f;
    const auto semis2 = valueOf (refs.osc2Semi) + valueOf (refs.osc2Fine) * 0.01f;

    const auto level1 = valueOf (refs.osc1Level);
    const auto level2 = valueOf (refs.osc2Level);
    const auto sub1 = valueOf (refs.osc1Sub);
    const auto sub2 = valueOf (refs.osc2Sub);

    const auto hardSync = valueOf (refs.oscSync) > 0.5f;
    const auto ring = juce::jlimit (0.0f, 1.0f, valueOf (refs.ringMod));
    const auto fm = juce::jlimit (0.0f, 1.0f, valueOf (refs.fmAmount));
    const auto fmRatio = juce::jlimit (0.25f, 8.0f, valueOf (refs.fmRatio));
    const auto noiseLevel = juce::jlimit (0.0f, 1.0f, valueOf (refs.noise));

    const auto mode = (FilterMode) juce::jlimit (0, 7, intValueOf (refs.filterMode));
    const auto baseCutoff = valueOf (refs.filterCutoff);
    const auto baseResonance = juce::jlimit (0.0f, 1.0f, valueOf (refs.filterResonance));
    const auto filterDrive = juce::jlimit (0.0f, 1.0f, valueOf (refs.filterDrive));
    const auto filterEnvAmount = juce::jlimit (-1.0f, 1.0f, valueOf (refs.filterEnvAmount));
    const auto filterKeyTrack = juce::jlimit (0.0f, 1.0f, valueOf (refs.filterKeyTrack));
    const auto filterLfo1Amt = juce::jlimit (-1.0f, 1.0f, valueOf (refs.filterLfo1));
    const auto filterLfo2Amt = juce::jlimit (-1.0f, 1.0f, valueOf (refs.filterLfo2));

    ampEnvelope.setParameters (valueOf (refs.ampAttack), valueOf (refs.ampDecay),
                               valueOf (refs.ampSustain), valueOf (refs.ampRelease));
    filterEnvelope.setParameters (valueOf (refs.filtAttack), valueOf (refs.filtDecay),
                                  valueOf (refs.filtSustain), valueOf (refs.filtRelease));
    modEnvelope.setParameters (valueOf (refs.modAttack), valueOf (refs.modDecay),
                               valueOf (refs.modSustain), valueOf (refs.modRelease));

    osc1.setWaveform (w1);
    osc1.setPulseWidth (valueOf (refs.osc1PW));
    osc2.setWaveform (w2);
    osc2.setPulseWidth (valueOf (refs.osc2PW));

    for (int i = 0; i < numModSlots; ++i)
    {
        const auto sourceIndex = juce::jlimit (0, (int) ModSource::count - 1,
                                               intValueOf (refs.matrixSource[(size_t) i]));
        const auto destIndex = juce::jlimit (0, (int) ModDestination::count - 1,
                                             intValueOf (refs.matrixDest[(size_t) i]));
        matrixRows[(size_t) i] = { (ModSource) sourceIndex,
                                   (ModDestination) destIndex,
                                   juce::jlimit (-1.0f, 1.0f, valueOf (refs.matrixAmount[(size_t) i])) };
    }

    configureFilter (mode, filterDrive);

    // Base tuning ------------------------------------------------------------------
    const auto bendSemitones = pitchBend * 2.0f;
    const auto noteSemis1 = (double) currentNote - 69.0 + bendSemitones
                          + (double) (oct1 * 12) + (double) semis1;
    const auto noteSemis2 = (double) currentNote - 69.0 + bendSemitones
                          + (double) (oct2 * 12) + (double) semis2;

    const auto baseFreq1 = 440.0 * std::exp2 (noteSemis1 / 12.0);
    const auto baseFreq2 = 440.0 * std::exp2 (noteSemis2 / 12.0);
    const auto baseInc1 = juce::jlimit (0.0, 0.48, baseFreq1 / sampleRate);
    const auto baseInc2 = juce::jlimit (0.0, 0.48, baseFreq2 / sampleRate);
    const auto baseSubInc1 = baseInc1 * 0.5;
    const auto baseSubInc2 = baseInc2 * 0.5;

    const auto keytrackSource = juce::jlimit (-1.0f, 1.0f, ((float) currentNote - 60.0f) / 36.0f);
    const auto keytrackOctaves = filterKeyTrack * ((float) currentNote - 60.0f) / 12.0f;
    const auto fmDepth = fm * 7.5f * fmRatio;

    const auto lfoForSample = [this, startSample] (const float* source, int sampleOffset) -> float
    {
        if (source == nullptr || globalLfoSampleCount <= 0)
            return 0.0f;
        const auto index = juce::jlimit (0, globalLfoSampleCount - 1, startSample + sampleOffset);
        return source[index];
    };

    bool finished = false;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto ampEnvValue = ampEnvelope.getNextSample();
        if (! ampEnvelope.isActive() && ampEnvValue <= 0.0f)
        {
            finished = true;
            break;
        }

        const auto filterEnvValue = filterEnvelope.getNextSample();
        const auto modEnvValue = modEnvelope.getNextSample();
        const auto lfo1Value = lfoForSample (globalLfo1, i);
        const auto lfo2Value = lfoForSample (globalLfo2, i);

        std::array<float, (size_t) ModSource::count> sources {};
        sources[(size_t) ModSource::lfo1] = lfo1Value;
        sources[(size_t) ModSource::lfo2] = lfo2Value;
        sources[(size_t) ModSource::ampEnv] = ampEnvValue;
        sources[(size_t) ModSource::filterEnv] = filterEnvValue;
        sources[(size_t) ModSource::modEnv] = modEnvValue;
        sources[(size_t) ModSource::velocity] = noteVelocity;
        sources[(size_t) ModSource::modWheel] = globalModWheel;
        sources[(size_t) ModSource::aftertouch] = globalAftertouch;
        sources[(size_t) ModSource::keytrack] = keytrackSource;
        sources[(size_t) ModSource::macro1] = globalMacros[0];
        sources[(size_t) ModSource::macro2] = globalMacros[1];
        sources[(size_t) ModSource::macro3] = globalMacros[2];
        sources[(size_t) ModSource::macro4] = globalMacros[3];

        std::array<float, (size_t) ModDestination::count> destinations {};
        for (const auto& row : matrixRows)
        {
            if (row.source == ModSource::none || row.destination == ModDestination::none
                || std::abs (row.amount) < 1.0e-6f)
                continue;

            destinations[(size_t) row.destination]
                += sources[(size_t) row.source] * row.amount;
        }

        // Pitch modulation (semitones) ---------------------------------------------
        const auto pitch1 = destinations[(size_t) ModDestination::osc1Pitch] * 24.0f
                          + destinations[(size_t) ModDestination::bothPitch] * 24.0f;
        const auto pitch2 = destinations[(size_t) ModDestination::osc2Pitch] * 24.0f
                          + destinations[(size_t) ModDestination::bothPitch] * 24.0f;

        const auto inc1 = baseInc1 * std::exp2 (pitch1 / 12.0f);
        const auto inc2 = baseInc2 * std::exp2 (pitch2 / 12.0f);

        osc1.setPhaseIncrement (inc1);
        bool osc1Wrapped = false;
        const auto osc1Sample = osc1.process (osc1Wrapped);

        if (hardSync && osc1Wrapped)
            osc2.reset (0.0);

        const auto fmFactor = juce::jlimit (0.02, 16.0, 1.0 + (double) fmDepth * (double) osc1Sample);
        osc2.setPhaseIncrement (inc2 * fmFactor);
        bool osc2Wrapped = false;
        auto osc2Sample = osc2.process (osc2Wrapped);

        if (ring > 0.0f)
            osc2Sample = juce::jmap (ring, osc2Sample, osc1Sample * osc2Sample);

        // Sine sub oscillators, one octave below each oscillator.
        subPhase1 += baseSubInc1 * std::exp2 (pitch1 / 12.0f);
        subPhase2 += baseSubInc2 * std::exp2 (pitch2 / 12.0f);
        if (subPhase1 >= 1.0)
            subPhase1 -= std::floor (subPhase1);
        if (subPhase2 >= 1.0)
            subPhase2 -= std::floor (subPhase2);

        const auto subSample1 = (float) std::sin (juce::MathConstants<double>::twoPi * subPhase1);
        const auto subSample2 = (float) std::sin (juce::MathConstants<double>::twoPi * subPhase2);

        auto oscillatorMix = osc1Sample * level1
                           + osc2Sample * level2
                           + subSample1 * sub1
                           + subSample2 * sub2
                           + nextNoise() * noiseLevel;

        // Filter modulation ----------------------------------------------------------
        auto cutoffOctaves = filterEnvValue * filterEnvAmount * 6.0f
                           + keytrackOctaves
                           + lfo1Value * filterLfo1Amt * 4.0f
                           + lfo2Value * filterLfo2Amt * 4.0f
                           + destinations[(size_t) ModDestination::filterCutoff] * 6.0f
                           + globalMacros[0] * 3.0f;

        const auto targetCutoff = juce::jlimit (20.0f, (float) (sampleRate * 0.47),
                                                baseCutoff * std::exp2 (cutoffOctaves));

        if (! cutoffInitialised)
        {
            smoothedCutoffHz = targetCutoff;
            filterResonanceSmoothed = juce::jlimit (0.0f, 1.0f,
                baseResonance + destinations[(size_t) ModDestination::filterResonance] * 0.5f);
            cutoffInitialised = true;
        }
        else
        {
            smoothedCutoffHz += (targetCutoff - smoothedCutoffHz) * 0.025f;
        }

        const auto targetResonance = juce::jlimit (
            0.0f, 1.0f,
            baseResonance + destinations[(size_t) ModDestination::filterResonance] * 0.5f);
        filterResonanceSmoothed += (targetResonance - filterResonanceSmoothed) * 0.02f;

        if (filterUpdateCountdown <= 0)
        {
            applyFilterCoefficients (smoothedCutoffHz, filterResonanceSmoothed);
            filterUpdateCountdown = kFilterControlInterval;
        }
        --filterUpdateCountdown;

        const auto filtered = processFilterSample (oscillatorMix);

        // Amp / pan / output ---------------------------------------------------------
        const auto ampMod = juce::jlimit (0.0f, 2.0f,
                                          1.0f + destinations[(size_t) ModDestination::ampLevel] * 2.0f);
        const auto pan = juce::jlimit (-1.0f, 1.0f, destinations[(size_t) ModDestination::pan]);
        const auto panAngle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        const auto panLeft = std::cos (panAngle);
        const auto panRight = std::sin (panAngle);
        const auto voiceOut = filtered * ampEnvValue * velocityGain * ampMod * kVoiceHeadroom;

        if (right != nullptr)
        {
            left[i] += voiceOut * panLeft;
            right[i] += voiceOut * panRight;
        }
        else
        {
            left[i] += voiceOut * 0.7071f;
        }

    }


    svfA.snapToZero();
    svfB.snapToZero();

    if (finished)
    {
        ampEnvelope.reset();
        filterEnvelope.reset();
        modEnvelope.reset();
        clearCurrentNote();
    }
}

} // namespace synthwave
