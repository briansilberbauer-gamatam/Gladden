#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <cstdint>

#include "DSP.h"
#include "Parameters.h"

namespace synthwave
{
//==============================================================================
class SynthSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override       { return true; }
    bool appliesToChannel (int) override    { return true; }
};

//==============================================================================
class SynthVoice final : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice (juce::AudioProcessorValueTreeState& state);
    ~SynthVoice() override = default;

    using juce::SynthesiserVoice::renderNextBlock;

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float noteVelocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float noteVelocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepare (double newSampleRate, int maximumBlockSize, int numChannels);
    void resetVoice();

    /** Called once per audio block before the Synthesiser iterates its voices. */
    void setGlobalModulation (const float* lfo1Samples,
                              const float* lfo2Samples,
                              int numGlobalSamples,
                              float modWheelValue,
                              float aftertouchValue,
                              const float* macroValues);

    int getCurrentlyPlayingNote() const noexcept    { return currentNote; }

private:
    struct Refs
    {
        std::atomic<float>* osc1Wave = nullptr;
        std::atomic<float>* osc1Octave = nullptr;
        std::atomic<float>* osc1Semi = nullptr;
        std::atomic<float>* osc1Fine = nullptr;
        std::atomic<float>* osc1Level = nullptr;
        std::atomic<float>* osc1PW = nullptr;
        std::atomic<float>* osc1Sub = nullptr;

        std::atomic<float>* osc2Wave = nullptr;
        std::atomic<float>* osc2Octave = nullptr;
        std::atomic<float>* osc2Semi = nullptr;
        std::atomic<float>* osc2Fine = nullptr;
        std::atomic<float>* osc2Level = nullptr;
        std::atomic<float>* osc2PW = nullptr;
        std::atomic<float>* osc2Sub = nullptr;

        std::atomic<float>* oscSync = nullptr;
        std::atomic<float>* ringMod = nullptr;
        std::atomic<float>* fmAmount = nullptr;
        std::atomic<float>* fmRatio = nullptr;
        std::atomic<float>* noise = nullptr;

        std::atomic<float>* filterMode = nullptr;
        std::atomic<float>* filterCutoff = nullptr;
        std::atomic<float>* filterResonance = nullptr;
        std::atomic<float>* filterDrive = nullptr;
        std::atomic<float>* filterEnvAmount = nullptr;
        std::atomic<float>* filterKeyTrack = nullptr;
        std::atomic<float>* filterLfo1 = nullptr;
        std::atomic<float>* filterLfo2 = nullptr;

        std::atomic<float>* ampAttack = nullptr;
        std::atomic<float>* ampDecay = nullptr;
        std::atomic<float>* ampSustain = nullptr;
        std::atomic<float>* ampRelease = nullptr;

        std::atomic<float>* filtAttack = nullptr;
        std::atomic<float>* filtDecay = nullptr;
        std::atomic<float>* filtSustain = nullptr;
        std::atomic<float>* filtRelease = nullptr;

        std::atomic<float>* modAttack = nullptr;
        std::atomic<float>* modDecay = nullptr;
        std::atomic<float>* modSustain = nullptr;
        std::atomic<float>* modRelease = nullptr;

        std::array<std::atomic<float>*, numModSlots> matrixSource {};
        std::array<std::atomic<float>*, numModSlots> matrixDest {};
        std::array<std::atomic<float>*, numModSlots> matrixAmount {};
    };

    void cacheParameterPointers();
    static float valueOf (const std::atomic<float>* parameter) noexcept;
    static int intValueOf (const std::atomic<float>* parameter) noexcept;

    void configureFilter (FilterMode mode, float drive) noexcept;
    float processFilterSample (float input) noexcept;
    void applyFilterCoefficients (float cutoffHz, float resonance) noexcept;
    void updateSubOscillators (double increment1, double increment2) noexcept;
    float nextNoise() noexcept;

    struct MatrixRow
    {
        ModSource source = ModSource::none;
        ModDestination destination = ModDestination::none;
        float amount = 0.0f;
    };

    juce::AudioProcessorValueTreeState& parameters;
    Refs refs;

    PolyBlepOscillator osc1;
    PolyBlepOscillator osc2;
    AdsrEnvelope ampEnvelope;
    AdsrEnvelope filterEnvelope;
    AdsrEnvelope modEnvelope;

    juce::dsp::StateVariableTPTFilter<float> svfA;
    juce::dsp::StateVariableTPTFilter<float> svfB;
    VoiceLadderFilter ladderA;
    VoiceLadderFilter ladderB;

    std::array<MatrixRow, numModSlots> matrixRows {};
    std::array<float, 4> globalMacros {};

    const float* globalLfo1 = nullptr;
    const float* globalLfo2 = nullptr;
    int globalLfoSampleCount = 0;
    float globalModWheel = 0.0f;
    float globalAftertouch = 0.0f;

    double sampleRate = 44100.0;
    int currentNote = 60;
    float noteVelocity = 0.0f;
    float velocityGain = 0.0f;
    float pitchBend = 0.0f;

    double subPhase1 = 0.0;
    double subPhase2 = 0.0;
    uint32_t noiseState = 0x1234567u;

    float smoothedCutoffHz = 2800.0f;
    bool cutoffInitialised = false;
    int filterUpdateCountdown = 0;
    float filterResonanceSmoothed = 0.0f;
    FilterMode configuredFilterMode = FilterMode::lowPass24;
    bool filterConfigurationValid = false;
    float configuredDrive = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthVoice)
};

} // namespace synthwave
