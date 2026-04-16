#pragma once

#include "Role.h"
#include "juce_util/logging.h"

#include <optional>

// State common to both roles.
struct CommonState {
    struct PreparedToPlay {
        double sample_rate;
        int samples_per_block;
    };

    shared_ptr<monostate> token = make_shared<monostate>();
    FileLogSink* file_log_sink = nullptr;
    optional<Role> role;
    optional<int> generator_id;
    optional<PreparedToPlay> prepared_to_play;
    optional<string> error;
};
