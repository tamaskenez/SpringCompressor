#pragma once

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

// Return the data of the first channel and the sampling rate.
// Return nullopt on any error.
std::optional<std::pair<std::vector<float>, double>> load_audio_file_first_channel(const std::filesystem::path& path);
