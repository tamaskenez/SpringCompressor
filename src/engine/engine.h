#pragma once

#include "TransferCurve.h"

#include <cmath>
#include <memory>
#include <span>
#include <vector>

constexpr double k_rms_sample_period_sec = 0.001;

struct TransferCurvePars;

enum class GainControlApplication {
    on_abs_input,
    on_squared_input,
    on_gr_db,
    on_gr_mag
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

    virtual std::optional<TransferCurveState> set_transfer_curve(const TransferCurvePars& p) = 0;
    virtual void set_attack_ms(float ms) = 0;
    virtual void set_release_ms(float ms) = 0;
    virtual void set_gain_control_application(GainControlApplication application) = 0;

    [[nodiscard]] virtual TransferCurveState get_transfer_curve_state() const = 0;
    [[nodiscard]] virtual float get_rms_sample_period_sec() const = 0;
    [[nodiscard]] virtual const std::vector<AF2>& get_rms_samples_of_last_block() const = 0;
};

std::unique_ptr<Engine> make_engine();
