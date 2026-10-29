#include "DSP.h"

#include <algorithm>

namespace synthwave
{
//==============================================================================
void PolyBlepOscillator::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (8000.0, newSampleRate);
    setFrequency (frequency);
    reset (phase);
}

void PolyBlepOscillator::reset (double startPhase) noexcept
{
    phase = startPhase - std::floor (startPhase);
    triangleIntegrator = -1.0;
}

void PolyBlepOscillator::setWaveform (int waveformIndex) noexcept
{
    const auto newWaveform = juce::jlimit (0, 3, waveformIndex);
    if (newWaveform != waveform)
    {
        waveform = newWaveform;
        triangleIntegrator = -1.0;
    }
}

void PolyBlepOscillator::setPulseWidth (float newPulseWidth) noexcept
{
    pulseWidth = juce::jlimit (0.05f, 0.95f, newPulseWidth);
}

void PolyBlepOscillator::setFrequency (double frequencyHz) noexcept
{
    frequency = juce::jlimit (0.01, sampleRate * 0.48, frequencyHz);
    phaseIncrement = frequency / sampleRate;
}

void PolyBlepOscillator::setPhaseIncrement (double increment) noexcept
{
    phaseIncrement = juce::jlimit (0.0, 0.48, increment);
    frequency = phaseIncrement * sampleRate;
}

double PolyBlepOscillator::polyBlep (double t, double dt) noexcept
{
    if (dt <= 0.0)
        return 0.0;

    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0;
    }

    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }

    return 0.0;
}

float PolyBlepOscillator::processSaw() noexcept
{
    return (float) (2.0 * phase - 1.0 - polyBlep (phase, phaseIncrement));
}

float PolyBlepOscillator::processSquare() noexcept
{
    const auto width = (double) pulseWidth;
    auto value = phase < width ? 1.0 : -1.0;
    value += polyBlep (phase, phaseIncrement);

    auto fallingPhase = phase - width;
    if (fallingPhase < 0.0)
        fallingPhase += 1.0;

    value -= polyBlep (fallingPhase, phaseIncrement);
    return (float) value;
}

float PolyBlepOscillator::processTriangle() noexcept
{
    // Integrate a band-limited 50% pulse. This yields a triangle without the
    // slope-discontinuity aliasing of the naive triangle formula.
    auto square = phase < 0.5 ? 1.0 : -1.0;
    square += polyBlep (phase, phaseIncrement);

    auto fallingPhase = phase - 0.5;
    if (fallingPhase < 0.0)
        fallingPhase += 1.0;
    square -= polyBlep (fallingPhase, phaseIncrement);

    triangleIntegrator += 4.0 * phaseIncrement * square;
    triangleIntegrator = juce::jlimit (-1.15, 1.15, triangleIntegrator);
    return (float) juce::jlimit (-1.0, 1.0, triangleIntegrator);
}

float PolyBlepOscillator::process (bool& wrapped) noexcept
{
    phase += phaseIncrement;
    wrapped = phase >= 1.0;
    if (wrapped)
        phase -= std::floor (phase);

    if (wrapped && waveform == 2)
        triangleIntegrator = -1.0;

    switch (waveform)
    {
        case 1:  return processSquare();
        case 2:  return processTriangle();
        case 3:  return (float) std::sin (juce::MathConstants<double>::twoPi * phase);
        default: return processSaw();
    }
}

//==============================================================================
void AdsrEnvelope::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1000.0, newSampleRate);
    updateDeltas();
}

void AdsrEnvelope::reset() noexcept
{
    stage = Stage::idle;
    level = 0.0f;
}

void AdsrEnvelope::noteOn() noexcept
{
    level = 0.0f;
    stage = Stage::attack;
    updateDeltas();
}

void AdsrEnvelope::noteOff() noexcept
{
    if (stage == Stage::idle || stage == Stage::release)
        return;

    if (level <= 1.0e-5f)
    {
        reset();
        return;
    }

    releaseDelta = level / juce::jmax (1.0f, release * (float) sampleRate);
    stage = Stage::release;
}

