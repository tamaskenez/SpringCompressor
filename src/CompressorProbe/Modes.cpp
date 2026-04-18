#include "Modes.h"

#include "meadow/math.h"
#include "meadow/matlab.h"

DecibelCycleLoop compute_decibel_cycle(double fs, const Mode::DecibelCycle& p)
{
    const auto period_samples = iround<unsigned>(fs / p.freq);
    vector<double> y(period_samples);
    unsigned kmax = period_samples / 2;
    double dphase = 0;
    switch (p.waveform) {
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
    const double m = ([&]() {
        switch (p.level_method) {
        case LevelMethod::peak:
            return 1 / peak;
        case LevelMethod::rms:
            return 1 / rms;
        }
    })();
    UNUSED double crest_factor = peak / rms;
    peak = 0;
    for (auto& yi : y) {
        yi *= m;
        peak = std::max(peak, abs(yi));
    }
    const auto max_dbfs_without_distortion = ifloor<int>(-matlab::mag2db(peak));
    vector<float> loop;
    const auto min_dbfs = std::min(p.min_dbfs, max_dbfs_without_distortion);
    const auto max_dbfs = std::min(p.max_dbfs, max_dbfs_without_distortion);
    const auto num_periods =
      iround<unsigned>(k_decibel_cycle_lengths_msec.at(p.cycle_index) / 1000.0 * fs / period_samples);
    const auto cycle_length_samples = period_samples * num_periods;
    loop.reserve(cycle_length_samples);

    const auto decibel_for_sample = [cycle_length_samples, min_dbfs, max_dbfs](unsigned sample_ix) {
        return std::lerp(max_dbfs, min_dbfs, abs(2.0 * sample_ix / cycle_length_samples - 1.0));
    };

    for (unsigned i = 0, j = 0; i < cycle_length_samples; ++i, j = (j + 1) % period_samples) {
        const auto A = matlab::db2mag(decibel_for_sample(i));
        loop.push_back(ffcast<float>(A * y[i]));
    }
    return {period_samples, MOVE(loop)};
}
