#pragma once

#include "JuceTimer.h"
#include "engine.h"
#include "meadow/evariant.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <readerwriterqueue/readerwriterqueue.h>

#include <memory>

namespace AudioToUIMsg
{
using TransferCurveState = TransferCurveState; // Needed for the EVARIANT_CASE macro.
struct RmsSamples {
    // Each AF2 item contains an input and corresponding output RMS value.
    std::inplace_vector<AF2, 16> samples;
};
EVARIANT_DECLARE_E_V(TransferCurveState, RmsSamples)
} // namespace AudioToUIMsg

class SpringCompressorProcessor
    : public juce::AudioProcessor
    , juce::AudioProcessorValueTreeState::Listener
{
public:
    SpringCompressorProcessor();
    ~SpringCompressorProcessor() override = default;

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
        return program0_name;
    }
    void changeProgramName(int, const juce::String& newName) override
    {
        program0_name = newName;
    }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // the sidechain can take any layout, the main bus needs to be the same on the input and output
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
            && !layouts.getMainInputChannelSet().isDisabled();
    }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<bool> editor_open{false};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<Engine> engine;
    struct RawParameterValues {
        using float_pointer = std::atomic<float>*;
        float_pointer threshold_db, ratio, makeup_gain_db, reference_level_db, knee_width_db;
    } raw_parameter_values;

    moodycamel::ReaderWriterQueue<AudioToUIMsg::V> audio_to_ui_queue;
    bool audio_thread_running = false;
    juce::String program0_name = "program#0";
    bool ignore_parameter_changed = false;
    std::atomic<bool> parameter_changed_was_called = false;
    std::atomic<bool> call_editor_set_transfer_curve_anyway = false;

    int rms_matrix_clock = 0; // Incremented by one on each incoming rms sample.
    // Each incoming rms sample will update the corresponding element in the rms_matrix with the current
    // rms_matrix_clock. The age of an element can be computed by `rms_matrix_clock - element_value`
    std::vector<int> rms_matrix;
    std::mdspan<int, std::dextents<int, 2>> rms_matrix_as_mdspan;

    std::vector<float> test_signal;
    size_t test_signal_playhead = 0;

    JuceTimer ui_refresh_timer;

    void parameterChanged(const juce::String&, float) override;
    void on_ui_refresh_timer_elapsed();
    void sync_engine_processor(bool called_from_audio_thread);
    void editor_set_transfer_curve(const TransferCurveState& tcs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringCompressorProcessor)
};
