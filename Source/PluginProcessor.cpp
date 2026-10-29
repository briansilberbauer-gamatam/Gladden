#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

#include <algorithm>
#include <cmath>

namespace synthwave
{
namespace
{
constexpr int kNumFactoryPresets = 8;

juce::dsp::ProcessSpec makeSpec (double sampleRate, int blockSize, int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) juce::jmax (1, blockSize);
    spec.numChannels = (juce::uint32) juce::jmax (1, numChannels);
    return spec;
}

float sourceValueForFx (ModSource source, float lfo1Value, float lfo2Value,
                        float modWheel, float aftertouch, const std::array<float, 4>& macros)
{
    switch (source)
    {
        case ModSource::lfo1:       return lfo1Value;
        case ModSource::lfo2:       return lfo2Value;
        case ModSource::modWheel:   return modWheel;
        case ModSource::aftertouch: return aftertouch;
        case ModSource::macro1:     return macros[0];
        case ModSource::macro2:     return macros[1];
        case ModSource::macro3:     return macros[2];
        case ModSource::macro4:     return macros[3];
        case ModSource::none:
        case ModSource::ampEnv:
        case ModSource::filterEnv:
        case ModSource::modEnv:
        case ModSource::velocity:
        case ModSource::keytrack:
        case ModSource::count:
        default:                    return 0.0f;
    }
}
} // namespace

//==============================================================================
SynthwaveLegacyProcessor::SynthwaveLegacyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numVoices; ++i)
        synthesiser.addVoice (new SynthVoice (apvts));

    synthesiser.addSound (new SynthSound());
    synthesiser.setNoteStealingEnabled (true);

    cacheParameterPointers();

    bpmForUi.store (120.0f);
    peakLeft.store (0.0f);
    peakRight.store (0.0f);

    setLatencySamples (0);
}

SynthwaveLegacyProcessor::~SynthwaveLegacyProcessor() = default;

void SynthwaveLegacyProcessor::cacheParameterPointers()
{
    auto cache = [this] (std::atomic<float>*& target, const char* id)
    {
        target = apvts.getRawParameterValue (id);
        jassert (target != nullptr);
    };

    cache (refs.lfo1Rate, params::lfo1Rate);
    cache (refs.lfo1Shape, params::lfo1Shape);
    cache (refs.lfo1Sync, params::lfo1Sync);
    cache (refs.lfo1Division, params::lfo1Division);
    cache (refs.lfo2Rate, params::lfo2Rate);
    cache (refs.lfo2Shape, params::lfo2Shape);
    cache (refs.lfo2Sync, params::lfo2Sync);
    cache (refs.lfo2Division, params::lfo2Division);

    cache (refs.chorusOn, params::chorusOn);
    cache (refs.chorusRate, params::chorusRate);
    cache (refs.chorusDepth, params::chorusDepth);
    cache (refs.chorusMix, params::chorusMix);
    cache (refs.chorusFeedback, params::chorusFeedback);

    cache (refs.delayOn, params::delayOn);
    cache (refs.delaySync, params::delaySync);
    cache (refs.delayDivision, params::delayDivision);
    cache (refs.delayTime, params::delayTime);
    cache (refs.delayFeedback, params::delayFeedback);
    cache (refs.delayMix, params::delayMix);
    cache (refs.delayPingPong, params::delayPingPong);
    cache (refs.delayTone, params::delayTone);

    cache (refs.reverbOn, params::reverbOn);
    cache (refs.reverbSize, params::reverbSize);
    cache (refs.reverbDamping, params::reverbDamping);
    cache (refs.reverbWidth, params::reverbWidth);
    cache (refs.reverbMix, params::reverbMix);

    cache (refs.masterDrive, params::masterDrive);
    cache (refs.eqLow, params::eqLow);
    cache (refs.eqMid, params::eqMid);
    cache (refs.eqHigh, params::eqHigh);
    cache (refs.masterGain, params::masterGain);

    cache (refs.macros[0], params::macro1);
    cache (refs.macros[1], params::macro2);
    cache (refs.macros[2], params::macro3);
    cache (refs.macros[3], params::macro4);

    for (int i = 0; i < numModSlots; ++i)
    {
        cache (refs.matrixSource[(size_t) i], params::matrixSourceIds[(size_t) i]);
        cache (refs.matrixDest[(size_t) i], params::matrixDestIds[(size_t) i]);
        cache (refs.matrixAmount[(size_t) i], params::matrixAmountIds[(size_t) i]);
    }
}

