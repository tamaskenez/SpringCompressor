#pragma once

// Low-pass filter using the physical simulation of a mass on a spring.
// The input sample is the free end of the spring, the other end with the mass is the output.
// The behavior can be set differently depending on whether the input signal is above or below the output signal.
class SpringLowPass
{
public:
    explicit SpringLowPass(double sample_rate_arg);

    void set_state(double mass_position_arg, double mass_velocity_arg)
    {
        mass_position = mass_position_arg;
        mass_velocity = mass_velocity_arg;
    }

    [[nodiscard]] double process(double sample_in);

    // Set spring_constant and damping_constant such that the resulting system will be critically damped with a time
    // constant of tau.
    void set_critically_damped_with_time_constant(double tau)
    {
        set_critically_damped_with_time_constant(tau, tau);
    }
    void set_critically_damped_with_time_constant(double tau_up, double tau_down);
    void set_critically_damped_with_cutoff_freq(double freq_up_hz, double freq_down_hz);
    void set_critically_damped_with_cutoff_freq(double freq_hz)
    {
        set_critically_damped_with_cutoff_freq(freq_hz, freq_hz);
    }

private:
    double sample_rate;
    struct Constants {
        double spring = 1, damping = 1;
        void set_critically_damped_with_time_constant(double tau);
    };
    Constants up_constants, down_constants;
    double mass_position = 0, mass_velocity = 0;
};
