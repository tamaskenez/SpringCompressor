#include "StateVariableTPTFilter.h"

#include <meadow/math.h>

template<class IOFloat>
StateVariableTPTFilter<IOFloat>::StateVariableTPTFilter()
{
    update();
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_type(Type type)
{
    filter_type = type;
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::set_cutoff_freq(double freq_hz)
{
    cutoffFrequency = freq_hz;
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
    g = std::tan(num::pi * cutoffFrequency / sample_rate);
    R2 = 1.0 / resonance;
    h = 1.0 / (1.0 + R2 * g + g * g);
}

template<class IOFloat>
void StateVariableTPTFilter<IOFloat>::process(std::span<IOFloat> samples)
{
    for (auto& x : samples) {
        const double xd = ffcast<double>(x);
        const double yHP = h * (xd - (R2 + g) * s1 - s2);
        const double yBP = g * yHP + s1;
        s1 = 2.0 * yBP - s1;
        const double yLP = g * yBP + s2;
        s2 = 2.0 * yLP - s2;
        switch (filter_type) {
        case Type::lowpass:
            x = ffcast<IOFloat>(yLP);
            break;
        case Type::bandpass:
            x = ffcast<IOFloat>(yBP);
            break;
        case Type::highpass:
            x = ffcast<IOFloat>(yHP);
            break;
        }
    }
}

template class StateVariableTPTFilter<float>;
template class StateVariableTPTFilter<double>;
