#include "sync.h"

#include <cmath>
#include <numbers>
#include <random>

namespace
{
constexpr size_t k_sync_signal_num_digits = 7;
constexpr size_t k_sync_signal_digit_length_samples = 17;
constexpr double k_sync_signal_amplitude_dbfs = -24;

array<bool, k_sync_signal_num_digits> generate_fingerprint()
{
    std::mt19937 rng(42);
    std::bernoulli_distribution bit_dist;
    array<bool, k_sync_signal_num_digits> digits;
    for (auto& d : digits) {
        d = bit_dist(rng);
    }
    return digits;
}

const auto k_sync_fingerprint = generate_fingerprint();

array<float, k_sync_signal_digit_length_samples> generate_ref_period()
{
    constexpr size_t n = k_sync_signal_digit_length_samples;
    array<float, n> ref{};
    for (size_t i = 0; i < n; ++i) {
        ref[i] = ffcast<float>(std::sin(2.0 * std::numbers::pi * ifcast<double>(i) / ifcast<double>(n)));
    }
    return ref;
}

const auto k_sync_ref_period = generate_ref_period();

} // namespace

// Generate BPSK-style sync signal.
// - generate k_sync_signal_num_digits random binary digits, from a fixed seed so it'll be the same every time.
// - generate a single period of sine, amplitude: k_sync_signal_amplitude_dbfs
// - concatenate k_sync_signal_num_digits periods of sine, invert the phase if the digit is 1
vector<float> make_sync_signal()
{
    const float amplitude = ffcast<float>(std::pow(10.0, k_sync_signal_amplitude_dbfs / 20.0));
    constexpr size_t n = k_sync_signal_digit_length_samples;
    vector<float> one_period(n);
    for (size_t i = 0; i < n; ++i) {
        one_period[i] =
          amplitude * ffcast<float>(std::sin(2.0 * std::numbers::pi * ifcast<double>(i) / ifcast<double>(n)));
    }

    vector<float> result(k_sync_signal_num_digits * n);
    for (size_t d = 0; d < k_sync_signal_num_digits; ++d) {
        const float sign = k_sync_fingerprint[d] ? -1.0f : 1.0f;
        for (size_t i = 0; i < n; ++i) {
            result[d * n + i] = sign * one_period[i];
        }
    }
    return result;
}

// Perform normalized correlation of x and y.
// - x and y are expected to be of same length
// - perform all internal calculation in double
double normalized_correlation(span<const float> x, span<const float> y)
{
    CHECK(x.size() == y.size());
    double dot = 0.0, norm_x = 0.0, norm_y = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        const double xi = x[i], yi = y[i];
        dot += xi * yi;
        norm_x += xi * xi;
        norm_y += yi * yi;
    }
    const double denom = std::sqrt(norm_x * norm_y);
    const double result = dot / denom;
    return std::isfinite(result) ? std::clamp(result, -1.0, 1.0) : 0.0;
}

// Decode the bpsk-style signal and check if it results in the expected random sequence.
// - The signal is expected to have the exact same length and structure as what produced by make_sync_signal()
// - Use k_sync_fingerprint for the random sequence.
bool test_sync_signal(span<const float> x)
{
    constexpr size_t n = k_sync_signal_digit_length_samples;
    CHECK(x.size() == k_sync_signal_num_digits * n);

    for (size_t d = 0; d < k_sync_signal_num_digits; ++d) {
        const double corr = normalized_correlation(x.subspan(d * n, n), k_sync_ref_period);
        const bool decoded = (corr < 0.0);
        if (decoded != k_sync_fingerprint[d]) {
            return false;
        }
    }
    return true;
}
