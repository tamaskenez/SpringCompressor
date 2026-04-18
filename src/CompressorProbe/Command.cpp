#include "Command.h"

#include "meadow/enum.h"

#include <magic_enum/magic_enum.hpp>

namespace
{
int s_next_command_index = 1;
}

Command::Command(const Mode::V& mode_arg)
    : command_index(s_next_command_index++)
    , mode(mode_arg)
{
}

Command::Command(span<const char> memory_block)
{
    CHECK(memory_block.size() == sizeof(*this));
    std::copy(memory_block.begin(), memory_block.end(), reinterpret_cast<char*>(this));
}

expected<Command, string> command_from_span(span<const char> memory_block)
{
    if (memory_block.size() != sizeof(Command)) {
        return unexpected(
          format("memory_block size ({}) != sizeof(Command::V) ({})", memory_block.size(), sizeof(Command))
        );
    }
    Command cmd(memory_block);
    if (!enum_cast_from_any_int<Mode::E>(cmd.mode.index())) {
        return unexpected(format("Invalid Mode::V::index(): {}", cmd.mode.index()));
    }
    return cmd;
}

expected<Response, string> response_from_span(span<const char> memory_block)
{
    if (memory_block.size() != sizeof(Response)) {
        return unexpected(
          format("memory_block size ({}) != sizeof(Response) ({})", memory_block.size(), sizeof(Response))
        );
    }
    Response response;
    std::memcpy(&response, memory_block.data(), sizeof(response));
    return response;
}

span<const char> command_as_span(const Command& cmd)
{
    return span(reinterpret_cast<const char*>(&cmd), sizeof(cmd));
}

span<const char> response_as_span(const Response& response)
{
    return span(reinterpret_cast<const char*>(&response), sizeof(response));
}