float SynthwaveLegacyProcessor::valueOf (const std::atomic<float>* parameter) noexcept
{
    return parameter != nullptr ? parameter->load (std::memory_order_relaxed) : 0.0f;
}

int SynthwaveLegacyProcessor::intValueOf (const std::atomic<float>* parameter) noexcept
{
    return juce::roundToInt (valueOf (parameter));
}

//==============================================================================
void SynthwaveLegacyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax (8000.0, sampleRate);
    maximumBlockSize = juce::jmax (1, samplesPerBlock);

    synthesiser.setCurrentPlaybackSampleRate (currentSampleRate);
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synthesiser.getVoice (i)))
            voice->prepare (currentSampleRate, maximumBlockSize, 2);

    prepareEffects (currentSampleRate, maximumBlockSize);

    midiScratch.ensureSize (16384);

    scopeDecimation = juce::jlimit (1, 64,
                                    juce::roundToInt (currentSampleRate / 12000.0));
    scopeCounter = 0;
    scopeFifo.reset();
    scopeData.fill (0.0f);

    lfo1.prepare (currentSampleRate, 0.0);
    lfo2.prepare (currentSampleRate, 0.25);
    lfoTransportWasPlaying = false;

    peakLeft.store (0.0f);
    peakRight.store (0.0f);
    gainReductionUi.store (0.0f);

    delayTimeInitialised = false;

    masterGainSmoothed.reset (currentSampleRate, 0.02);
    masterGainSmoothed.setCurrentAndTargetValue (
        dbToGain (valueOf (refs.masterGain)));

    delaySamplesSmoothedL.reset (currentSampleRate, 0.05);
    delaySamplesSmoothedR.reset (currentSampleRate, 0.05);

    bpmForUi.store (120.0f);
    transportRunning.store (false);
}

void SynthwaveLegacyProcessor::prepareEffects (double sampleRate, int preparedBlockSize)
{
    chorus.prepare (makeSpec (sampleRate, preparedBlockSize, 2));
    chorus.reset();
    chorus.setCentreDelay (11.0f);

    const auto mono = makeSpec (sampleRate, preparedBlockSize, 1);
    const auto maxDelaySamples = (int) std::ceil (sampleRate * 2.5) + 4;

    delayL.setMaximumDelayInSamples (maxDelaySamples);
    delayR.setMaximumDelayInSamples (maxDelaySamples);
    delayL.prepare (mono);
    delayR.prepare (mono);
    delayL.reset();
    delayR.reset();

    reverb.prepare (makeSpec (sampleRate, preparedBlockSize, 2));
    reverb.reset();

    eqLow.reset();
    eqMid.reset();
    eqHigh.reset();

    delayToneFilterL.prepare (sampleRate);
    delayToneFilterR.prepare (sampleRate);
}

void SynthwaveLegacyProcessor::releaseResources()
{
    synthesiser.allNotesOff (0, false);
    chorus.reset();
    delayL.reset();
    delayR.reset();
    reverb.reset();
    eqLow.reset();
    eqMid.reset();
    eqHigh.reset();
    delayToneFilterL.reset();
    delayToneFilterR.reset();
}

