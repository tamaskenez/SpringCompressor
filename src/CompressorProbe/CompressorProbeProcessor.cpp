#include "CompressorProbeProcessor.h"

#include "CompressorProbeEditor.h"
#include "ParameterID.h"
#include "config.h"
#include "juce_util/logging.h"
#include "juce_util/misc.h"
#include "meadow/math.h"
#include "meadow/matlab.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/enum.h>

namespace
{
constexpr int k_ui_refresh_timer_ms = 33;
} // namespace

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessorWithDefaults(
        BusesProperties()
          .withInput("Input", k_default_buses_layout, true)
          .withOutput("Output", k_default_buses_layout, true),
        AudioProcessorProperties{.name = JucePlugin_Name, .has_editor = true}
      )
    , ts_state(this)
    , mt_state(
        *this,
        [this] {
            return dynamic_cast<CompressorProbeEditor*>(getActiveEditor());
        }
      )
    , ui_refresh_timer([this]() {
        on_ui_refresh_timer_elapsed_mt();
    })
{
    ui_refresh_timer.startTimer(k_ui_refresh_timer_ms);
    for (const auto& id : get_enum_names_in_StringArray<ParameterID>()) {
        mt_state.apvts.addParameterListener(id, this);
    }

#if 0
    // TESTING EnvelopeFilter
    double fs = 48000;
    EnvelopeFilterLoopGenerator g(fs);
    Mode::EnvelopeFilter p{
      .carrier_freq = 10000,
      .max_carrier_amp_dbfs = -3,
      .min_mod_freq = 100,
      .max_mod_freq = 1000,
      .mod_amp_db = 6,
      .cycle_length_index = 1
    };
    g.init(p);
    FILE* f = fopen("/tmp/a.m", "wt");
    println(f, "A = [");
    for (unsigned i = 0; i < g.cycle_length_samples; ++i) {
        println(f, "{}", g.freq_at_sample(i));
    }
    println(f, "];");
    fclose(f);

    vector<float> x(g.cycle_length_samples);
    g.generate_block(p, 0, x);

    f = fopen("/tmp/b.m", "wt");
    println(f, "B = [");
    for (auto xi:x){
        println(f, "{}", xi);
    }
    println(f, "];");
    fclose(f);
    NOP;
#endif
}

CompressorProbeProcessor::~CompressorProbeProcessor()
{
    auto lock = std::scoped_lock(*this_lifetime_token_mutex);
    for (const auto& id : get_enum_names_in_StringArray<ParameterID>()) {
        mt_state.apvts.removeParameterListener(id, this);
    }
    this_lifetime_token.reset();
}

// ==== Entry points called on message thread. ====
juce::AudioProcessorEditor* CompressorProbeProcessor::createEditor()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto* p = new CompressorProbeEditor(*this, ts_state, mt_state, this);
    mt_state.update_channels_on_editor(ts_state.get_role(), ts_state.num_channels.load(), p);

    if (is_development_mode() && !development_mode.generator) {
        development_mode.generator = make_unique<GeneratorRole>(*this);
        call_async_on_mt([this] {
            on_role_selected_by_user_mt(Role::Probe);
        });
    }

    return p;
}

void CompressorProbeProcessor::on_ui_refresh_timer_elapsed_mt()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = ts_state.file_log_sink->activate();
    if (auto* p = ts_state.role_impl.load()) {
        p->on_ui_refresh_timer_elapsed_mt();
    }
    if (is_development_mode() && development_mode.generator) {
        development_mode.generator->on_ui_refresh_timer_elapsed_mt();
    }
    mt_state.on_ui_refresh_timer_elapsed();
}

void CompressorProbeProcessor::on_role_selected_by_user_mt(Role new_role)
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = ts_state.file_log_sink->activate();

    LOG(INFO) << format("on_role_selected_by_user({})", magic_enum::enum_name(new_role));

    auto lock = std::scoped_lock(mutex);

    CHECK(!ts_state.role_impl.load());

    switch (new_role) {
    case Role::Generator:
        role_impl_storage = make_unique<GeneratorRole>(*this);
        break;
    case Role::Probe:
        role_impl_storage = make_unique<ProbeRole>(*this);
        break;
    }

    CHECK(common_state.prepared_to_play.has_value() == ts_state.prepared_to_play.load());

    if (common_state.prepared_to_play) {
        role_impl_storage->prepare_to_play(
          common_state.prepared_to_play->sample_rate, common_state.prepared_to_play->samples_per_block
        );
        if (is_development_mode() && development_mode.generator) {
            development_mode.generator->prepare_to_play(
              common_state.prepared_to_play->sample_rate, common_state.prepared_to_play->samples_per_block
            );
        }
    }

    // The following line signals to the potentially simultaneous processBlock calls that the current role is active,
    // prepared to play and ready for its process_block to be called.
    ts_state.role_impl.store(role_impl_storage.get());

    mt_state.update_channels_on_editor(ts_state.get_role(), ts_state.num_channels.load());
    if (auto* e = mt_state.get_active_editor_fn()) {
        e->refresh_ui();
    }
}

