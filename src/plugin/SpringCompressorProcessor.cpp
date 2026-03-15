#include "SpringCompressorProcessor.h"

#include "SpringCompressorEditor.h"
#include "TransferCurveComponent.h"
#include "engine.h"
#include "util.h"

#include <meadow/cppext.h>
#include <meadow/math.h>
#include <meadow/matlab.h>

#include <cassert>
#include <span>

namespace
{
constexpr int k_ui_refresh_timer_ms = 33;
constexpr int k_rms_matrix_size = TransferCurveComponent::k_db_max - TransferCurveComponent::k_db_min + 1;
constexpr double k_max_rms_dot_age_sec = 3;
const int k_max_rms_dot_age_ticks = iround<int>(k_max_rms_dot_age_sec / k_rms_sample_period_sec);
constexpr bool k_play_test_signal = false;
const std::filesystem::path k_test_signal_path =
#ifdef NDEBUG
  ""
#else
  PROJECT_SOURCE_DIR "/matlab/test_signal_sine_1k_amp_60_0_db.wav"
#endif
  ;
} // namespace

SpringCompressorProcessor::SpringCompressorProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , engine(make_engine())
    , raw_parameter_values{
        .threshold_db = apvts.getRawParameterValue("threshold"),
        .ratio = apvts.getRawParameterValue("ratio"),
        .makeup_gain_db = apvts.getRawParameterValue("makeup"),
        .reference_level_db = apvts.getRawParameterValue("reference_level"),
        .knee_width_db = apvts.getRawParameterValue("knee_width")
    }
    , rms_matrix(square(k_rms_matrix_size), INT_MIN)
    , rms_matrix_as_mdspan(rms_matrix.data(), k_rms_matrix_size, k_rms_matrix_size)
    , ui_refresh_timer([this]() {
        on_ui_refresh_timer_elapsed();
    })
{
    assert(cmp_equal(
      &(rms_matrix_as_mdspan[k_rms_matrix_size - 1, k_rms_matrix_size - 1]) - rms_matrix.data() + 1, rms_matrix.size()
    ));

    for (auto* id : {"threshold",
                     "ratio",
                     "makeup",
                     "reference_level",
                     "knee_width",
                     "level_method",
                     "levellpf_mode",
                     "levellpf_order",
                     "levellpf_attack",
                     "levellpf_release",
                     "levelmb_freqlo",
                     "levelmb_freqhi",
                     "levelmb_peroctave",
                     "levelmb_order",
                     "levelmb_lporder",
                     "levelmb_lpratio",
                     "levelmb_minrelease",
                     "grlp_enable",
                     "grlp_order",
                     "grlp_attack",
                     "grlp_release"})
        apvts.addParameterListener(id, this);
    ui_refresh_timer.startTimer(k_ui_refresh_timer_ms);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpringCompressorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"threshold", 1},
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ratio", 1},
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
        4.0f,
        juce::AudioParameterFloatAttributes{}.withLabel(":1")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"makeup", 1},
        "Makeup Gain",
        juce::NormalisableRange<float>(-32.0f, 32.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reference_level", 1},
        "Reference Level",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"knee_width", 1},
        "Knee width",
        juce::NormalisableRange<float>(0.0f, 40.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")
      )
    );

    auto make_choice = [&](const char* id, juce::StringArray choices) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, id, choices, 0));
    };
    auto make_skewed_float = [&](const char* id, float min, float max, float step, float centre, float def) {
        auto r = juce::NormalisableRange<float>(min, max, step);
        r.setSkewForCentre(centre);
        params.push_back(
          std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, id, r, def, juce::AudioParameterFloatAttributes{}
          )
        );
    };

    make_choice("level_method", {"direct", "lpf", "mb"});

    make_choice("levellpf_mode", {"amp", "rms"});
    make_choice("levellpf_order", {"1", "2"});
    make_skewed_float("levellpf_attack", 0.f, 50.f, 0.1f, 10.f, 10.f);
    make_skewed_float("levellpf_release", 0.f, 2000.f, 1.f, 100.f, 100.f);

    make_skewed_float("levelmb_freqlo", 20.f, 1000.f, 1.f, 100.f, 100.f);
    make_skewed_float("levelmb_freqhi", 1000.f, 12000.f, 1.f, 3000.f, 3000.f);
    make_skewed_float("levelmb_peroctave", 0.5f, 6.f, 0.1f, 2.f, 2.f);
    make_choice("levelmb_order", {"1", "2"});
    make_choice("levelmb_lporder", {"1", "2"});
    make_skewed_float("levelmb_lpratio", 0.1f, 10.f, 0.01f, 2.f, 2.f);
    make_skewed_float("levelmb_minrelease", 0.f, 50.f, 0.1f, 10.f, 10.f);

    make_choice("grlp_enable", {"off", "mag", "pow", "db"});
    make_choice("grlp_order", {"1", "2"});
    make_skewed_float("grlp_attack", 0.f, 50.f, 0.1f, 10.f, 10.f);
    make_skewed_float("grlp_release", 0.f, 2000.f, 1.f, 100.f, 100.f);

    return {params.begin(), params.end()};
}

void SpringCompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (k_play_test_signal) {
        test_signal = load_audio_file_first_channel(k_test_signal_path).value().first;
    }
    engine->prepare_to_play(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    audio_thread_running = true;
}

void SpringCompressorProcessor::releaseResources()
{
    audio_thread_running = false;
    engine->release_resources();
}

void SpringCompressorProcessor::sync_engine_processor(bool called_from_audio_thread)
{
    Engine::SetParsResult r = Engine::SetParsResult::transfer_curve_didnt_change;

    if (parameter_changed_was_called.exchange(false)) {
        // TODO: query all the parameters and fill in EnginePars.
        r = engine->set_pars(
          EnginePars{
            .transfer_curve = TransferCurvePars{
                                                .threshold_db = raw_parameter_values.threshold_db->load(),
                                                .ratio = raw_parameter_values.ratio->load(),
                                                .knee_width_db = raw_parameter_values.knee_width_db->load(),
                                                .makeup_gain_db = raw_parameter_values.makeup_gain_db->load(),
                                                .reference_level_db = raw_parameter_values.reference_level_db->load()
            }
        }
        );
    }

    if (call_editor_set_transfer_curve_anyway.exchange(false)) {
        r = Engine::SetParsResult::transfer_curve_changed;
    }

    switch (r) {
    case Engine::SetParsResult::transfer_curve_didnt_change:
        break;
    case Engine::SetParsResult::transfer_curve_changed: {
        const auto tcs = engine->get_transfer_curve_state();
        if (called_from_audio_thread) {
            audio_to_ui_queue.enqueue(tcs);
        } else {
            editor_set_transfer_curve(tcs);
        }
    }
    }
}

void SpringCompressorProcessor::parameterChanged(UNUSED const juce::String& name, UNUSED float value)
{
    if (ignore_parameter_changed) {
        return;
    }

    parameter_changed_was_called = true;
    if (!audio_thread_running) {
        // Handle it now.
        sync_engine_processor(false);
    }
}

void SpringCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // In isBusesLayoutSupported we only enabled layouts which satisfy this.
    assert(getMainBusNumInputChannels() == getMainBusNumOutputChannels());
    // We don't have any sidechains for now.
    assert(getMainBusNumInputChannels() == getTotalNumInputChannels());
    assert(getMainBusNumOutputChannels() == getTotalNumOutputChannels());

    sync_engine_processor(true);

    auto input_buffer = getBusBuffer(buffer, true, 0);
    auto output_buffer = getBusBuffer(buffer, false, 0);
    auto num_channels = sucast(input_buffer.getNumChannels());
    assert(std::cmp_equal(output_buffer.getNumChannels(), num_channels));
    // In practice, these will be the same pointers.
    auto read_pointers = std::span(input_buffer.getArrayOfReadPointers(), num_channels);
    auto write_pointers = std::span(output_buffer.getArrayOfWritePointers(), num_channels);
    assert(std::ranges::equal(read_pointers, write_pointers));

    if (k_play_test_signal && !test_signal.empty()) {
        for (int i = 0; i < buffer.getNumSamples();
             ++i, test_signal_playhead = (test_signal_playhead + 1) % test_signal.size()) {
            const auto input_sample = test_signal[test_signal_playhead];
            for (int c = 0; c < uscast(num_channels); ++c) {
                output_buffer.setSample(c, i, input_sample);
            }
        }
    }

    engine->process_block(write_pointers, buffer.getNumSamples());

    if (editor_open) {
        AudioToUIMsg::RmsSamples msg;
        for (auto& s : engine->get_rms_samples_of_last_block()) {
            msg.samples.push_back(s);
            if (msg.samples.size() == decltype(msg.samples)::capacity()) {
                audio_to_ui_queue.enqueue(msg);
                msg.samples.clear();
            }
        }
        if (!msg.samples.empty()) {
            audio_to_ui_queue.enqueue(msg);
        }
    }
}