void SynthwaveLegacyProcessor::reset()
{
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synthesiser.getVoice (i)))
            voice->resetVoice();

    synthesiser.allNotesOff (0, false);
    keyboardState.reset();
    chorus.reset();
    delayL.reset();
    delayR.reset();
    reverb.reset();
    eqLow.reset();
    eqMid.reset();
    eqHigh.reset();
    delayToneFilterL.reset();
    delayToneFilterR.reset();

    delayTimeInitialised = false;
    peakLeft.store (0.0f);
    peakRight.store (0.0f);
    gainReductionUi.store (0.0f);
    scopeFifo.reset();
}

bool SynthwaveLegacyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
SynthwaveLegacyProcessor::TransportInfo SynthwaveLegacyProcessor::readTransportInfo() const
{
    TransportInfo info;

    if (auto* hostPlayHead = getPlayHead())
    {
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto hostBpm = position->getBpm())
                info.bpm = *hostBpm;

            info.playing = position->getIsPlaying();
        }
    }

    info.bpm = juce::jlimit (20.0, 400.0, info.bpm);
    return info;
}

void SynthwaveLegacyProcessor::updateGlobalLfos (const TransportInfo& transport, int numSamples)
{
    const auto division1 = juce::jlimit (0, 9, intValueOf (refs.lfo1Division));
    const auto division2 = juce::jlimit (0, 9, intValueOf (refs.lfo2Division));

    const auto freeRate1 = juce::jlimit (0.01f, 20.0f, valueOf (refs.lfo1Rate));
    const auto freeRate2 = juce::jlimit (0.01f, 20.0f, valueOf (refs.lfo2Rate));

    const auto syncedRate1 = transport.bpm / 60.0 / beatsPerCycleForDivision (division1);
    const auto syncedRate2 = transport.bpm / 60.0 / beatsPerCycleForDivision (division2);

    const auto rate1 = valueOf (refs.lfo1Sync) > 0.5f ? syncedRate1 : (double) freeRate1;
    const auto rate2 = valueOf (refs.lfo2Sync) > 0.5f ? syncedRate2 : (double) freeRate2;

    lfo1.setShape (intValueOf (refs.lfo1Shape));
    lfo2.setShape (intValueOf (refs.lfo2Shape));
    lfo1.setRateHz (rate1);
    lfo2.setRateHz (rate2);

    if (transport.playing && ! lfoTransportWasPlaying)
    {
        lfo1.reset (0.0);
        lfo2.reset (0.25);
    }
    lfoTransportWasPlaying = transport.playing;

    const auto samplesToFill = juce::jmin (numSamples, maxLfoBufferSize);
    lfo1.fillBuffer (lfoBuffer1.data(), samplesToFill);
    lfo2.fillBuffer (lfoBuffer2.data(), samplesToFill);

    bpmForUi.store ((float) transport.bpm);
    transportRunning.store (transport.playing);
}

SynthwaveLegacyProcessor::FxModulation
SynthwaveLegacyProcessor::computeFxModulation (float lfo1Value, float lfo2Value) const
{
    FxModulation result;

    std::array<float, 4> macros {
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[0])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[1])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[2])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[3]))
    };

    for (int i = 0; i < numModSlots; ++i)
    {
        const auto source = (ModSource) juce::jlimit (0, (int) ModSource::count - 1,
                                                      intValueOf (refs.matrixSource[(size_t) i]));
        const auto destination = (ModDestination) juce::jlimit (
            0, (int) ModDestination::count - 1,
            intValueOf (refs.matrixDest[(size_t) i]));
        const auto amount = juce::jlimit (-1.0f, 1.0f,
                                          valueOf (refs.matrixAmount[(size_t) i]));

        if (source == ModSource::none || destination == ModDestination::none
            || std::abs (amount) < 1.0e-6f)
            continue;

        const auto value = sourceValueForFx (source, lfo1Value, lfo2Value,
                                             modWheelUi.load(), aftertouchUi.load(), macros)
                         * amount;

        switch (destination)
        {
            case ModDestination::chorusMix:     result.chorusMix += value; break;
            case ModDestination::delayMix:      result.delayMix += value; break;
            case ModDestination::reverbMix:     result.reverbMix += value; break;
            case ModDestination::masterDrive:   result.masterDrive += value; break;
            case ModDestination::none:
            case ModDestination::osc1Pitch:
            case ModDestination::osc2Pitch:
            case ModDestination::bothPitch:
            case ModDestination::osc1Shape:
            case ModDestination::osc2Shape:
            case ModDestination::filterCutoff:
            case ModDestination::filterResonance:
            case ModDestination::ampLevel:
            case ModDestination::pan:
            case ModDestination::count:
            default: break;
        }
    }

    // The four macro knobs always remain useful, even with an empty matrix.
    // Macro 1 is applied per-voice to filter cutoff.
    result.chorusMix += macros[2] * 0.5f;
    result.reverbMix += macros[1] * 0.5f;
    result.masterDrive += macros[3] * 0.5f;

    return result;
}

