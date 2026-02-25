#pragma once

#include <memory>

class Engine
{
public:
    virtual ~Engine() = default;
};

std::unique_ptr<Engine> make_engine();
