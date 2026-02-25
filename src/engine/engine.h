#pragma once

#include <memory>

class Engine
{
public:
    virtual ~Engine() = default;

    virtual void prepare_to_play(double sampleRate, int maxBlockSize, int numChannels) = 0;
    virtual void process(float* const* channelData, int numChannels, int numSamples) = 0;
    virtual void release_resources() = 0;

    virtual void setThresholdDb(float dB) = 0;
    virtual void setRatio(float ratio) = 0;
    virtual void setAttackMs(float ms) = 0;
    virtual void setReleaseMs(float ms) = 0;
    virtual void setMakeupGainDb(float dB) = 0;
};

std::unique_ptr<Engine> make_engine();
