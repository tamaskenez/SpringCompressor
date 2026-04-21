#include "CompressorProbeThreadSafeState.h"

#include "RoleInterface.h"
#include "juce_util/logging.h"

CompressorProbeThreadSafeState::CompressorProbeThreadSafeState(juce::AudioProcessor* instance)
    : file_log_sink(make_unique<FileLogSink>(instance, JucePlugin_Manufacturer, format("{}-", JucePlugin_Name)))
{
}

CompressorProbeThreadSafeState::~CompressorProbeThreadSafeState() = default;

optional<Role> CompressorProbeThreadSafeState::get_role() const
{
    if (auto* p = role_impl.load()) {
        return p->get_role();
    }
    return nullopt;
}
