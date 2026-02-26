#pragma once

#include <memory>
#include <span>

class Engine
{
public:
    virtual ~Engine() = default;

    virtual void prepare_to_play(double sampleRate, int maxBlockSize, int numChannels) = 0;
    virtual void process_block(std::span<float* const> channel_data, int num_samples) = 0;
    virtual void release_resources() = 0;

    virtual void set_threshold_db(float dB) = 0;
    virtual void set_ratio(float ratio) = 0;
    virtual void set_attack_ms(float ms) = 0;
    virtual void set_release_ms(float ms) = 0;
    virtual void set_makeup_gain_db(float dB) = 0;
};

std::unique_ptr<Engine> make_engine();
