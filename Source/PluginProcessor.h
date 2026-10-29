#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

#include "DSP.h"
#include "Parameters.h"
#include "SynthVoice.h"

namespace synthwave
{
class SynthwaveLegacyEditor;

//==============================================================================
class SynthwaveLegacyProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int scopeCapacity = 4096;
    static constexpr int maxLfoBufferSize = 16384;

    SynthwaveLegacyProcessor();
    ~SynthwaveLegacyProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                              { return true; }

    const juce::String getName() const override                  { return "Synthwave Legacy"; }
    bool acceptsMidi() const override                            { return true; }
    bool producesMidi() const override                           { return false; }
    bool isMidiEffect() const override                           { return false; }
    double getTailLengthSeconds() const override                 { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept       { return apvts; }
    juce::MidiKeyboardState& getKeyboardState() noexcept                   { return keyboardState; }

    float getPeakLevel (int channel) const noexcept;
    int readScopeSamples (float* destination, int maxSamples);
    float getDisplayedBpm() const noexcept                                 { return bpmForUi.load(); }
    bool isTransportRunning() const noexcept                               { return transportRunning.load(); }
    float getDisplayedModWheel() const noexcept                            { return modWheelUi.load(); }
    float getDisplayedAftertouch() const noexcept                          { return aftertouchUi.load(); }
    float getDisplayedPeakReduction() const noexcept                       { return gainReductionUi.load(); }

    void loadFactoryPreset (int index);

private:
    //==========================================================================
    struct FxRefs
    {
        std::atomic<float>* lfo1Rate = nullptr;
        std::atomic<float>* lfo1Shape = nullptr;
        std::atomic<float>* lfo1Sync = nullptr;
        std::atomic<float>* lfo1Division = nullptr;
        std::atomic<float>* lfo2Rate = nullptr;
        std::atomic<float>* lfo2Shape = nullptr;
        std::atomic<float>* lfo2Sync = nullptr;
        std::atomic<float>* lfo2Division = nullptr;

        std::atomic<float>* chorusOn = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* chorusFeedback = nullptr;

        std::atomic<float>* delayOn = nullptr;
        std::atomic<float>* delaySync = nullptr;
        std::atomic<float>* delayDivision = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* delayPingPong = nullptr;
        std::atomic<float>* delayTone = nullptr;

        std::atomic<float>* reverbOn = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamping = nullptr;
        std::atomic<float>* reverbWidth = nullptr;
        std::atomic<float>* reverbMix = nullptr;

        std::atomic<float>* masterDrive = nullptr;
        std::atomic<float>* eqLow = nullptr;
        std::atomic<float>* eqMid = nullptr;
        std::atomic<float>* eqHigh = nullptr;
        std::atomic<float>* masterGain = nullptr;

        std::array<std::atomic<float>*, 4> macros {};
        std::array<std::atomic<float>*, numModSlots> matrixSource {};
        std::array<std::atomic<float>*, numModSlots> matrixDest {};
        std::array<std::atomic<float>*, numModSlots> matrixAmount {};
    };

    struct FxModulation
    {
        float chorusMix = 0.0f;
        float delayMix = 0.0f;
        float reverbMix = 0.0f;
        float masterDrive = 0.0f;
    };

    struct TransportInfo
    {
        double bpm = 120.0;
        bool playing = false;
    };

    void cacheParameterPointers();
    static float valueOf (const std::atomic<float>* parameter) noexcept;
    static int intValueOf (const std::atomic<float>* parameter) noexcept;

    TransportInfo readTransportInfo() const;
    void updateGlobalLfos (const TransportInfo& transport, int numSamples);
    FxModulation computeFxModulation (float lfo1Value, float lfo2Value) const;

    void prepareEffects (double sampleRate, int maximumBlockSize);
    void processChorus (juce::AudioBuffer<float>& buffer, int numSamples,
                        bool enabled, float rate, float depth, float mix, float feedback);
    void processDelay (juce::AudioBuffer<float>& buffer, int numSamples,
                       bool enabled, float mix, float feedback, bool pingPong, float toneHz);
    void processReverb (juce::AudioBuffer<float>& buffer, int numSamples,
                        bool enabled, float mix, float size, float damping, float width);
    void processMaster (juce::AudioBuffer<float>& buffer, int numSamples, float drive,
                        float lowDb, float midDb, float highDb);
    void updateMetersAndScope (const juce::AudioBuffer<float>& buffer, int numSamples);

    void pushScopeSample (float sample);
    FxRefs refs;

    juce::AudioProcessorValueTreeState apvts;
    juce::Synthesiser synthesiser;
    juce::MidiKeyboardState keyboardState;
    juce::MidiBuffer midiScratch;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR;
    juce::dsp::Reverb reverb;

    StereoBiquad eqLow;
    StereoBiquad eqMid;
    StereoBiquad eqHigh;

    OnePoleLowPass delayToneFilterL;
    OnePoleLowPass delayToneFilterR;

    GlobalLfo lfo1;
    GlobalLfo lfo2;
    std::array<float, maxLfoBufferSize> lfoBuffer1 {};
    std::array<float, maxLfoBufferSize> lfoBuffer2 {};

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothedL;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothedR;

    juce::AbstractFifo scopeFifo { scopeCapacity };
    std::array<float, scopeCapacity> scopeData {};
    int scopeDecimation = 4;
    int scopeCounter = 0;

    double currentSampleRate = 44100.0;
    int maximumBlockSize = 512;
    bool delayTimeInitialised = false;
    bool lfoTransportWasPlaying = false;

    std::atomic<float> peakLeft { 0.0f };
    std::atomic<float> peakRight { 0.0f };
    std::atomic<float> bpmForUi { 120.0f };
    std::atomic<bool> transportRunning { false };
    std::atomic<float> modWheelUi { 0.0f };
    std::atomic<float> aftertouchUi { 0.0f };
    std::atomic<float> gainReductionUi { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthwaveLegacyProcessor)
};

} // namespace synthwave
