#include "engine.h"

struct EngineImpl : public Engine {
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
