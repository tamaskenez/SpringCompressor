#include "CompressorProbeProcessor.h"

#include "CompressorProbeEditor.h"
#include "juce_util/logging.h"

#include <magic_enum/magic_enum.hpp>

namespace
{
constexpr int k_ui_refresh_timer_ms = 33;
}

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , file_log_sink(make_unique<FileLogSink>(this, JucePlugin_Manufacturer, format("{}-", JucePlugin_Name)))
    , ui_refresh_timer([this]() {
        on_ui_refresh_timer_elapsed();
    })

{
    common_state.file_log_sink = file_log_sink.get();
    ui_refresh_timer.startTimer(k_ui_refresh_timer_ms);
}

CompressorProbeProcessor::~CompressorProbeProcessor() = default;

void CompressorProbeProcessor::on_ui_refresh_timer_elapsed()
{
    auto a = file_log_sink->activate();
    if (auto e = get_active_editor()) {
        e->refresh_ui();
    }
}

void CompressorProbeProcessor::prepareToPlay(double sample_rate, int samples_per_block)
{
    auto a = file_log_sink->activate();

    common_state.generator_id.reset();
    if (common_state.role) {
        CHECK(role_impl);
        role_impl->prepare_to_play(sample_rate, samples_per_block);
        role_impl_for_audio_thread = role_impl.get();
    }
    CHECK(!common_state.prepared_to_play);
    common_state.prepared_to_play = {sample_rate, samples_per_block};
}

void CompressorProbeProcessor::releaseResources()
{
    auto a = file_log_sink->activate();

    if (!common_state.prepared_to_play) {
        LOG(WARNING) << "CompressorProbeProcessor::releaseResources() called without prepareToPlay()";
    }

    if (common_state.role) {
        CHECK(role_impl);
        role_impl->release_resources();
    }
    common_state.prepared_to_play.reset();
    role_impl_for_audio_thread = nullptr;
}

void CompressorProbeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages)
{
    juce::ScopedNoDenormals no_denormals;

    if (auto* p = role_impl_for_audio_thread.load()) {
        p->process_block(buffer, midi_messages);
    } else {
        buffer.clear();
    }
}

juce::AudioProcessorEditor* CompressorProbeProcessor::createEditor()
{
    return new CompressorProbeEditor(*this, this, common_state);
}

void CompressorProbeProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}

void CompressorProbeProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/) {}

void CompressorProbeProcessor::on_role_selected_by_user(Role new_role)
{
    auto a = file_log_sink->activate();

    LOG(INFO) << format("on_role_selected_by_user({})", magic_enum::enum_name(new_role));
    CHECK(!common_state.role);
    CHECK(!role_impl);
    common_state.role = new_role;
    switch (new_role) {
    case Role::Generator:
        role_impl = make_unique<GeneratorRole>(common_state);
        break;
    case Role::Probe:
        role_impl = make_unique<ProbeRole>(common_state);
        break;
    }
    if (common_state.prepared_to_play) {
        role_impl->prepare_to_play(
          common_state.prepared_to_play->sample_rate, common_state.prepared_to_play->samples_per_block
        );
        role_impl_for_audio_thread = role_impl.get();
    }
    if (auto e = get_active_editor()) {
        e->refresh_ui();
    }
}

std::pair<GeneratorStatus, std::optional<std::string>> CompressorProbeProcessor::get_generator_status() const
{
    CHECK(common_state.role == Role::Generator);
    auto* p = dynamic_cast<const GeneratorRole*>(role_impl.get());
    CHECK(p);
    return {p->get_status(), p->get_current_command_to_string()};
}

CompressorProbeEditor* CompressorProbeProcessor::get_active_editor() const
{
    return dynamic_cast<CompressorProbeEditor*>(this->getActiveEditor());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProbeProcessor();
}