juce::AudioProcessorEditor* SpringCompressorProcessor::createEditor()
{
    editor_open = true;
    auto* editor = new SpringCompressorEditor(*this, editor_open, apvts);
    // We need to set the transfer curve on the editor but can't query the engine directly if the audio thread is
    // running.
    if (audio_thread_running) {
        call_editor_set_transfer_curve_anyway = true;
    } else {
        editor_set_transfer_curve(engine->get_transfer_curve_state());
    }
    return editor;
}

void SpringCompressorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpringCompressorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void SpringCompressorProcessor::editor_set_transfer_curve(const TransferCurveState& tcs)
{
    if (auto* editor = this->getActiveEditor()) {
        dynamic_cast<SpringCompressorEditor*>(editor)->set_transfer_curve(tcs);
        editor->repaint();
    }
}

void SpringCompressorProcessor::on_ui_refresh_timer_elapsed()
{
    AudioToUIMsg::V msg;
    bool rms_matrix_updated = false;
    while (audio_to_ui_queue.try_dequeue(msg)) {
        switch (enum_of(msg)) {
            EVARIANT_CASE2(msg, AudioToUIMsg, TransferCurveState, tcs)
            {
                editor_set_transfer_curve(tcs);
            }
            break;
            EVARIANT_CASE2(msg, AudioToUIMsg, RmsSamples, rms_samples)
            {
                rms_matrix_updated |= !rms_samples.samples.empty();
                for (auto rs : rms_samples.samples) {
                    const auto input_db = matlab::mag2db(rs[0]);
                    const auto output_db = matlab::mag2db(rs[1]);
                    if (in_cc_range(
                          input_db, TransferCurveComponent::k_db_min - 1, TransferCurveComponent::k_db_max + 1
                        )
                        && in_cc_range(
                          output_db, TransferCurveComponent::k_db_min - 1, TransferCurveComponent::k_db_max + 1
                        )) {
                        const auto db_to_index = [](float db) {
                            constexpr int M = 2;
                            const int db_int = iround<int>(db);
                            return db_int - modulo(db_int, M) - TransferCurveComponent::k_db_min;
                        };
                        const auto xy = AI2{db_to_index(input_db), db_to_index(output_db)};
                        if (in_co_range(xy[0], 0, k_rms_matrix_size) && in_co_range(xy[1], 0, k_rms_matrix_size)) {
                            rms_matrix_as_mdspan[xy[0], xy[1]] = rms_matrix_clock;
                        }
                    }
                    ++rms_matrix_clock;
                }
                // Maintenance: periodically decrease all timestamps and the clock to prevent overflow.
                if (rms_matrix_clock >= k_max_rms_dot_age_ticks) {
                    for (auto& x : rms_matrix) {
                        if (x < 0) {
                            x = INT_MIN;
                        } else {
                            x -= rms_matrix_clock;
                        }
                    }
                    rms_matrix_clock = 0;
                }
            }
            break;
        }
    }
    if (rms_matrix_updated) {
        if (auto* editor = this->getActiveEditor()) {
            dynamic_cast<SpringCompressorEditor*>(editor)->update_rms_dots(
              rms_matrix_clock, rms_matrix_as_mdspan, k_rms_sample_period_sec
            );
        }
    }
    if (!audio_thread_running) {
        // We're calling sync here to make sure we don't miss anything because the audio thread is stopped after a
        // paramChange call but before it could call sync_engine_processor.
        sync_engine_processor(false);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpringCompressorProcessor();
}