//==============================================================================
void SynthwaveLegacyProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    buffer.clear();

    const auto transport = readTransportInfo();

    float modWheel = modWheelUi.load();
    float aftertouch = aftertouchUi.load();

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isController() && message.getControllerNumber() == 1)
            modWheel = (float) message.getControllerValue() / 127.0f;
        else if (message.isChannelPressure())
            aftertouch = (float) message.getChannelPressureValue() / 127.0f;
        else if (message.isAftertouch())
            aftertouch = juce::jmax (aftertouch, (float) message.getAfterTouchValue() / 127.0f);
    }

    modWheel = juce::jlimit (0.0f, 1.0f, modWheel);
    aftertouch = juce::jlimit (0.0f, 1.0f, aftertouch);
    modWheelUi.store (modWheel);
    aftertouchUi.store (aftertouch);

    midiScratch.clear();
    midiScratch.addEvents (midiMessages, 0, numSamples, 0);
    keyboardState.processNextMidiBuffer (midiScratch, 0, numSamples, true);

    updateGlobalLfos (transport, numSamples);
    const auto modulationSamples = juce::jmin (numSamples, maxLfoBufferSize);

    std::array<float, 4> macros {
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[0])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[1])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[2])),
        juce::jlimit (0.0f, 1.0f, valueOf (refs.macros[3]))
    };

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synthesiser.getVoice (i)))
            voice->setGlobalModulation (lfoBuffer1.data(), lfoBuffer2.data(),
                                       modulationSamples, modWheel, aftertouch,
                                       macros.data());

    synthesiser.renderNextBlock (buffer, midiScratch, 0, numSamples);

    const auto chorusEnabled = valueOf (refs.chorusOn) > 0.5f;
    const auto delayEnabled = valueOf (refs.delayOn) > 0.5f;
    const auto reverbEnabled = valueOf (refs.reverbOn) > 0.5f;
    const auto pingPong = valueOf (refs.delayPingPong) > 0.5f;

    const auto fxMod = computeFxModulation (lfoBuffer1[0], lfoBuffer2[0]);

    const auto chorusMix = chorusEnabled
        ? juce::jlimit (0.0f, 1.0f, valueOf (refs.chorusMix) + fxMod.chorusMix)
        : 0.0f;
    const auto chorusDepth = juce::jlimit (0.0f, 1.0f,
                                           valueOf (refs.chorusDepth) + macros[2] * 0.25f);
    const auto chorusRate = juce::jlimit (0.05f, 8.0f, valueOf (refs.chorusRate));
    const auto chorusFeedback = juce::jlimit (-0.95f, 0.95f, valueOf (refs.chorusFeedback));

    const auto delayMix = delayEnabled
        ? juce::jlimit (0.0f, 1.0f, valueOf (refs.delayMix) + fxMod.delayMix)
        : 0.0f;
    const auto delayFeedback = delayEnabled
        ? juce::jlimit (0.0f, 0.95f, valueOf (refs.delayFeedback))
        : 0.0f;
    const auto delayTone = juce::jlimit (200.0f, 18000.0f, valueOf (refs.delayTone));

    const auto reverbMix = reverbEnabled
        ? juce::jlimit (0.0f, 1.0f, valueOf (refs.reverbMix) + fxMod.reverbMix)
        : 0.0f;
    const auto reverbSize = juce::jlimit (0.0f, 1.0f, valueOf (refs.reverbSize));
    const auto reverbDamping = juce::jlimit (0.0f, 1.0f, valueOf (refs.reverbDamping));
    const auto reverbWidth = juce::jlimit (0.0f, 1.0f, valueOf (refs.reverbWidth));

    const auto masterDrive = juce::jlimit (0.0f, 1.0f,
                                           valueOf (refs.masterDrive) + fxMod.masterDrive);
    const auto masterLow = juce::jlimit (-12.0f, 12.0f, valueOf (refs.eqLow));
    const auto masterMid = juce::jlimit (-12.0f, 12.0f, valueOf (refs.eqMid));
    const auto masterHigh = juce::jlimit (-12.0f, 12.0f, valueOf (refs.eqHigh));

    // Delay time is computed here and smoothed sample-by-sample inside processDelay.
    double delayMs = (double) valueOf (refs.delayTime);
    if (valueOf (refs.delaySync) > 0.5f)
    {
        const auto division = juce::jlimit (0, 9, intValueOf (refs.delayDivision));
        delayMs = syncedDelayMs (division, transport.bpm);
    }

    delayMs = juce::jlimit (10.0, 2000.0, delayMs);
    const auto delaySamples = (float) (delayMs * 0.001 * currentSampleRate);
    if (! delayTimeInitialised)
    {
        delaySamplesSmoothedL.setCurrentAndTargetValue (delaySamples);
        delaySamplesSmoothedR.setCurrentAndTargetValue (delaySamples);
        delayTimeInitialised = true;
    }
    else
    {
        delaySamplesSmoothedL.setTargetValue (delaySamples);
        delaySamplesSmoothedR.setTargetValue (delaySamples);
    }

    masterGainSmoothed.setTargetValue (dbToGain (valueOf (refs.masterGain)));

    processChorus (buffer, numSamples, chorusEnabled, chorusRate, chorusDepth,
                   chorusMix, chorusFeedback);
    processDelay (buffer, numSamples, delayEnabled, delayMix, delayFeedback,
                  pingPong, delayTone);
    processReverb (buffer, numSamples, reverbEnabled, reverbMix,
                   reverbSize, reverbDamping, reverbWidth);
    processMaster (buffer, numSamples, masterDrive, masterLow, masterMid, masterHigh);

    updateMetersAndScope (buffer, numSamples);
}

