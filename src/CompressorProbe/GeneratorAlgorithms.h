#pragma once

class DecibelCycleGenerator
{
public:
    double min_dbfs, max_dbfs;
    int period_samples;
    int num_periods;
    int total_samples;
    double rads_per_sample;

    int sample_in_period = 0;
    int period_in_cycle = 0;
    DecibelCycleGenerator(double min_dbfs_arg, double max_dbfs_arg, int period_samples_arg, int num_periods_arg)
        : min_dbfs(min_dbfs_arg)
        , max_dbfs(max_dbfs_arg)
        , period_samples(period_samples_arg)
        , num_periods(num_periods_arg)
        , total_samples(period_samples * num_periods)
        , rads_per_sample(2 * num::pi / period_samples)
    {
    }

    double decibel_for_sample(int sample_ix) const
    {
        return std::lerp(max_dbfs, min_dbfs, abs(2.0 * sample_ix / total_samples - 1.0));
    }

    void generate(span<float> block)
    {
        for (auto& s : block) {
            auto A = matlab::db2mag(decibel_for_samples(period_in_cycle * period_samples + sample_in_period));
            s = A * sin(sample_in_period * rads_per_sample);
            if (++sample_in_period == period_samples) {
                sample_in_period = 0;
                if (++period_in_cycle == num_periods) {
                    period_in_cycle = 0;
                }
            }
        }
    }
};
