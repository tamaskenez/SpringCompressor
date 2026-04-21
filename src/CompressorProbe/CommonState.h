#pragma once

#include "Modes.h"

#include <optional>

// State common to both roles.
struct CommonState {
    struct PreparedToPlay {
        double sample_rate;
        int samples_per_block;
    };

    optional<PreparedToPlay> prepared_to_play;

    // For audio thread.
    int next_process_block_index = 0;
};