void AdsrEnvelope::setParameters (float attackSeconds, float decaySeconds,
                                  float sustainLevel, float releaseSeconds) noexcept
{
    attack = juce::jlimit (0.001f, 20.0f, attackSeconds);
    decay = juce::jlimit (0.001f, 20.0f, decaySeconds);
    sustain = juce::jlimit (0.0f, 1.0f, sustainLevel);
    release = juce::jlimit (0.003f, 30.0f, releaseSeconds);
    updateDeltas();
}

void AdsrEnvelope::updateDeltas() noexcept
{
    attackDelta = 1.0f / juce::jmax (1.0f, attack * (float) sampleRate);
    decayDelta = (1.0f - sustain) / juce::jmax (1.0f, decay * (float) sampleRate);
}

float AdsrEnvelope::getNextSample() noexcept
{
    switch (stage)
    {
        case Stage::attack:
            level += attackDelta;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::decay;
            }
            break;

        case Stage::decay:
            level -= decayDelta;
            if (level <= sustain)
            {
                level = sustain;
                stage = Stage::sustain;
            }
            break;

        case Stage::sustain:
            level = sustain;
            break;

        case Stage::release:
            level -= releaseDelta;
            if (level <= 0.0f)
            {
                level = 0.0f;
                stage = Stage::idle;
            }
            break;

        case Stage::idle:
        default:
            level = 0.0f;
            break;
    }

    return level;
}

//==============================================================================
void GlobalLfo::prepare (double newSampleRate, double initialPhase) noexcept
{
    sampleRate = juce::jmax (1000.0, newSampleRate);
    reset (initialPhase);
}

void GlobalLfo::reset (double phaseToUse) noexcept
{
    phase = phaseToUse - std::floor (phaseToUse);
    heldValue = makeRandomBipolar();
}

float GlobalLfo::makeRandomBipolar() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return (float) ((double) (randomState & 0x00ffffffu) / 8388608.0 - 1.0);
}

float GlobalLfo::processSample() noexcept
{
    const auto samplesPerCycle = rateHz > 0.0 ? sampleRate / rateHz : 0.0;
    const auto increment = samplesPerCycle > 0.0 ? 1.0 / samplesPerCycle : 0.0;

    phase += increment;
    if (phase >= 1.0)
    {
        phase -= std::floor (phase);
        heldValue = makeRandomBipolar();
    }

    switch (shape)
    {
        case 1:  return (float) (1.0 - 4.0 * std::abs (phase - 0.5));
        case 2:  return (float) (2.0 * phase - 1.0);
        case 3:  return phase < 0.5 ? 1.0f : -1.0f;
        case 4:  return heldValue;
        case 0:
        default: return (float) std::sin (juce::MathConstants<double>::twoPi * phase);
    }
}

void GlobalLfo::fillBuffer (float* destination, int numSamples) noexcept
{
    if (destination == nullptr)
        return;

    for (int i = 0; i < numSamples; ++i)
        destination[i] = processSample();
}

//==============================================================================
void StereoBiquad::reset() noexcept
{
    z1L = z2L = z1R = z2R = 0.0f;
}

void StereoBiquad::setCoefficients (float nb0, float nb1, float nb2,
                                    float na0, float na1, float na2) noexcept
{
    if (std::abs (na0) < 1.0e-12f)
    {
        coefficients = {};
        return;
    }

    const auto invA0 = 1.0f / na0;
    coefficients.b0 = nb0 * invA0;
    coefficients.b1 = nb1 * invA0;
    coefficients.b2 = nb2 * invA0;
    coefficients.a1 = na1 * invA0;
    coefficients.a2 = na2 * invA0;
}

