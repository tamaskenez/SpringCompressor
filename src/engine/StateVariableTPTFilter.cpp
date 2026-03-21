#include "StateVariableTPTFilter.h"

#include <meadow/math.h>

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_type(Type type)
{
    filter_type = type;
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_cutoff_freq(double freq_hz)
{
    cutoff_freq = freq_hz;
    update();
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_resonance(double resonance_arg)
{
    resonance = resonance_arg;
    update();
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::prepare_to_play(double sample_rate_arg)
{
    sample_rate = sample_rate_arg;
    update();
    reset();
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::reset(double state)
{
    s1 = state;
    s2 = state;
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::update()
{
    if (!std::isnan(sample_rate)) {
        g = std::tan(num::pi * cutoff_freq / sample_rate);
        R2 = 1.0 / resonance;
        h = 1.0 / (1.0 + R2 * g + g * g);
    }
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::process(std::span<IOFloat> samples)
{
    assert(!std::isnan(sample_rate));
    array<double, 3> lbh;
    double& yLP = lbh[0];
    double& yBP = lbh[1];
    double& yHP = lbh[2];
    for (auto& x : samples) {
        const double xd = ffcast<double>(x);
        yHP = h * (xd - (R2 + g) * s1 - s2);
        yBP = g * yHP + s1;
        s1 = 2.0 * yBP - s1;
        yLP = g * yBP + s2;
        s2 = 2.0 * yLP - s2;
        x = ffcast<IOFloat>(lbh[std::to_underlying(filter_type)]);
    }
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_lowpass_3db_cutoff_freq_hz_Q(double freq_hz, double Q)
{
    filter_type = Type::lowpass;
    // The SVF natural frequency ω₀ ≠ the -3 dB frequency for non-Butterworth Q.
    // For H_LP(s) = 1/(s²+s/Q+1), the -3 dB point satisfies:
    //   (1-ω²)² + ω²/Q² = 2  →  ω² = [(2 - 1/Q²) + √((2 - 1/Q²)² + 4)] / 2
    // We invert this to find f0 such that -3 dB lands at freq_hz.
    const double a = 2.0 - 1.0 / (Q * Q);
    const double omega_ratio = std::sqrt((a + std::sqrt(a * a + 4.0)) / 2.0);
    cutoff_freq = freq_hz / omega_ratio;
    resonance = Q;
    update();
}

template class StateVariableTPTFilter<float>;
template class StateVariableTPTFilter<double>;