//==============================================================================
void SynthwaveLegacyProcessor::processChorus (juce::AudioBuffer<float>& buffer,
                                              int numSamples, bool enabled,
                                              float rate, float depth, float mix,
                                              float feedback)
{
    chorus.setRate (rate);
    chorus.setDepth (depth);
    chorus.setFeedback (feedback);
    chorus.setMix (enabled ? mix : 0.0f);
    chorus.setCentreDelay (11.0f);

    const auto chunkSize = juce::jmax (1, maximumBlockSize);
    const auto channels = (size_t) juce::jmin (2, buffer.getNumChannels());

    for (int offset = 0; offset < numSamples; offset += chunkSize)
    {
        const auto samplesThisTime = juce::jmin (chunkSize, numSamples - offset);
        auto block = juce::dsp::AudioBlock<float> (buffer)
                         .getSubsetChannelBlock (0, channels)
                         .getSubBlock ((size_t) offset, (size_t) samplesThisTime);
        juce::dsp::ProcessContextReplacing<float> context (block);
        chorus.process (context);
    }
}

void SynthwaveLegacyProcessor::processDelay (juce::AudioBuffer<float>& buffer,
                                             int numSamples, bool enabled, float mix,
                                             float feedback, bool pingPong, float toneHz)
{
    juce::ignoreUnused (enabled);

    if (buffer.getNumChannels() < 2)
        return;

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    delayToneFilterL.setCutoff (toneHz);
    delayToneFilterR.setCutoff (toneHz);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto delayTimeL = juce::jlimit (1.0f, (float) (currentSampleRate * 2.4),
                                              delaySamplesSmoothedL.getNextValue());
        const auto delayTimeR = juce::jlimit (1.0f, (float) (currentSampleRate * 2.4),
                                              delaySamplesSmoothedR.getNextValue());

        const auto wetL = delayL.popSample (0, delayTimeL, true);
        const auto wetR = delayR.popSample (0, delayTimeR, true);

        const auto inputL = left[i];
        const auto inputR = right[i];

        const auto tonedL = delayToneFilterL.process (wetL);
        const auto tonedR = delayToneFilterR.process (wetR);

        const auto fbL = pingPong ? tonedR : tonedL;
        const auto fbR = pingPong ? tonedL : tonedR;

        delayL.pushSample (0, inputL + fbL * feedback);
        delayR.pushSample (0, inputR + fbR * feedback);

        left[i] = inputL * (1.0f - mix) + wetL * mix;
        right[i] = inputR * (1.0f - mix) + wetR * mix;
    }
}

