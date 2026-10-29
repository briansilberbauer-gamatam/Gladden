#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace synthwave
{
juce::StringArray presetNames();

/** Resets every parameter to its default, then applies the selected factory
    preset. Safe to call from the message thread only. */
void applyFactoryPreset (juce::AudioProcessorValueTreeState& state, int presetIndex);
} // namespace synthwave
