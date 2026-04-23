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
    }
}
