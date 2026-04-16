#include "Command.h"

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
} // namespace Command
