#include "SelfModEngine.h"

#include <meadow/matlab_signal.h>

#include <cmath>
#include <vector>

namespace
{

// Direct Form I state for a 1st-order IIR section.
// Transfer function: H(z) = (b0 + b1*z⁻¹) / (1 + a1*z⁻¹)
struct FilterState {
    double x_prev = 0.0, y_prev = 0.0;

    double process(double x, double b0, double b1, double a1)
    {
        const double y = b0 * x + b1 * x_prev - a1 * y_prev;
        x_prev = x;
        y_prev = y;
        return y;
    }
    void reset()
    {
        x_prev = 0;
        y_prev = 0;
    }
};

struct ChannelState {
    FilterState abs_filter; // tracks LP(|x|)
    FilterState sq_filter;  // tracks LP(x²)
};

} // namespace

struct SelfModEngineImpl : public SelfModEngine {
    double b0 = 0.0, b1 = 0.0, a1 = 0.0;
    std::vector<ChannelState> channel_states;
    double lp_freq = 100;
    double sample_rate = 1;
    double intensity = 1;

    void prepare_to_play(double sample_rate_arg, int /*max_block_size*/, int num_channels) override
    {
        sample_rate = sample_rate_arg;
        update_filters();
        channel_states.assign(num_channels, {});
    }

    void process_block(std::span<float* const> channel_data, int num_samples) override
    {
        for (size_t ch = 0; ch < channel_data.size() && ch < channel_states.size(); ++ch) {
            auto* buf = channel_data[ch];
            auto& state = channel_states[ch];
            for (int i = 0; i < num_samples; ++i) {
                const double x = buf[i];
                const double lp_abs = state.abs_filter.process(std::abs(x), b0, b1, a1);
                const double lp_sq = state.sq_filter.process(x * x, b0, b1, a1);
                const double m = lp_abs / sqrt(lp_sq);
                buf[i] = std::isfinite(m) ? static_cast<float>(std::pow(m, intensity) * x) : 0.0f;
            }
        }
    }

    void release_resources() override
    {
        channel_states.clear();
    }

    void set_lp_freq(double freq) override
    {
        if (abs(lp_freq - freq) > 0.001) {
            lp_freq = freq;
            update_filters();
        }
    }
    void set_intensity(double intensity_arg) override
    {
        intensity = intensity_arg;
    }
    void update_filters()
    {
        const double cutoff_norm = lp_freq / (sample_rate / 2.0);
        const auto coeffs = matlab::butter(1, matlab::FilterType::LowPass{cutoff_norm});
        b0 = coeffs.b[0];
        b1 = coeffs.b[1];
        a1 = coeffs.a[1];
        for (auto& f : channel_states) {
            f.abs_filter.reset();
            f.sq_filter.reset();
        }
    }
};

std::unique_ptr<SelfModEngine> make_selfmod_engine()
{
    return std::make_unique<SelfModEngineImpl>();
}
