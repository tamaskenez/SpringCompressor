#include "CompressorProbeProcessor.h"

#include "CompressorProbeEditor.h"
#include "juce_util/logging.h"
#include "meadow/enum.h"

#include <magic_enum/magic_enum.hpp>

namespace
{
constexpr int k_ui_refresh_timer_ms = 33;

enum class ParameterID {
    mode,
    steady_curve_freq,
    steady_curve_waveform,
    steady_curve_level_method,
    steady_curve_min_dbfs,
    steady_curve_max_dbfs,
    steady_curve_length,
    channels,
    y_unit,
    auto_scale,
};

template<class E>
juce::StringArray get_enum_names_in_StringArray()
{
    juce::StringArray labels;
    for (auto [_, n] : magic_enum::enum_entries<E>()) {
        labels.add(juce::String(n.data(), n.size()));
    }
    return labels;
}

template<class E>
juce::StringArray get_labels_for_enum_in_StringArray()
{
    juce::StringArray labels;
    for (auto [e, _] : magic_enum::enum_entries<E>()) {
        auto sv = get_label_for_enum(e);
        labels.add(juce::String(sv.data(), sv.size()));
    }
    return labels;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    return {
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode", get_labels_for_enum_in_StringArray<Mode::E>(), 0
      ),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_freq", 1}, "Freq", 20, 10000, 1000),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_waveform", 1},
        "Waveform",
        juce::StringArray{"sine", "square", "hi-crest-square"},
        0
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_level_method", 1}, "Level Method", juce::StringArray{"peak", "rms"}, 0
      ),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_min_dbfs", 1}, "Min dBFS", -60, 0, -60),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_max_dbfs", 1}, "Max dBFS", -50, 0, -6),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_length", 1},
        "Length",
        juce::StringArray{"500 ms", "1000 ms", "2000 ms", "4000 ms", "8000 ms"},
        2
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"channels", 1}, "Channels", juce::StringArray{"left", "right", "(left+right)/2"}, 0
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"y_unit", 1}, "Y Unit", juce::StringArray{"linear", "dB"}, 0
      ),
      make_unique<juce::AudioParameterBool>(juce::ParameterID{"auto_scale", 1}, "Auto Scale", true),
    };
}
} // namespace

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , file_log_sink(make_unique<FileLogSink>(this, JucePlugin_Manufacturer, format("{}-", JucePlugin_Name)))
    , ui_refresh_timer([this]() {
        on_ui_refresh_timer_elapsed();
    })

{
    common_state.file_log_sink = file_log_sink.get();
    ui_refresh_timer.startTimer(k_ui_refresh_timer_ms);
    for (const auto& id : get_enum_names_in_StringArray<ParameterID>()) {
        apvts.addParameterListener(id, this);
    }
}

CompressorProbeProcessor::~CompressorProbeProcessor()
{
    for (const auto& id : get_enum_names_in_StringArray<ParameterID>()) {
        apvts.removeParameterListener(id, this);
    }
}

void CompressorProbeProcessor::on_ui_refresh_timer_elapsed()
{
    auto a = file_log_sink->activate();
    if (auto e = get_active_editor()) {
        e->refresh_ui();
    }
    if (role_impl) {
        role_impl->on_ui_refresh_timer_elapsed();
    }
}

void CompressorProbeProcessor::prepareToPlay(double sample_rate, int samples_per_block)
{
    const auto num_channels = getTotalNumInputChannels();
    CHECK(getTotalNumOutputChannels() == num_channels);

    auto a = file_log_sink->activate();

    common_state.generator_id.reset();
    CHECK(!common_state.prepared_to_play);
    common_state.prepared_to_play = {sample_rate, samples_per_block, num_channels};
    if (common_state.role) {
        CHECK(role_impl);
        role_impl->prepare_to_play(sample_rate, samples_per_block);
        role_impl_for_audio_thread = role_impl.get();
    }
    common_state.next_process_block_index = 0;
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
    ++common_state.next_process_block_index;
}

juce::AudioProcessorEditor* CompressorProbeProcessor::createEditor()
{
    return new CompressorProbeEditor(*this, apvts, this, common_state);
}

void CompressorProbeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) {
        copyXmlToBinary(*xml, destData);
    }
}

void CompressorProbeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }
}

void CompressorProbeProcessor::parameterChanged(const juce::String& parameter_id, float new_value)
{
    if (common_state.role != Role::Probe) {
        return;
    }
    const auto e = magic_enum::enum_cast<ParameterID>(parameter_id.toRawUTF8());
    LOG_IF(FATAL, !e) << format("Unknown parameter_id: {}", parameter_id.toRawUTF8());
    switch (*e) {
    case ParameterID::mode: {
        const auto mode_enum = enum_cast_from_float<Mode::E>(new_value);
        LOG_IF(FATAL, !mode_enum) << format("new_value is not valid as Mode::E: {}", new_value);
        on_mode_changed(*mode_enum);
    } break;
    case ParameterID::steady_curve_freq:
    case ParameterID::steady_curve_waveform:
    case ParameterID::steady_curve_level_method:
    case ParameterID::steady_curve_min_dbfs:
    case ParameterID::steady_curve_max_dbfs:
    case ParameterID::steady_curve_length:
    case ParameterID::channels:
    case ParameterID::y_unit:
    case ParameterID::auto_scale:
        break;
    }
}

void CompressorProbeProcessor::on_role_selected_by_user(Role new_role)
{
    auto a = file_log_sink->activate();

    LOG(INFO) << format("on_role_selected_by_user({})", magic_enum::enum_name(new_role));
    CHECK(!common_state.role);
    CHECK(!role_impl);
    common_state.role = new_role;
    switch (new_role) {
    case Role::Generator:
        role_impl = make_unique<GeneratorRole>(*this);
        break;
    case Role::Probe:
        role_impl = make_unique<ProbeRole>(*this);
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

void CompressorProbeProcessor::on_mode_changed(Mode::E mode)
{
    auto* p = dynamic_cast<ProbeRole*>(role_impl.get());
    CHECK(p);
    p->on_mode_changed(mode);
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

const ProbeRoleState* CompressorProbeProcessor::get_probe_state() const
{
    CHECK(common_state.role == Role::Probe);
    return dynamic_cast<const ProbeRole*>(role_impl.get())->get_state();
}

Mode::E CompressorProbeProcessor::get_mode() const
{
    auto* rap = apvts.getParameter("mode");
    auto e = enum_cast_from_float<Mode::E>(rap->convertFrom0to1(rap->getValue()));
    CHECK(e);
    return *e;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProbeProcessor();
}
