#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <cstdint>

namespace synthwave
{
//==============================================================================
inline float dbToGain (float decibels) noexcept
{
    return std::pow (10.0f, decibels / 20.0f);
}

inline float softClip (float x) noexcept
{
    // A fast-ish rational tanh approximation with unity slope at zero.
    const auto ax = std::abs (x);
    if (ax < 1.0e-5f)
        return x;

    const auto x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float wrapPhase (double phase) noexcept
{
    phase -= std::floor (phase);
    return (float) phase;
}

/** Constants in quarter-note beats for the 10 sync divisions exposed by the UI. */
inline constexpr std::array<double, 10> syncDivisionBeats {
    0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.5,
    2.0 / 3.0, 1.0, 2.0, 4.0, 8.0
};

//==============================================================================
/** Band-limited virtual-analog oscillator using PolyBLEP correction. */
class PolyBlepOscillator
{
public:
    void prepare (double newSampleRate) noexcept;
    void reset (double startPhase = 0.0) noexcept;

    void setWaveform (int waveformIndex) noexcept;
    void setPulseWidth (float newPulseWidth) noexcept;
    void setFrequency (double frequencyHz) noexcept;
    void setPhaseIncrement (double increment) noexcept;

    float process (bool& wrapped) noexcept;

    double getFrequency() const noexcept       { return frequency; }
    double getPhase() const noexcept           { return phase; }

private:
    float processSaw() noexcept;
    float processSquare() noexcept;
    float processTriangle() noexcept;
    static double polyBlep (double t, double dt) noexcept;

    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double frequency = 440.0;
    double triangleIntegrator = -1.0;
    float pulseWidth = 0.5f;
    int waveform = 0;
};

//==============================================================================
/** ADSR envelope with click-free linear segments and a hard zero floor. */
class AdsrEnvelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;
    void setParameters (float attackSeconds, float decaySeconds,
                        float sustainLevel, float releaseSeconds) noexcept;

    float getNextSample() noexcept;
    float getLevel() const noexcept            { return level; }
    Stage getStage() const noexcept            { return stage; }
    bool isActive() const noexcept             { return stage != Stage::idle; }
    bool isReleasing() const noexcept          { return stage == Stage::release; }

private:
    void updateDeltas() noexcept;

    double sampleRate = 44100.0;
    float attack = 0.01f;
    float decay = 0.2f;
    float sustain = 0.7f;
    float release = 0.4f;
    float level = 0.0f;
    float attackDelta = 0.0f;
    float decayDelta = 0.0f;
    float releaseDelta = 0.0f;
    Stage stage = Stage::idle;
};

//==============================================================================
/** Host-synchronisable global LFO. */
class GlobalLfo
{
public:
    void prepare (double newSampleRate, double initialPhase = 0.0) noexcept;
    void reset (double phaseToUse = 0.0) noexcept;
    void setShape (int lfoShape) noexcept        { shape = juce::jlimit (0, 4, lfoShape); }
    void setRateHz (double newRateHz) noexcept   { rateHz = juce::jlimit (0.0, 40.0, newRateHz); }
    void fillBuffer (float* destination, int numSamples) noexcept;
    float processSample() noexcept;

private:
    float makeRandomBipolar() noexcept;

    double sampleRate = 44100.0;
    double phase = 0.0;
    double rateHz = 1.0;
    int shape = 0;
    float heldValue = 0.0f;
    uint32_t randomState = 0x9e3779b9u;
};

//==============================================================================
/** Simple TPT-ladder wrapper exposing JUCE's protected per-sample path. */
class VoiceLadderFilter : public juce::dsp::LadderFilter<float>
{
public:
    void prepareVoice (const juce::dsp::ProcessSpec& spec)
    {
        prepare (spec);
        reset();
    }

    float processVoiceSample (float input) noexcept
    {
        updateSmoothers();
        return processSample (input, 0);
    }
};

//==============================================================================
/** RBJ biquad with per-channel state. Coefficients are calculated on the
    message/audio thread without allocating. */
class StereoBiquad
{
public:
    struct Coefficients
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    void reset() noexcept;
    void setLowShelf (double sampleRate, float frequencyHz, float gainDb) noexcept;
    void setPeak (double sampleRate, float frequencyHz, float gainDb, float q) noexcept;
    void setHighShelf (double sampleRate, float frequencyHz, float gainDb) noexcept;

    float processLeft (float input) noexcept   { return process (input, z1L, z2L); }
    float processRight (float input) noexcept  { return process (input, z1R, z2R); }

private:
    void setCoefficients (float nb0, float nb1, float nb2, float na0, float na1, float na2) noexcept;
    float process (float input, float& z1, float& z2) noexcept;

    Coefficients coefficients;
    float z1L = 0.0f, z2L = 0.0f, z1R = 0.0f, z2R = 0.0f;
};

//==============================================================================
class OnePoleLowPass
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept                 { state = 0.0f; }
    void setCutoff (float frequencyHz) noexcept;
    float process (float input) noexcept  { state += coefficient * (input - state); return state; }

private:
    double sampleRate = 44100.0;
    float coefficient = 1.0f;
    float state = 0.0f;
};

} // namespace synthwave
