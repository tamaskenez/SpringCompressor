#include "generate_scope_data.h"

#include <gtest/gtest.h>

TEST(generate_scope_data, T1)
{
    EnginePars pars;
    pars.input_level_method = EnginePars::InputLevelMethod::multiband;
    UNUSED auto r = generate_scope_data(pars);
    NOP;
}
