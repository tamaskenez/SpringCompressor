#pragma once

#include <variant>

// Commands from the Probe to the Generator
namespace Command
{
// Stop the current output mode.
struct Stop {
};

// Emit periods of sine wave, the amplitude going from min_dbfs to max_dbfs and back in num_periods, which is one full
// cycle, then it repeats.
struct DecibelCycle {
    double min_dbfs, max_dbfs;
    int period_samples;
    int num_periods;
};
using V = std::variant<Stop, DecibelCycle>;

} // namespace Command
