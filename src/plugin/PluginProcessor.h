#pragma once

#include "engine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <readerwriterqueue/readerwriterqueue.h>

#include <any>
#include <memory>

class SpringCompressorProcessor
    : public juce::AudioProcessor
    , private juce::AudioProcessorValueTreeState::Listener
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<Engine> engine;
    struct RawParameterValues {
        using float_pointer = std::atomic<float>*;
        float_pointer threshold_db, ratio, attack_ms, release_ms, makeup_gain_db, reference_level_db, knee_width_db,
          gain_control_application;
    } raw_parameter_values;

    TransferCurveNormalizer last_normalizer = TransferCurveNormalizer::makeup_gain;
    moodycamel::ReaderWriterQueue<TransferCurvePars> ui_to_audio_queue;
    moodycamel::ReaderWriterQueue<std::any> audio_to_ui_queue;
    bool audio_thread_running = false;
    juce::String program0_name = "program#0";

    void parameterChanged(const juce::String&, float) override;
    void engine_set_transfer_curve_and_update_ui(const TransferCurvePars& tcp);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringCompressorProcessor)
};
