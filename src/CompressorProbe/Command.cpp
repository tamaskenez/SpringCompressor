#include "Command.h"

#include <magic_enum/magic_enum.hpp>

namespace Command
{
std::string Stop::to_string() const
{
    return "Connected, idle";
}

std::string DecibelCycle::to_string() const
{
    return std::format(
      "Cycling from {:.2f} to {:.2f} dBFS in {:.3f} s with a sine of {} Hz.",
      min_dbfs,
      max_dbfs,
      period_samples * num_periods / fs,
      iround<int>(period_samples / fs)
    );
}

std::string to_string(const V& v)
{
    return std::visit(
      [](auto&& x) {
          return x.to_string();
      },
      v
    );
}
int command_index(const Command::V& v)
{
    return std::visit(
      [](auto&& x) {
          return x.command_index;
      },
      v
    );
}
} // namespace Command

expected<Command::V, string> command_from_span(span<const char> memory_block)
{
    static_assert(std::is_trivially_copyable_v<Command::V>);
    if (memory_block.size() != sizeof(Command::V)) {
        return unexpected(
          format("memory_block size ({}) != sizeof(Command::V) ({})", memory_block.size(), sizeof(Command::V))
        );
    }
    Command::V cmd;
    std::memcpy(&cmd, memory_block.data(), sizeof(cmd));

    using ut = std::underlying_type_t<Command::E>;
    if (!std::in_range<ut>(cmd.index()) || !magic_enum::enum_cast<Command::E>(iicast<ut>(cmd.index()))) {
        return unexpected(format("Invalid Command::V::index(): {}", cmd.index()));
    }
    return cmd;
}

expected<Response, string> response_from_span(span<const char> memory_block)
{
    static_assert(std::is_trivially_copyable_v<Response>);
    if (memory_block.size() != sizeof(Response)) {
        return unexpected(
          format("memory_block size ({}) != sizeof(Response) ({})", memory_block.size(), sizeof(Response))
        );
    }
    Response response;
    std::memcpy(&response, memory_block.data(), sizeof(response));
    return response;
}
