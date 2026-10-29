#include "PluginProcessor.h"

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
bool isFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;

    return true;
}

float peakOf (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = juce::jmax (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
    return peak;
}
} // namespace

int main (int argc, char* argv[])
{
    using namespace synthwave;

    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    SynthwaveLegacyProcessor processor;
    processor.setRateAndBufferSizeDetails (48000.0, 256);
    processor.prepareToPlay (48000.0, 256);

    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, 0.90f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.85f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 55, 0.80f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.78f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.70f), 0);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 96), 0);

    juce::AudioBuffer<float> buffer (2, 256);
    float maxPeak = 0.0f;
    double energy = 0.0;

    for (int block = 0; block < 300; ++block)
    {
        buffer.clear();
        auto events = midi;
        if (block > 0)
            events.clear();

        processor.processBlock (buffer, events);

        if (! isFinite (buffer))
        {
            std::cerr << "FAIL: non-finite sample generated in block " << block << "\n";
            return 1;
        }

        maxPeak = juce::jmax (maxPeak, peakOf (buffer));
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                energy += (double) buffer.getSample (channel, sample)
                        * (double) buffer.getSample (channel, sample);
    }

    const auto rms = std::sqrt (energy / (300.0 * 256.0 * 2.0));
    if (rms < 1.0e-4)
    {
        std::cerr << "FAIL: synth produced silence (RMS " << rms << ")\n";
        return 1;
    }

    if (maxPeak > 8.0f)
    {
        std::cerr << "FAIL: runaway output peak " << maxPeak << "\n";
        return 1;
    }

    // Scope FIFO must be readable and contain finite data.
    std::array<float, 512> scope {};
    const auto scopeSamples = processor.readScopeSamples (scope.data(), (int) scope.size());
    for (int i = 0; i < scopeSamples; ++i)
    {
        if (! std::isfinite (scope[(size_t) i]))
        {
            std::cerr << "FAIL: non-finite scope sample\n";
            return 1;
        }
    }

    // State round-trip.
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    if (state.getSize() == 0)
    {
        std::cerr << "FAIL: empty plugin state\n";
        return 1;
    }

    {
        // Deterministic all-parameter state round-trip regression test.
        std::vector<std::pair<juce::RangedAudioParameter*, float>> savedValues;
        int seed = 0;
        for (auto* parameter : processor.getParameters())
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            {
                seed = seed * 1664525 + 1013904223;
                auto value = (float) ((seed & 0xffff) / 65535.0);
                if (ranged->isDiscrete())
                    value = (seed & 0x10000) != 0 ? 1.0f : 0.0f;
                ranged->setValueNotifyingHost (value);
            }
        }

        processor.getStateInformation (state);
        for (auto* parameter : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                savedValues.emplace_back (ranged, ranged->getValue());

        for (auto* parameter : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                ranged->setValueNotifyingHost (1.0f - ranged->getValue());

        processor.setStateInformation (state.getData(), (int) state.getSize());

        int failures = 0;
        for (const auto& entry : savedValues)
        {
            const auto actual = entry.first->getValue();
            if (std::abs (actual - entry.second) > 1.0e-4f)
            {
                std::cerr << "STATE FAIL " << entry.first->paramID
                          << " expected=" << entry.second << " actual=" << actual << "\n";
                ++failures;
            }
        }

        if (failures != 0)
        {
            std::cerr << "FAIL: " << failures << " parameters not restored\n";
            return 1;
        }
    }

    auto* master = processor.getValueTreeState().getParameter (params::masterGain);
    if (master == nullptr)
        return 1;

    const auto originalMaster = master->getValue();
    master->setValueNotifyingHost (0.9f);

    processor.setStateInformation (state.getData(), (int) state.getSize());
    const auto restoredMaster = processor.getValueTreeState().getParameter (params::masterGain)->getValue();
    if (std::abs (originalMaster - restoredMaster) > 1.0e-5f)
    {
        std::cerr << "FAIL: parameter state was not restored\n";
        return 1;
    }

    // Mirror pluginval's state-restoration walk: change one automatable
    // parameter, restore, and verify. This catches discrete/boolean edge cases.
    {
        SynthwaveLegacyProcessor fresh;
        fresh.setRateAndBufferSizeDetails (48000.0, 256);
        fresh.prepareToPlay (48000.0, 256);
        juce::MemoryBlock freshState;
        fresh.getStateInformation (freshState);

        int failures = 0;
        for (auto* parameter : fresh.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);
            if (ranged == nullptr || ! ranged->isAutomatable() || ranged->isMetaParameter())
                continue;

            const auto original = ranged->getValue();
            const auto testValue = ranged->isDiscrete() ? (original >= 0.5f ? 0.0f : 1.0f) : 0.243098f;
            ranged->setValueNotifyingHost (testValue);
            fresh.setStateInformation (freshState.getData(), (int) freshState.getSize());

            if (std::abs (ranged->getValue() - original) > 0.1f)
            {
                std::cerr << "STATE WALK FAIL " << ranged->paramID
                          << " original=" << original << " restored=" << ranged->getValue() << "\n";
                ++failures;
            }
        }

        if (failures != 0)
        {
            std::cerr << "FAIL: state restoration walk had " << failures << " failures\n";
            return 1;
        }

        fresh.releaseResources();
    }

    // Exercise every factory preset and every filter mode for stability.
    const auto filterMode = processor.getValueTreeState().getParameter (params::filterMode);
    for (int preset = 0; preset < processor.getNumPrograms(); ++preset)
    {
        processor.loadFactoryPreset (preset);

        for (int mode = 0; mode < 8; ++mode)
        {
            if (auto* parameter = filterMode)
                parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) mode));

            for (int block = 0; block < 12; ++block)
            {
                buffer.clear();
                juce::MidiBuffer events;
                if (block == 0)
                {
                    events.addEvent (juce::MidiMessage::noteOn (1, 48 + mode, 0.85f), 0);
                    events.addEvent (juce::MidiMessage::noteOn (1, 60, 0.75f), 0);
                    events.addEvent (juce::MidiMessage::noteOn (1, 67, 0.70f), 0);
                }
                processor.processBlock (buffer, events);
                if (! isFinite (buffer) || peakOf (buffer) > 12.0f)
                {
                    std::cerr << "FAIL: preset " << preset << " filter mode " << mode
                              << " produced invalid output\n";
                    return 1;
                }
            }
        }
    }


    for (int i = 1; i + 1 < argc; ++i)
    {
        if (juce::String (argv[i]) == "--render-demo")
        {
            const juce::File outputFile (juce::String (argv[i + 1]));
            auto& tree = processor.getValueTreeState();
            const auto set = [&tree] (const char* id, float value)
            {
                if (auto* parameter = tree.getParameter (id))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            };

            processor.loadFactoryPreset (0);
            processor.reset();
            set (params::osc1Level, 1.0f);
            set (params::osc2Level, 0.0f);
            set (params::osc1Sub, 0.0f);
            set (params::osc2Sub, 0.0f);
            set (params::noise, 0.0f);
            set (params::filterMode, (float) FilterMode::lowPass24);
            set (params::filterCutoff, 16000.0f);
            set (params::filterResonance, 0.0f);
            set (params::filterDrive, 0.0f);
            set (params::filterEnvAmount, 0.0f);
            set (params::filterKeyTrack, 0.0f);
            set (params::ampAttack, 0.005f);
            set (params::ampDecay, 4.0f);
            set (params::ampSustain, 1.0f);
            set (params::ampRelease, 0.5f);
            set (params::chorusOn, 0.0f);
            set (params::delayOn, 0.0f);
            set (params::reverbOn, 0.0f);
            set (params::masterDrive, 0.0f);
            set (params::masterGain, 0.0f);

            processor.getKeyboardState().noteOn (1, 57, 1.0f);

            constexpr int captureSeconds = 2;
            constexpr int captureSamples = 48000 * captureSeconds;
            juce::AudioBuffer<float> capture (2, captureSamples);
            int writePos = 0;
            while (writePos < captureSamples)
            {
                const int n = juce::jmin (256, captureSamples - writePos);
                buffer.setSize (2, n, false, false, true);
                buffer.clear();
                juce::MidiBuffer events;
                processor.processBlock (buffer, events);
                for (int channel = 0; channel < 2; ++channel)
                    capture.copyFrom (channel, writePos, buffer, channel, 0, n);
                writePos += n;
            }
            buffer.setSize (2, 256, false, false, true);

            outputFile.deleteFile();
            if (auto stream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
            {
                juce::WavAudioFormat format;
                std::unique_ptr<juce::AudioFormatWriter> writer (
                    format.createWriterFor (stream.get(), 48000.0, 2, 24, {}, 0));
                if (writer != nullptr)
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer (capture, 0, captureSamples);
                    std::cout << "Demo WAV written to " << outputFile.getFullPathName() << "\n";
                }
            }
        }
    }

    // Optional visual smoke test: render the complete editor into a PNG without
    // opening a native window. This exercises layout, LookAndFeel and every
    // parameter attachment.
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (juce::String (argv[i]) == "--gui-snapshot")
        {
            const juce::File outputFile (juce::String (argv[i + 1]));

            processor.loadFactoryPreset (1);
            processor.reset();
            processor.getKeyboardState().noteOn (1, 48, 0.80f);
            processor.getKeyboardState().noteOn (1, 60, 0.85f);
            processor.getKeyboardState().noteOn (1, 64, 0.78f);
            processor.getKeyboardState().noteOn (1, 67, 0.74f);

            std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
            if (editor == nullptr)
            {
                std::cerr << "FAIL: createEditor returned null\n";
                return 1;
            }

            editor->setSize (1520, 960);
            editor->setVisible (true);

            for (int block = 0; block < 24; ++block)
            {
                buffer.clear();
                juce::MidiBuffer events;
                processor.processBlock (buffer, events);
            }

            juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

            const auto saveSnapshot = [] (juce::Component& component, const juce::File& file)
            {
                const auto image = component.createComponentSnapshot (component.getLocalBounds(), true, 1.0f);
                if (! image.isValid())
                    return false;

                file.deleteFile();
                juce::FileOutputStream stream (file);
                if (! stream.openedOk())
                    return false;

                juce::PNGImageFormat png;
                return png.writeImageToStream (image, stream);
            };

            if (! saveSnapshot (*editor, outputFile))
            {
                std::cerr << "FAIL: could not encode PNG\n";
                return 1;
            }

            std::cout << "GUI snapshot written to " << outputFile.getFullPathName() << "\n";

            const auto findAndSwitchTab = [&] (auto& self, juce::Component& component) -> void
            {
                if (auto* tabbed = dynamic_cast<juce::TabbedComponent*> (&component))
                {
                    tabbed->setCurrentTabIndex (1);
                    return;
                }

                for (auto* child : component.getChildren())
                    self (self, *child);
            };
            findAndSwitchTab (findAndSwitchTab, *editor);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (120);

            const juce::File fxFile (outputFile.getSiblingFile (
                outputFile.getFileNameWithoutExtension() + "FX.png"));
            if (! saveSnapshot (*editor, fxFile))
            {
                std::cerr << "FAIL: could not encode FX PNG\n";
                return 1;
            }

            std::cout << "GUI snapshot written to " << fxFile.getFullPathName() << "\n";

            editor->setSize (1180, 800);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
            const juce::File smallFile (outputFile.getSiblingFile (
                outputFile.getFileNameWithoutExtension() + "_small.png"));
            if (! saveSnapshot (*editor, smallFile))
            {
                std::cerr << "FAIL: could not encode small PNG\n";
                return 1;
            }
            std::cout << "GUI snapshot written to " << smallFile.getFullPathName() << "\n";

            editor.reset();
        }
    }

    processor.releaseResources();

    std::cout << "PASS: Synthwave Legacy DSP smoke test\n"
              << "  RMS=" << rms << "  peak=" << maxPeak
              << "  scopeSamples=" << scopeSamples << "\n";
    return 0;
}
