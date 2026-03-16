#include "SpringLowPass.h"

SpringLowPass::SpringLowPass(double sample_rate_arg)
    : sample_rate(sample_rate_arg)
{
}

void SpringLowPass::Constants::set_critically_damped_with_time_constant(double tau)
{
    // For mass=1: ω₀ = √k, critical damping ζ=1 requires c = 2√k.
    // Setting τ = 1/ω₀ gives k = 1/τ², c = 2/τ.

    tau *= sqrt(num::sqrt2 - 1); // Make it normalized to -3 dB instead of -6 dB, at 1/(2*pi*tau)

    spring = 1.0 / (tau * tau);
    damping = 2.0 / tau;
}

void SpringLowPass::set_critically_damped_with_time_constant(double tau_up, double tau_down)
{
    up_constants.set_critically_damped_with_time_constant(tau_up);
    down_constants.set_critically_damped_with_time_constant(tau_down);

    // This limit corresponds to the rule of thumb that (1/fs) must be
    // max T/10 where T is the time constant of the system.
    // It gives a -3 dB cutoff frequency of 3089 Hz for the sampling rate = 48000 Hz.
    const double min_tau = 10 / (2.0 * num::pi * sample_rate * sqrt(num::sqrt2 - 1));
    assert(min_tau <= tau_up);
    assert(min_tau <= tau_down);
}

void SpringLowPass::set_critically_damped_with_cutoff_freq(double freq_up_hz, double freq_down_hz)
{
    set_critically_damped_with_time_constant(1 / (2 * num::pi * freq_up_hz), 1 / (2 * num::pi * freq_down_hz));
}

double SpringLowPass::process(double sample_in)
{
    const double dt = 1.0 / sample_rate;

    // mass is fixed to 1.0
    // Derivatives: dy/dt = v,  dv/dt = k*(x - y) - c*v
    auto acc = [&](double y, double v) {
        const bool need_to_go_up = sample_in > y;
        const auto spring_constant = need_to_go_up ? up_constants.spring : down_constants.spring;
        const auto damping_constant = need_to_go_up ? up_constants.damping : down_constants.damping;
        return spring_constant * (sample_in - y) - damping_constant * v;
    };

    const double k1_y = mass_velocity;
    const double k1_v = acc(mass_position, mass_velocity);

    const double dt_half = dt / 2;

    const double k2_y = mass_velocity + dt_half * k1_v;
    const double k2_v = acc(mass_position + dt_half * k1_y, mass_velocity + dt_half * k1_v);

    const double k3_y = mass_velocity + dt_half * k2_v;
    const double k3_v = acc(mass_position + dt_half * k2_y, mass_velocity + dt_half * k2_v);

    const double k4_y = mass_velocity + dt * k3_v;
    const double k4_v = acc(mass_position + dt * k3_y, mass_velocity + dt * k3_v);

    const double dt_sixth = dt / 6;

    mass_position += dt_sixth * (k1_y + 2 * (k2_y + k3_y) + k4_y);
    mass_velocity += dt_sixth * (k1_v + 2 * (k2_v + k3_v) + k4_v);

    return mass_position;
}

void SpringLowPass::reset()
{
    mass_position = 0.0;
    mass_velocity = 0.0;
}
