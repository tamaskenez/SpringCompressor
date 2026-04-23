#include "Modes.h"

#include "meadow/math.h"
#include "meadow/matlab.h"

namespace
{
unsigned integer_period(double fs, int freq)
{
    return iround<unsigned>(fs / freq);
}
} // namespace

namespace Mode
{
string DecibelCycle::to_string() const
{
    return format(
      "freq={}, waveform={}, level_ref={}, min_dbfs={}, max_dbfs={}, cycle_length_index={}",
      freq,
      magic_enum::enum_name(waveform),
      magic_enum::enum_name(level_ref),
      min_dbfs,
      max_dbfs,
      cycle_length_index
    );
}
} // namespace Mode

DecibelCycleLoopGenerator::DecibelCycleLoopGenerator(double fs_arg)
    : fs(fs_arg)
    , normalized_period(integer_period(fs, k_min_decibel_cycle_freq))
{
}

void DecibelCycleLoopGenerator::compute_normalized_period(int new_freq, Waveform new_waveform, LevelRef new_level_ref)
{
    const auto period_samples = integer_period(fs, new_freq);
    auto& y = normalized_period.samples;
    CHECK(y.capacity() >= period_samples);
    y.assign(period_samples, 0.0);

    unsigned kmax = period_samples / 2;
    double dphase = 0;
    switch (new_waveform) {
    case Waveform::sine:
        kmax = 1;
        break;
    case Waveform::square:
        dphase = num::pi / 2;
        break;
    case Waveform::hi_crest_square:
        break;
    }
    double sum = 0;
    double peak = 0;
    for (unsigned i = 0; i < period_samples; ++i) {
        const double a = 2.0 * num::pi * i / period_samples;
        for (unsigned k = 1; k <= kmax; k += 2) {
            y[i] += cos(k * a + (k - 1) * dphase) / k;
        }
        sum += square(y[i]);
        peak = std::max(peak, abs(y[i]));
    }
    const double rms = sqrt(sum / period_samples);
    const double m = [&] {
        switch (new_level_ref) {
        case LevelRef::peak:
            return 1 / peak;
        case LevelRef::rms:
            return 1 / rms;
        }
    }();

    normalized_period.crest_factor = peak / rms;
    peak = 0;
    for (auto& yi : y) {
        yi *= m;
        peak = std::max(peak, abs(yi));
    }
    normalized_period.peak = peak;
}

void DecibelCycleLoopGenerator::generate_block(
  const Mode::DecibelCycle& new_params, unsigned sample_index, span<float> output_block
)
{
    bool changed = false;
    if (params.freq == INT_MIN || integer_period(fs, new_params.freq) != integer_period(fs, params.freq)
        || new_params.waveform != params.waveform || new_params.level_ref != params.level_ref) {
        compute_normalized_period(new_params.freq, new_params.waveform, new_params.level_ref);
        params.freq = new_params.freq;
        params.waveform = new_params.waveform;
        params.level_ref = new_params.level_ref;
        changed = true;
    }

    if (changed || new_params != params) {
        params = new_params;

        const auto max_dbfs_without_distortion = ifloor<int>(-matlab::mag2db(normalized_period.peak));
        min_dbfs = std::min(params.min_dbfs, max_dbfs_without_distortion);
        max_dbfs = std::min(params.max_dbfs, max_dbfs_without_distortion);
        num_periods = iround<unsigned>(
          k_decibel_cycle_lengths_msec.at(params.cycle_length_index) / 1000.0 * fs
          / ifcast<double>(normalized_period.samples.size())
        );
        cycle_length_samples = iicast<unsigned>(normalized_period.samples.size() * num_periods);
    }

    CHECK(sample_index < cycle_length_samples);

    for (auto& s : output_block) {
        const auto A = matlab::db2mag(decibel_for_sample_ix(sample_index));
        const auto period_sample_ix = sample_index % normalized_period.samples.size();
        s = ffcast<float>(A * normalized_period.samples[period_sample_ix]);
        sample_index = (sample_index + 1) % cycle_length_samples;
    }
}

double DecibelCycleLoopGenerator::decibel_for_sample_ix(unsigned sample_ix) const
{
    return std::lerp(max_dbfs, min_dbfs, abs(2.0 * sample_ix / cycle_length_samples - 1.0));
}

string_view get_label_for_enum(Mode::E e)
{
    using std::literals::operator""sv;
    switch (e) {
    case Mode::E::Bypass:
        return "Bypass plus GR tracker signal"sv;
    case Mode::E::DecibelCycle:
        return "Steady state compression curve"sv;
    case Mode::E::EnvelopeFilter:
        return "Envelope filter"sv;
    }
}

EnvelopeFilterLoopGenerator::EnvelopeFilterLoopGenerator(double fs_arg)
    : fs(fs_arg)
{
}

void EnvelopeFilterLoopGenerator::init(const Mode::EnvelopeFilter& new_params)
{
    if (params != new_params) {
        params = new_params;

        carrier_period_samples = integer_period(fs, new_params.carrier_freq);
        num_periods = iround<size_t>(
          k_decibel_cycle_lengths_msec.at(params.cycle_length_index) / 1000.0 * fs
          / ifcast<double>(carrier_period_samples)
        );
        cycle_length_samples = iicast<size_t>(carrier_period_samples * num_periods);

        const double cls = ifcast<double>(cycle_length_samples);
        T = cls / 2.0 / fs;
        f0 = new_params.min_mod_freq;
        f1 = new_params.max_mod_freq;
        k1 = 2.0 * log(f1 / f0) / cls;
        L1 = 2.0 * num::pi * f0 * T / log(f1 / f0);
        L2 = 2.0 * num::pi * f1 * T / log(f0 / f1);
        alpha_T = modulo(L1 * (f1 / f0 - 1.0), 2 * num::pi);
        P1 = pow(f1 / f0, 2.0 / cls);
        f1_over_f0 = f1 / f0;

        const double A1 = matlab::db2mag(ifcast<double>(params.max_carrier_amp_dbfs - params.mod_amp_db));
        const double A2 = matlab::db2mag(ifcast<double>(params.max_carrier_amp_dbfs));
        MC = (A1 + A2) / 2;
        MA = (A2 - A1) / 2;
    }
}

double EnvelopeFilterLoopGenerator::freq_at_sample(size_t j) const
{
    if (j < cycle_length_samples / 2) {
        return f0 * exp(k1 * ifcast<double>(j));
    } else {
        return f1 * exp(-k1 * (ifcast<double>(j) - ifcast<double>(cycle_length_samples) / 2.0));
    }
}

void EnvelopeFilterLoopGenerator::generate_block(
  const Mode::EnvelopeFilter& new_params, unsigned sample_index, span<float> output_block
)
{
    init(new_params);
    const double M = 2.0 * num::pi / ifcast<double>(carrier_period_samples);
    for (size_t i = 0; i < output_block.size(); ++i) {
        auto j = sample_index + i;
        const auto carrier = cos(M * ifcast<double>(j % carrier_period_samples));
        const double alpha_t = j < cycle_length_samples / 2
                               ? L1 * (pow(P1, ifcast<double>(j)) - 1.0)
                               : alpha_T + L2 * (pow(P1, -ifcast<double>(j)) * f1_over_f0 - 1.0);
        output_block[i] = ffcast<float>(carrier * (MC + MA * cos(alpha_t)));
    }
}
