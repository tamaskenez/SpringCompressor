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

struct RecursiveCrossoverRMSDetectorPars {
    // The first crossover frequency is exactly freq_hi_hps which creates a band
    // freq_hi_hps .. Nyquist. Lower bands will be created according to the
    // crossovers_per_octave parameter.
    // The lowest band will be the first one whose crossover frequency falls below or
    // at freq_lo_hps. The LPF part of the lowest crossover will be discarded.
    double freq_lo_hps;
    double freq_hi_hps;
    double crossovers_per_octaves;
    int bpf_order;
    struct LowPassFilter {
        int order; // 1 or 2.
        // Unlike release time, the same, uniform attack time will be applied to each band, if specified.
        // If not specified, regular, symmetrical low-pass filters will be used.
        optional<double> attack_time_samples;
        double release_ratio_to_period; // Of the lowest frequency of the band.
        // Release time (1/lpf_cutoff) will be calculated from the band's lowest
        // frequency, then clamped by min_release_time_samples from below.
        // If the release_time is less than the attack_time, the attack_time will be used with a symmetrical LPF.
    } low_pass_filter;
    double min_release_time_samples = 0;
    bool output_power = false; // If false, return RMS values. if true, return RMS^2 = power.
};

// A multiband RMS detector which works by feeding the signal into a tree
// of HPF/LPF crossovers and recursively splitting the LPF band.
// Each band's squared samples go into an LPF which is tuned to the lowest frequency of the band.
template<class IOFloat>
class RecursiveCrossoverRMSDetector
{
public:
    using Pars = RecursiveCrossoverRMSDetectorPars;

    RecursiveCrossoverRMSDetector(const Pars& pars, size_t max_block_size);
    void process(span<IOFloat> samples);

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
    bool output_power = false; // See RecursiveCrossoverRMSDetectorPars::output_power.

    template<class Crossovers>
    void process_core(span<IOFloat> samples, Crossovers& crossovers);
};
