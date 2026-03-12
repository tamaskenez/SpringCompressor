#include "RecursiveCrossoverRMSDetector.h"

#include "engine_util.h"
#include "meadow/math.h"

namespace RecursiveCrossoverRMSDetectorDetail
{
void Bufs::reserve(size_t N)
{
    crossover_input.reserve(N);
    highpass.reserve(N);
    total_power.reserve(N);
}
size_t Bufs::capacity() const
{
    const auto c = crossover_input.capacity();
    assert(highpass.capacity() == c && total_power.capacity() == c);
    return c;
}
} // namespace RecursiveCrossoverRMSDetectorDetail

RecursiveCrossoverRMSDetector::RecursiveCrossoverRMSDetector(const Pars& pars, size_t max_block_size)
{
    assert(0 < pars.freq_lo_hps && pars.freq_lo_hps < pars.freq_hi_hps && pars.freq_hi_hps < 1);
    assert(pars.crossovers_per_octaves > 0);
    assert(in_cc_range(pars.bpf_order, 1, 2));
    assert(in_cc_range(pars.low_pass_filter.order, 1, 2));
    assert(!pars.low_pass_filter.attack_time_samples || 0 <= *pars.low_pass_filter.attack_time_samples);
    assert(0 < pars.low_pass_filter.release_ratio_to_period);
    assert(0 < pars.max_release_freq_hps);

    bufs.reserve(max_block_size);

    // Calculate the crossover frequencies.
    const double freq_beta = pow(2, -1.0 / pars.crossovers_per_octaves);
    vector<double> freqs;
    freqs.reserve(iceil<size_t>(log(pars.freq_lo_hps / pars.freq_hi_hps) / log(freq_beta)) + 1);
    for (auto freq = pars.freq_hi_hps;; freq *= freq_beta) {
        freqs.push_back(freq);
        if (freq <= pars.freq_lo_hps) {
            break;
        }
    }

    // Initialize crossover_variant.
    switch (pars.bpf_order) {
    case 1: {
        auto cs = vector<Crossover1>();
        cs.reserve(freqs.size());
        crossovers_variant = std::move(cs);
    } break;
    case 2: {
        auto cs = vector<Crossover2>();
        cs.reserve(freqs.size());
        crossovers_variant = std::move(cs);
    } break;
    default:
        assert(false);
    }

    envelope_filters.reserve(freqs.size());

    optional<double> release_freq_hps_equivalent_of_attack_time;
    if (pars.low_pass_filter.attack_time_samples) {
        release_freq_hps_equivalent_of_attack_time =
          samples_to_freq_hps(*pars.low_pass_filter.attack_time_samples) / (2 * num::pi);
    }

    for (auto freq : freqs) {
        switch (pars.bpf_order) {
        case 1:
            std::get<vector<Crossover1>>(crossovers_variant)
              .emplace_back(
                Crossover1{
                  .highpass = FirstOrderIIR_TDF2{matlab::butter(1, matlab::FilterType::HighPass{freq})},
                  .lowpass = FirstOrderIIR_TDF2{matlab::butter(1, matlab::FilterType::LowPass{freq})}
                }
              );
            break;
        case 2:
            std::get<vector<Crossover2>>(crossovers_variant)
              .emplace_back(
                Crossover2{
                  .highpass = Biquad_TDF2{matlab::butter(2, matlab::FilterType::HighPass{freq})},
                  .lowpass = Biquad_TDF2{matlab::butter(2, matlab::FilterType::LowPass{freq})}
                }
              );
            break;
        default:
            assert(false);
        }
        auto attack_time_samples = pars.low_pass_filter.attack_time_samples;
        auto release_freq_hps =
          std::min(freq / pars.low_pass_filter.release_ratio_to_period, pars.max_release_freq_hps);

        // If release time is smaller than attack time (that is, release freq is greater then the freq equivalent of
        // attack time), use a symmetric filter with the release time equivalent to attack time.
        if (release_freq_hps_equivalent_of_attack_time
            && release_freq_hps >= *release_freq_hps_equivalent_of_attack_time) {
            attack_time_samples = nullopt;
            release_freq_hps = *release_freq_hps_equivalent_of_attack_time;
        }

        envelope_filters.emplace_back(
          EnvelopeFilterOutputType::power, pars.low_pass_filter.order, attack_time_samples, release_freq_hps
        );
    }
}

template<class Crossovers>
void RecursiveCrossoverRMSDetector::process_core(span<float> samples, Crossovers& crossovers)
{
    assert(crossovers.size() == envelope_filters.size());
    const auto N = samples.size();
    assert(N <= bufs.capacity());
    bufs.crossover_input.resize(N);
    std::copy(samples.begin(), samples.end(), bufs.crossover_input.begin());
    bufs.total_power.assign(N, 0.0);
    for (size_t i = 0; /* breaks from body */
         ;
         i++) {
        auto& c = crossovers[i];
        bufs.highpass = bufs.crossover_input;
        c.highpass.process(bufs.highpass);
        envelope_filters[i].process(bufs.highpass);
        for (unsigned j = 0; j < N; ++j) {
            bufs.total_power[j] += bufs.highpass[j];
        }
        if (i + 1 == crossovers.size()) {
            break;
        }
        c.lowpass.process(bufs.crossover_input);
    }
    for (unsigned j = 0; j < N; ++j) {
        samples[j] = ffcast<float>(std::sqrt(bufs.total_power[j]));
    }
}

void RecursiveCrossoverRMSDetector::process(span<float> samples)
{
    std::visit(
      overloaded{
        [samples, this](vector<Crossover1>& cs) {
            process_core(samples, cs);
        },
        [samples, this](vector<Crossover2>& cs) {
            process_core(samples, cs);
        }
      },
      crossovers_variant
    );
}
