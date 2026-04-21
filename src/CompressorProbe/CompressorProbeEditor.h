#pragma once

#include "DecibelCyclePanel.h"
#include "Role.h"
#include "WaveScope.h"
#include "juce_util/misc.h"

#include <juce_audio_processors/juce_audio_processors.h>

class ProcessorInterface;
struct CompressorProbeThreadSafeState;
class CompressorProbeMessageThreadState;
class CompressorProbeProcessorMTh;

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

class CompressorProbeEditor : public juce::AudioProcessorEditor
{
public:
    CompressorProbeEditor(
      juce::AudioProcessor& p,
      CompressorProbeThreadSafeState& ts_state,
      CompressorProbeMessageThreadState& state_mt,
      ProcessorInterface* processor_interface
    );
    ~CompressorProbeEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void refresh_ui();

    void enable_channels(bool enable);

private:
    void refresh_generator_ui();
    void refresh_probe_ui();

    CompressorProbeThreadSafeState& ts_state;
    CompressorProbeMessageThreadState& state_mt;
    ProcessorInterface* processor_interface;
    ComboBoxWithAttachment mode;
    WaveScope wave_scope;
    std::unique_ptr<DecibelCyclePanel> decibel_cycle_panel;
    std::unique_ptr<RoleSelectionOverlay> role_overlay;
    juce::Label title_label, role_label, error_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeEditor)
};
