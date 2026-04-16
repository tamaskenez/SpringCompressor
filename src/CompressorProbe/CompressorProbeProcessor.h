#pragma once

#include "CommonState.h"
#include "GeneratorRole.h"
#include "ProbeRole.h"
#include "ProcessorInterface.h"

#include "juce_util/JuceTimer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

class CompressorProbeEditor;
class RoleInterface;
class FileLogSink;

class CompressorProbeProcessor
    : public juce::AudioProcessor
    , public ProcessorInterface
{
public:
    CompressorProbeProcessor();
    ~CompressorProbeProcessor() override;

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override
    {
        return false;
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }

    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return {};
    }
    void changeProgramName(int, const juce::String&) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
            && !layouts.getMainInputChannelSet().isDisabled();
    }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void on_ui_refresh_timer_elapsed();
    CompressorProbeEditor* get_active_editor() const;

    // ProcessorInterface functions
    void on_role_selected_by_user(Role role) override;
    std::pair<GeneratorStatus, std::optional<std::string>> get_generator_status() const override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeProcessor)

private:
    unique_ptr<FileLogSink> file_log_sink;
    CommonState common_state;
    unique_ptr<RoleInterface> role_impl;
    JuceTimer ui_refresh_timer;

    std::atomic<RoleInterface*> role_impl_for_audio_thread;
};