void StereoBiquad::setLowShelf (double sampleRate, float frequencyHz, float gainDb) noexcept
{
    const auto frequency = juce::jlimit (20.0f, (float) sampleRate * 0.45f, frequencyHz);
    const auto w0 = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const auto cosW = std::cos (w0);
    const auto sinW = std::sin (w0);
    const auto A = std::pow (10.0, (double) gainDb / 40.0);
    const auto alpha = sinW * 0.5 * std::sqrt ((A + 1.0 / A) * (1.0 - 1.0) + 2.0);
    const auto twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const auto b0 = A * ((A + 1.0) - (A - 1.0) * cosW + twoSqrtAAlpha);
    const auto b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW);
    const auto b2 = A * ((A + 1.0) - (A - 1.0) * cosW - twoSqrtAAlpha);
    const auto a0 = (A + 1.0) + (A - 1.0) * cosW + twoSqrtAAlpha;
    const auto a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW);
    const auto a2 = (A + 1.0) + (A - 1.0) * cosW - twoSqrtAAlpha;

    setCoefficients ((float) b0, (float) b1, (float) b2,
                     (float) a0, (float) a1, (float) a2);
}

void StereoBiquad::setPeak (double sampleRate, float frequencyHz, float gainDb, float q) noexcept
{
    const auto frequency = juce::jlimit (20.0f, (float) sampleRate * 0.45f, frequencyHz);
    const auto w0 = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const auto cosW = std::cos (w0);
    const auto sinW = std::sin (w0);
    const auto A = std::pow (10.0, (double) gainDb / 40.0);
    const auto alpha = sinW / (2.0 * juce::jmax (0.1f, q));

    const auto b0 = 1.0 + alpha * A;
    const auto b1 = -2.0 * cosW;
    const auto b2 = 1.0 - alpha * A;
    const auto a0 = 1.0 + alpha / A;
    const auto a1 = -2.0 * cosW;
    const auto a2 = 1.0 - alpha / A;

    setCoefficients ((float) b0, (float) b1, (float) b2,
                     (float) a0, (float) a1, (float) a2);
}

void StereoBiquad::setHighShelf (double sampleRate, float frequencyHz, float gainDb) noexcept
{
    const auto frequency = juce::jlimit (20.0f, (float) sampleRate * 0.45f, frequencyHz);
    const auto w0 = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const auto cosW = std::cos (w0);
    const auto sinW = std::sin (w0);
    const auto A = std::pow (10.0, (double) gainDb / 40.0);
    const auto alpha = sinW * 0.5 * std::sqrt ((A + 1.0 / A) * (1.0 - 1.0) + 2.0);
    const auto twoSqrtAAlpha = 2.0 * std::sqrt (A) * alpha;

    const auto b0 = A * ((A + 1.0) + (A - 1.0) * cosW + twoSqrtAAlpha);
    const auto b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW);
    const auto b2 = A * ((A + 1.0) + (A - 1.0) * cosW - twoSqrtAAlpha);
    const auto a0 = (A + 1.0) - (A - 1.0) * cosW + twoSqrtAAlpha;
    const auto a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW);
    const auto a2 = (A + 1.0) - (A - 1.0) * cosW - twoSqrtAAlpha;

    setCoefficients ((float) b0, (float) b1, (float) b2,
                     (float) a0, (float) a1, (float) a2);
}

float StereoBiquad::process (float input, float& z1, float& z2) noexcept
{
    const auto output = coefficients.b0 * input + z1;
    z1 = coefficients.b1 * input - coefficients.a1 * output + z2;
    z2 = coefficients.b2 * input - coefficients.a2 * output;
    return output;
}

//==============================================================================
void OnePoleLowPass::prepare (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1000.0, newSampleRate);
    setCutoff (5500.0f);
}

void OnePoleLowPass::setCutoff (float frequencyHz) noexcept
{
    coefficient = juce::jlimit (0.0001f, 0.999f, 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * juce::jmax (20.0f, frequencyHz)
        / (float) sampleRate));
}

} // namespace synthwave
