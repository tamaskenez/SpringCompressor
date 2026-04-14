#pragma once

#include "GeneratorRole.h"
#include "ProbeRole.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

enum class Role {
    Unset,
    Generator,
    Probe
};

class CompressorProbeProcessor : public juce::AudioProcessor
{
public:
    CompressorProbeProcessor();
    ~CompressorProbeProcessor() override = default;

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

    Role get_role() const noexcept
    {
        return role.load();
    }
    void set_role(Role r) noexcept
    {
        role.store(r);
    }

    bool is_engine_running() const noexcept
    {
        return engine_running.load();
    }

    // Generator UI state — delegates to GeneratorRole.
    int get_generator_id() const noexcept
    {
        return generator_role.get_id();
    }
    GeneratorStatus get_generator_status() const noexcept
    {
        return generator_role.get_status();
    }

    // Probe UI state — delegates to ProbeRole.
    int get_probe_confirmed_id() const noexcept
    {
        return probe_role.get_confirmed_id();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeProcessor)

private:
    std::atomic<Role> role{Role::Unset};
    std::atomic<bool> engine_running{false};
    GeneratorRole generator_role;
    ProbeRole probe_role;
};
