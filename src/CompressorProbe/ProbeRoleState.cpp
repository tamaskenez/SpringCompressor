#include "ProbeRoleState.h"

#include "pipes.h"

AnalyzerWorkspace::DecibelCycle::DecibelCycle(const Mode::DecibelCycle& decibel_cycle_params, double sample_rate)
    : decibel_cycle_loop_generator(sample_rate)
{
    // Update the decibel cycle loop generator's parameter dependent data, so we can use cycle_length_samples later.
    decibel_cycle_loop_generator.generate_block(decibel_cycle_params, 0, span<float>());
}

AnalyzerWorkspace::RatioByFreq::RatioByFreq(const Mode::RatioByFreq& params, double sample_rate)
    : g(sample_rate)
{
    g.init(params);
    input_output_dbs.assign(g.num_periods, AD2{NAN, NAN});
}

ProbeRoleState::ProbeRoleState() = default;

ProbeRoleState::~ProbeRoleState() = default;
