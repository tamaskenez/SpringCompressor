#include "SelfModEngine.h"
#include <vector>

struct SelfModEngineImpl : public SelfModEngine {
    void prepare_to_play(double /*sample_rate*/, int /*max_block_size*/, int /*num_channels*/) override {}
    void process_block(std::span<float* const> /*channel_data*/, int /*num_samples*/) override
    {
        // Process all channels separately.
    }
    void release_resources() override {}
};

std::unique_ptr<SelfModEngine> make_selfmod_engine()
{
    return std::make_unique<SelfModEngineImpl>();
}
