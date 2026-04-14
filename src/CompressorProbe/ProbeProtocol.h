#pragma once

#include <array>

// Wire-protocol constants shared between GeneratorRole and ProbeRole.
// Both sides must use identical values to communicate correctly.
namespace ProbeProtocol
{

// Fixed FFT size for ID tone encoding/decoding.
inline constexpr int nfft = 4096;

// 18 prime bin indices, all below nfft/10 (so the highest tone stays below 20 kHz at 192 kHz).
// Bin 0 (sync_on) is always active.  Bin 1 (sync_off) is never active.
// Bins 2-17 encode the 16-bit generator ID in binary (bit 0 = bin 2, ..., bit 15 = bin 17).
inline constexpr std::array<int, 18> id_bins{
  211, 223, 233, 241, 257, 269, 281, 293, 307, 317, 331, 347, 353, 367, 379, 389, 401, 409
};

// Detection threshold for Goertzel power.
// Expected power for a tone at amplitude 0.1 over nfft samples: (0.1 * nfft/2)^2 ≈ 41 943.
// Threshold at ~1/10 of that handles up to 10 dB of compressor attenuation.
inline constexpr double detection_threshold = 4000.0;

} // namespace ProbeProtocol