void SynthwaveLegacyProcessor::processReverb (juce::AudioBuffer<float>& buffer,
                                              int numSamples, bool enabled, float mix,
                                              float size, float damping, float width)
{
    reverb.setEnabled (enabled);

    if (enabled)
    {
        juce::dsp::Reverb::Parameters params;
        params.roomSize = size;
        params.damping = damping;
        params.width = width;
        params.wetLevel = juce::jlimit (0.0f, 1.0f, mix) * 0.5f;
        params.dryLevel = juce::jlimit (0.0f, 1.0f, 1.0f - mix) * 0.5f;
        params.freezeMode = 0.0f;
        reverb.setParameters (params);
    }

    auto block = juce::dsp::AudioBlock<float> (buffer)
                     .getSubsetChannelBlock (0, (size_t) juce::jmin (2, buffer.getNumChannels()))
                     .getSubBlock (0, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);
}

void SynthwaveLegacyProcessor::processMaster (juce::AudioBuffer<float>& buffer,
                                              int numSamples, float drive,
                                              float lowDb, float midDb, float highDb)
{
    eqLow.setLowShelf (currentSampleRate, 120.0f, lowDb);
    eqMid.setPeak (currentSampleRate, 1000.0f, midDb, 0.85f);
    eqHigh.setHighShelf (currentSampleRate, 7600.0f, highDb);

    const auto wet = juce::jlimit (0.0f, 1.0f, drive);
    const auto preGain = 1.0f + wet * 12.0f;
    const auto outputComp = 1.0f / (1.0f + wet * 1.5f);

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        auto lor = left[i];
        auto ror = right != nullptr ? right[i] : 0.0f;

        if (wet > 0.0f)
        {
            lor = lor * (1.0f - wet) + std::tanh (lor * preGain) * outputComp * wet;
            ror = ror * (1.0f - wet) + std::tanh (ror * preGain) * outputComp * wet;
        }

        lor = eqHigh.processLeft (eqMid.processLeft (eqLow.processLeft (lor)));
        ror = eqHigh.processRight (eqMid.processRight (eqLow.processRight (ror)));

        const auto masterGain = masterGainSmoothed.getNextValue();
        const auto limit = [] (float value) noexcept
        {
            constexpr auto threshold = 0.94f;
            const auto magnitude = std::abs (value);
            if (magnitude <= threshold)
                return value;

            const auto compressed = threshold + (1.0f - threshold)
                                  * std::tanh ((magnitude - threshold) / (1.0f - threshold));
            return std::copysign (compressed, value);
        };

        left[i] = limit (lor * masterGain);

        if (right != nullptr)
            right[i] = limit (ror * masterGain);
    }
}

