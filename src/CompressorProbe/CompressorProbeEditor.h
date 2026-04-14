#pragma once

#include "CompressorProbeProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class RoleSelectionOverlay : public juce::Component
{
public:
    std::function<void(Role)> on_role_selected;

    RoleSelectionOverlay();
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::TextButton generator_btn{"Generator"};
    juce::TextButton probe_btn{"Probe"};
    juce::Label generator_hint;
    juce::Label probe_hint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoleSelectionOverlay)
};

class CompressorProbeEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit CompressorProbeEditor(CompressorProbeProcessor& p);
    ~CompressorProbeEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void role_selected(Role r);
    void build_role_ui();
    void timerCallback() override;
    void refresh_generator_ui();
    void refresh_probe_ui();

    CompressorProbeProcessor& probe_processor;
    std::unique_ptr<RoleSelectionOverlay> role_overlay;
    juce::Label title_label;
    juce::Label mode_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeEditor)
};
