#pragma once

template<class IOFloat>
class StateVariableTPTFilter
{
public:
    enum class Type {
        lowpass,
        bandpass,
        highpass
    };

    StateVariableTPTFilter();

    void set_type(Type type);
    void set_cutoff_freq(double freq_hz);
    void set_resonance(double resonance);

    void prepare_to_play(double sample_rate);

    void reset(double state = 0.0);

    void process(std::span<IOFloat> samples);

private:
    void update();

    double g, h, R2;
    double s1 = 0, s2 = 0;
    double sample_rate = 44100.0;
    Type filter_type = Type::lowpass;
    double cutoffFrequency = 1000.0;
    double resonance = 1.0 / std::sqrt(2.0);
};
