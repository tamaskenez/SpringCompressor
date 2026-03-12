#pragma once

#include "EnvelopeFilter.h"
#include "FirstOrderIIR_TD2.h"

#include <meadow/cppext.h>

namespace RecursiveCrossoverRMSDetectorDetail
{
struct Bufs {
    vector<double> crossover_input;
    vector<double> highpass;
    vector<double> total_power;

    void reserve(size_t N);
    size_t capacity() const;
};

} // namespace RecursiveCrossoverRMSDetectorDetail

// A multiband RMS detector which works by feeding the signal into a tree
// of HPF/LPF crossovers and recursively splitting the LPF band.
// Each band's squared samples go into an LPF which is tuned to the lowest frequency of the band.
class RecursiveCrossoverRMSDetector
{
public:
    struct LowPassFilter {
        int order;                            // 1 or 2.
        optional<double> attack_time_samples; // Otherwise, regular, symmetrical low-pass filter will be used.
        double release_ratio_to_period;       // Of the lowest frequency of the band.
        // Attack and release parameters will be calculated from the band's lowest
        // frequency, then clamped by the corresponding min/max limits of `Pars`.
    };
    struct Pars {
        // The first crossover frequency is exactly freq_hi_hps which creates a band
        // freq_hi_hps .. Nyquist. Lower bands will be created according to the
        // crossovers_per_octave parameter.
        // The lowest band will be the first one whose crossover frequency falls below or
        // at freq_lo_lps. The LPF part of the lowest crossover will be discarded.
        double freq_lo_hps;
        double freq_hi_hps;
        double crossovers_per_octaves;
        int bpf_order;
        LowPassFilter low_pass_filter;
        double max_release_freq_hps = INFINITY;
    };

    RecursiveCrossoverRMSDetector(const Pars& pars, size_t max_block_size);
    void process(span<float> samples);

private:
    template<class FilterImplementation>
    struct Crossover {
        FilterImplementation highpass, lowpass;
    };
    using Crossover1 = Crossover<FirstOrderIIR_TDF2>;
    using Crossover2 = Crossover<Biquad_TDF2>;
    variant<vector<Crossover1>, vector<Crossover2>> crossovers_variant;
    vector<EnvelopeFilter<double>> envelope_filters;
    RecursiveCrossoverRMSDetectorDetail::Bufs bufs;

    template<class Crossovers>
    void process_core(span<float> samples, Crossovers& crossovers);
};
