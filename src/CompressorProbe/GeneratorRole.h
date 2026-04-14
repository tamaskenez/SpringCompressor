#pragma once

#include "ProbeProtocol.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <memory>

enum class GeneratorStatus {
    Idle,
    TransmittingId,
    Connected
};

class GeneratorRole
{
public:
    GeneratorRole() = default;
    ~GeneratorRole();

    // Called from prepareToPlay (audio thread).
    void prepare();

    // Called from processBlock (audio thread). Passes audio through when Connected.
    void process_block(juce::AudioBuffer<float>& buffer);

    // UI-thread-safe accessors.
    int get_id() const noexcept
    {
        return id.load();
    }
    GeneratorStatus get_status() const noexcept
    {
        return status.load();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorRole)

private:
    void setup();

    std::atomic<int> id{-1}; // -1 until claimed; 0-65535 once a pipe is created
    std::atomic<GeneratorStatus> status{GeneratorStatus::Idle};
    std::unique_ptr<juce::InterprocessConnection> pipe;
    std::array<float, ProbeProtocol::nfft> tone_buf{};
    int tone_playhead = 0;
};
