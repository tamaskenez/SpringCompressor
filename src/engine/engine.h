#pragma once

#include "TransferCurve.h"

#include <cmath>
#include <memory>
#include <span>
#include <vector>

constexpr double k_rms_sample_period_sec = 0.001;

struct TransferCurvePars;

struct EnginePars {
    // First the signal (or the sidechain) needs to be converted to dB values.
    // There are multiple options, determined by the input_level_method.
    enum class InputLevelMethod {
        direct,   // Convert sample-by-sample, no filtering.
        lpf,      // Calculate RMS with a low-pass filter.
        multiband // Calculate RMS with separate low-pass filters for different frequencies.
    } input_level_method = InputLevelMethod::direct;

    // Parameters of the single low-pass filter.
    struct InputLevelLPF {
        bool rms = false;                // Whether to measure amplitude or RMS.
        int order = 1;                   // filter order, 1 or 2
        optional<float> attack_time_sec; // 1/cutoff.
        float release_time_sec = 0.1f;   // 1/cutoff.
        bool operator==(const InputLevelLPF&) const = default;
    } input_level_lpf;

    // Parameters of the multiband RMS detector.
    struct InputLevelMultiband {
        float freq_lo_hz = 20;    // Lowest freq we're interested in to measure (actual could be lower).
        float freq_hi_hz = 10240; // Highest crossover, the topmost band will be freq_hi to Nyquist.
        float crossovers_per_octave = 1;
        int crossover_order = 2;             // 1 or 2
        int lp_order = 1;                    // 1 or 2
        float lp_ratio = 2;                  // LPF's period (= 1/cutoff) to period of band's lowest freq
        float min_release_time_sec = 0.001f; // Min LPF period (1/cutoff)
        bool operator==(const InputLevelMultiband&) const = default;
    } input_level_multiband;

    // The dB values will be transformed by the compressor's transform curve.
    TransferCurvePars transfer_curve;

    // The output of the transform curve (GR = gain reduction signal) can be further smoothed by a low-pass filter.
    enum class GRFilterMode {
        off, // No smoothing.
        mag, // Filter the linear gain values.
        pow, // Filter the squared gain values.
        db   // Filter the gain values in dB.
    } gr_filter_mode = GRFilterMode::off;
    struct GRFilter // Gain-reduction filter.
    {
        int order = 1;                   // 1 or 2.
        optional<float> attack_time_sec; // 1/cutoff
        float release_time_sec = 0.1f;   // 1/cutoff
        bool operator==(const GRFilter&) const = default;
    } gr_filter;
};

class Engine
{
public:
    struct Trace {
        double smoothed_signal_power = NAN, gain = NAN;
    };
    virtual ~Engine() = default;

    virtual void prepare_to_play(double sampleRate, int maxBlockSize, int numChannels) = 0;
    // trace_block is for debugging, returns internal data for channel 0, only if !NDEBUG
    virtual void process_block(std::span<float* const> channel_data, int num_samples) = 0;
    virtual std::vector<Trace> process_block_with_trace(std::span<float* const> channel_data, int num_samples) = 0;
    virtual void release_resources() = 0;

    enum class SetParsResult {
        transfer_curve_didnt_change,
        transfer_curve_changed
    };
    [[nodiscard]] virtual SetParsResult set_pars(const EnginePars& pars) = 0;

    [[nodiscard]] virtual TransferCurveState get_transfer_curve_state() const = 0;
    [[nodiscard]] virtual const std::vector<AF2>& get_rms_samples_of_last_block() const = 0;
};

std::unique_ptr<Engine> make_engine();
