#pragma once

#include "GeneratorStatus.h"
#include "Role.h"
#include "config.h"

#include <meadow/cppext.h>

class FileLogSink;
class RoleInterface;

namespace juce
{
class AudioProcessor;
}

constexpr int k_invalid_generator_id = -1;

struct CompressorProbeThreadSafeState {
    unique_ptr<FileLogSink> file_log_sink;
    std::atomic<int> num_channels = k_default_buses_layout.size();
    std::atomic<RoleInterface*> role_impl;
    std::atomic<GeneratorStatus> generator_status{GeneratorStatus::Idle}; // Only when role is generator.
    std::atomic<int> generator_id_in_probe = k_invalid_generator_id;
    std::atomic<int> generator_id_in_generator = k_invalid_generator_id;
    std::atomic<bool> prepared_to_play;

    explicit CompressorProbeThreadSafeState(juce::AudioProcessor* instance);
    ~CompressorProbeThreadSafeState();
    optional<Role> get_role() const;
};
