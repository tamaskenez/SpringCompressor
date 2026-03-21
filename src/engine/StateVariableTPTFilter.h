#pragma once

template<class IOFloat>
class StateVariableTPTFilter
{
public:
    enum class Type : unsigned {
        lowpass,
        bandpass,
        highpass
    };

    StateVariableTPTFilter() = default;

    void set_type(Type type);
    void set_cutoff_freq(double freq_hz);
    void set_resonance(double resonance);
    void set_lowpass_3db_cutoff_freq_hz_Q(double freq_hz, double Q);

    void prepare_to_play(double sample_rate);

    void reset(double state = 0.0);

    void process(std::span<IOFloat> samples);

private:
    void update();

    double g = NAN, h = NAN, R2 = NAN;
    double s1 = 0, s2 = 0;
    Type filter_type = Type::lowpass;

    double cutoff_freq = 1000.0;
    double resonance = 1.0 / std::sqrt(2.0);

    double sample_rate = NAN;
};
