#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class SelfModEditor : public juce::AudioProcessorEditor
{
public:
    explicit SelfModEditor(SelfModProcessor&);
    ~SelfModEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelfModEditor)
};
