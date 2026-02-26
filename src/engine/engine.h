#pragma once

#include <cmath>
#include <memory>
#include <span>
#include <vector>

class Engine
{
public:
    struct Trace {
        double smoothed_signal_power = NAN, gain = NAN;
    };
    virtual ~Engine() = default;

    virtual void prepare_to_play(double sampleRate, int maxBlockSize, int numChannels) = 0;
    // trace_block is for debugging, returns internal data for channel 0, only if !NDEBUG
    virtual void
    process_block(std::span<float* const> channel_data, int num_samples, std::vector<Trace>* trace_block = nullptr) = 0;
    virtual void release_resources() = 0;

    virtual void set_threshold_db(float dB) = 0;
    virtual void set_ratio(float ratio) = 0;
    virtual void set_attack_ms(float ms) = 0;
    virtual void set_release_ms(float ms) = 0;
    virtual void set_makeup_gain_db(float dB) = 0;
};

std::unique_ptr<Engine> make_engine();
