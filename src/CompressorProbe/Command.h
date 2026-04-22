#pragma once

#include "Modes.h"

#include <meadow/evariant.h>

#include <string>
#include <variant>

// Commands from the Probe to the Generator
struct Command {
    explicit Command(std::nullopt_t) {}
    explicit Command(int command_index, const Mode::V& mode_arg);
    explicit Command(span<const char> memory_block);

    int command_index = -1;
    Mode::V mode;
};

static_assert(std::is_trivially_copyable_v<Command>);

struct Response {
    int command_index;
    chr::steady_clock::time_point command_received_timestamp;
};

static_assert(std::is_trivially_copyable_v<Response>);

expected<Command, string> command_from_span(span<const char> memory_block);
span<const char> command_as_span(const Command& cmd);
expected<Response, string> response_from_span(span<const char> memory_block);
span<const char> response_as_span(const Response& response);