// ==== Entry points called on any (non-audio) thread. ====

void CompressorProbeProcessor::prepareToPlay(double sample_rate, int samples_per_block)
{
    auto a = ts_state.file_log_sink->activate();

    auto lock = std::scoped_lock(mutex);

    ts_state.generator_id_in_probe = k_invalid_generator_id;
    CHECK(!common_state.prepared_to_play);
    common_state.prepared_to_play = {sample_rate, samples_per_block};
    call_async_on_mt([this, sample_rate]() {
        mt_state.sample_rate = sample_rate;
    });
    if (auto* p = ts_state.role_impl.load()) {
        p->prepare_to_play(sample_rate, samples_per_block);
    }
    if (is_development_mode() && development_mode.generator) {
        development_mode.generator->prepare_to_play(sample_rate, samples_per_block);
    }
    ts_state.prepared_to_play = true;
    at.block_sample_index = 0;
}

void CompressorProbeProcessor::releaseResources()
{
    auto a = ts_state.file_log_sink->activate();

    auto lock = std::scoped_lock(mutex);

    CHECK(common_state.prepared_to_play.has_value() == ts_state.prepared_to_play.load());
    if (!common_state.prepared_to_play) {
        LOG(WARNING) << "CompressorProbeProcessor::releaseResources() called without prepareToPlay()";
        return;
    }

    if (auto* p = ts_state.role_impl.load()) {
        p->release_resources();
    }
    if (is_development_mode() && development_mode.generator) {
        development_mode.generator->release_resources();
    }
    common_state.prepared_to_play.reset();
    call_async_on_mt([this]() {
        mt_state.sample_rate.reset();
    });
    ts_state.prepared_to_play = false;
}

void CompressorProbeProcessor::numChannelsChanged()
{
    auto lock = std::scoped_lock(mutex);

    const auto ni = getTotalNumInputChannels();
    const auto no = getTotalNumOutputChannels();
    CHECK(ni == no);
    ts_state.num_channels = no;
    call_async_on_mt([this] {
        mt_state.update_channels_on_editor(ts_state.get_role(), ts_state.num_channels.load());
    });
}

void CompressorProbeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = mt_state.apvts.copyState().createXml()) { // copyState is thread-safe.
        copyXmlToBinary(*xml, destData);
    }
}

void CompressorProbeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        mt_state.apvts.replaceState(juce::ValueTree::fromXml(*xml)); // replaceState is thread-safe.
    }
}

bool CompressorProbeProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto in = layouts.getMainInputChannelSet();
    auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

// ==== Function called on the audio thread. ====

void CompressorProbeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /* midi_messages*/)
{
    juce::ScopedNoDenormals no_denormals;

    if (is_development_mode() && development_mode.generator) {
        development_mode.generator->process_block(at.block_sample_index, buffer);
        const auto* rp = buffer.getReadPointer(0);
        float max_abs = 0.0f;
        double sum_x2 = 0.0;
        for (int j = 0; j < buffer.getNumSamples(); ++j) {
            max_abs = std::max(max_abs, std::abs(rp[j]));
            sum_x2 += square(rp[j]);
        }
        UNUSED const auto peak_db = ffcast<double>(matlab::mag2db(max_abs));
        UNUSED const auto rms_db = matlab::pow2db(sum_x2 / buffer.getNumSamples());
        const double threshold = -32;
        const double ratio = 4.0;
        const auto in = rms_db;
        double gr = NAN;
        if (in <= threshold) {
            gr = 1.0;
        } else {
            const auto out = (in - threshold) / ratio + threshold;
            gr = matlab::db2mag(out - in);
        }
        buffer.applyGain(ffcast<float>(gr));
    }

    if (auto* p = ts_state.role_impl.load()) {
        p->process_block(at.block_sample_index, buffer);
    } else {
        buffer.clear();
    }

    at.block_sample_index += buffer.getNumSamples();
}

// ==== Functions called on any thread, including audio. ====
void CompressorProbeProcessor::parameterChanged(const juce::String& parameter_id, UNUSED float new_value)
{
    if (ts_state.get_role() != Role::Probe) {
        return;
    }
    const auto e = magic_enum::enum_cast<ParameterID>(parameter_id.toRawUTF8());
    LOG_IF(FATAL, !e) << format("Unknown parameter_id: {}", parameter_id.toRawUTF8());
    switch (*e) {
    case ParameterID::mode:
    case ParameterID::steady_curve_freq:
    case ParameterID::steady_curve_waveform:
    case ParameterID::steady_curve_level_ref:
    case ParameterID::steady_curve_min_dbfs:
    case ParameterID::steady_curve_max_dbfs:
    case ParameterID::steady_curve_length:
        call_async_on_mt([this] {
            auto* p = dynamic_cast<ProbeRole*>(ts_state.role_impl.load());
            CHECK(p);
            p->on_mode_changed_mt();
        });
        break;
    case ParameterID::channels:
    case ParameterID::y_unit:
    case ParameterID::auto_scale:
        break;
    }
}

// ==== Static factory function. ====

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProbeProcessor();
}
