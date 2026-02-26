#include "engine.h"

#include <algorithm>
#include <cmath>
#include <vector>

static constexpr float kFloorDb = -120.0f;

struct EngineImpl : public Engine {
    double sampleRate = 44100.0;

    float thresholdDb = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupGainDb = 0.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    std::vector<float> envelope; // per-channel gain-reduction state (dB, <= 0)

    void prepare_to_play(double sr, int /*maxBlockSize*/, int numChannels) override
    {
        sampleRate = sr;
        envelope.assign(numChannels, 0.0f);
        updateCoefficients();
    }

    void release_resources() override
    {
        std::ranges::fill(envelope, 0.0f);
    }

    // Feedforward peak compressor (Giannoulis et al. 2013, type 1).
    void process_block(std::span<float* const> channelData, int numSamples) override
    {
        const float makeupLinear = std::pow(10.0f, makeupGainDb / 20.0f);

        for (auto* channel_buf : channelData) {
            for (int i = 0; i < numSamples; ++i) {
                channel_buf[i] *= makeupLinear;
            }
        }
    }

    void setThresholdDb(float dB) override
    {
        thresholdDb = dB;
    }
    void setRatio(float r) override
    {
        ratio = r;
    }
    void setMakeupGainDb(float dB) override
    {
        makeupGainDb = dB;
    }

    void setAttackMs(float ms) override
    {
        attackMs = ms;
        updateCoefficients();
    }

    void setReleaseMs(float ms) override
    {
        releaseMs = ms;
        updateCoefficients();
    }

    void updateCoefficients()
    {
        // alpha = exp(-1 / (tau_samples)) where tau = time_ms * sampleRate / 1000
        attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * attackMs * 0.001f));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * releaseMs * 0.001f));
    }
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