void SynthwaveLegacyProcessor::updateMetersAndScope (const juce::AudioBuffer<float>& buffer,
                                                     int numSamples)
{
    const auto numChannels = buffer.getNumChannels();
    if (numChannels <= 0)
        return;

    const auto* left = buffer.getReadPointer (0);
    const auto* right = numChannels > 1 ? buffer.getReadPointer (1) : left;

    float peakL = 0.0f;
    float peakR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        peakL = juce::jmax (peakL, std::abs (left[i]));
        peakR = juce::jmax (peakR, std::abs (right[i]));

        if (++scopeCounter >= scopeDecimation)
        {
            scopeCounter = 0;
            pushScopeSample ((left[i] + right[i]) * 0.5f);
        }
    }

    peakLeft.store (peakL);
    peakRight.store (peakR);
    gainReductionUi.store (0.0f);
}

void SynthwaveLegacyProcessor::pushScopeSample (float sample)
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    scopeFifo.prepareToWrite (1, start1, size1, start2, size2);

    if (size1 > 0)
        scopeData[(size_t) start1] = sample;
    else if (size2 > 0)
        scopeData[(size_t) start2] = sample;

    scopeFifo.finishedWrite (size1 + size2);
}

int SynthwaveLegacyProcessor::readScopeSamples (float* destination, int maxSamples)
{
    if (destination == nullptr || maxSamples <= 0)
        return 0;

    auto ready = scopeFifo.getNumReady();
    if (ready > maxSamples)
    {
        const auto toDiscard = ready - maxSamples;
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        scopeFifo.prepareToRead (toDiscard, start1, size1, start2, size2);
        scopeFifo.finishedRead (size1 + size2);
        ready = scopeFifo.getNumReady();
    }

    const auto toRead = juce::jmin (ready, maxSamples);
    if (toRead <= 0)
        return 0;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    scopeFifo.prepareToRead (toRead, start1, size1, start2, size2);

    int written = 0;
    for (int i = 0; i < size1; ++i)
        destination[written++] = scopeData[(size_t) (start1 + i)];
    for (int i = 0; i < size2; ++i)
        destination[written++] = scopeData[(size_t) (start2 + i)];

    scopeFifo.finishedRead (written);
    return written;
}

float SynthwaveLegacyProcessor::getPeakLevel (int channel) const noexcept
{
    return channel == 0 ? peakLeft.load() : peakRight.load();
}

//==============================================================================
int SynthwaveLegacyProcessor::getNumPrograms()
{
    return kNumFactoryPresets;
}

int SynthwaveLegacyProcessor::getCurrentProgram()
{
    const auto value = intValueOf (apvts.getRawParameterValue (params::preset));
    return juce::jlimit (0, kNumFactoryPresets - 1, value);
}

void SynthwaveLegacyProcessor::setCurrentProgram (int index)
{
    loadFactoryPreset (index);
}

const juce::String SynthwaveLegacyProcessor::getProgramName (int index)
{
    const auto names = presetChoices();
    return names[juce::jlimit (0, names.size() - 1, index)];
}

void SynthwaveLegacyProcessor::changeProgramName (int, const juce::String&)
{
    // Factory presets are immutable.
}

void SynthwaveLegacyProcessor::loadFactoryPreset (int index)
{
    if (juce::isPositiveAndBelow (index, kNumFactoryPresets))
        applyFactoryPreset (apvts, index);
}

void SynthwaveLegacyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void SynthwaveLegacyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (tree.isValid())
        apvts.replaceState (tree);
}

//==============================================================================
juce::AudioProcessorEditor* SynthwaveLegacyProcessor::createEditor()
{
    return new SynthwaveLegacyEditor (*this);
}

} // namespace synthwave

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new synthwave::SynthwaveLegacyProcessor();
}
