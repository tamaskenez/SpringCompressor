#pragma once

#include <format>
#include <string>
#include <variant>

// Commands from the Probe to the Generator
namespace Command
{
// Stop the current output mode.
struct Stop {
    std::string to_string() const;
};

// Emit periods of sine wave, the amplitude going from min_dbfs to max_dbfs and back in num_periods, which is one full
// cycle, then it repeats.
struct DecibelCycle {
    double fs;
    double min_dbfs, max_dbfs;
    int period_samples;
    int num_periods;
    std::string to_string() const;
};
using V = std::variant<Stop, DecibelCycle>;

std::string to_string(const V& v);

} // namespace Command
