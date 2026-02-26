#pragma once

// Low-pass filter using the physical simulation of a mass on a spring.
// The input sample is the free end of the spring, the other end with the mass is the output.
class SpringLowPass
{
public:
    explicit SpringLowPass(double sample_rate_arg);
    double process(double sample_in);

    // Set spring_constant and damping_constant such that the resulting system will be critically damped with a time
    // constant of tau.
    void set_critically_damped_with_time_constant(double tau);

private:
    double sample_rate;
    double spring_constant = 1, damping_constant = 1;
    double mass_position = 0, mass_velocity = 0;
};
