#pragma once

#include <memory>
#include <span>

class SelfModEngine
{
public:
    virtual ~SelfModEngine() = default;

    virtual void prepare_to_play(double sample_rate, int max_block_size, int num_channels) = 0;
    virtual void process_block(std::span<float* const> channel_data, int num_samples) = 0;
    virtual void release_resources() = 0;

    virtual void set_lp_freq(double freq) = 0;
    virtual void set_intensity(double intensity) = 0;
};

std::unique_ptr<SelfModEngine> make_selfmod_engine();
