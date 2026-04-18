#include "SpringCompressorProcessor.h"

#include "SpringCompressorEditor.h"
#include "TransferCurveComponent.h"
#include "engine.h"
#include "generate_scope_data.h"
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
        .knee_width_db = apvts.getRawParameterValue("knee_width"),
        .level_method = apvts.getRawParameterValue("level_method"),
        .levellpf_mode = apvts.getRawParameterValue("levellpf_mode"),
        .levellpf_order = apvts.getRawParameterValue("levellpf_order"),
        .levellpf_attack = apvts.getRawParameterValue("levellpf_attack"),
        .levellpf_release = apvts.getRawParameterValue("levellpf_release"),
        .levelmb_freqlo = apvts.getRawParameterValue("levelmb_freqlo"),
        .levelmb_freqhi = apvts.getRawParameterValue("levelmb_freqhi"),
        .levelmb_peroctave = apvts.getRawParameterValue("levelmb_peroctave"),
        .levelmb_order = apvts.getRawParameterValue("levelmb_order"),
        .levelmb_lporder = apvts.getRawParameterValue("levelmb_lporder"),
        .levelmb_lpratio = apvts.getRawParameterValue("levelmb_lpratio"),
        .levelmb_minrelease = apvts.getRawParameterValue("levelmb_minrelease"),
        .grlp_enable = apvts.getRawParameterValue("grlp_enable"),
        .grlp_order = apvts.getRawParameterValue("grlp_order"),
        .grlp_attack = apvts.getRawParameterValue("grlp_attack"),
        .grlp_release = apvts.getRawParameterValue("grlp_release"),
        .scope_mode = apvts.getRawParameterValue("scope_mode"),
        .scope_freq = apvts.getRawParameterValue("scope_freq"),
        .mod_enable = apvts.getRawParameterValue("mod_enable"),
        .mod_lpf_order = apvts.getRawParameterValue("mod_lpf_order"),
        .mod_lpf_freq = apvts.getRawParameterValue("mod_lpf_freq"),
        .mod_gain = apvts.getRawParameterValue("mod_gain"),
        .mod_tanh = apvts.getRawParameterValue("mod_tanh")
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
                     "grlp_release",
                     "scope_mode",
                     "scope_freq",
                     "mod_enable",
                     "mod_lpf_order",
                     "mod_lpf_freq",
                     "mod_gain",
                     "mod_tanh"})
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
    make_skewed_float("levellpf_release", 1.f, 2000.f, 1.f, 500.f, 100.f);

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
    make_skewed_float("grlp_release", 1.0f, 2000.f, 1.f, 500.f, 100.f);

    make_choice("scope_mode", {"transfer", "attack", "release", "hdist", "inhdist", "threshold"});
    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"scope_freq", 1}, "scope_freq", 0.0f, 1.0f, 0.0f)
    );

    make_choice("mod_enable", {"off", "on"});
    make_choice("mod_lpf_order", {"1", "2"});
    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mod_lpf_freq", 1},
        "mod_lpf_freq",
        juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f),
        1000.0f
      )
    );
    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mod_gain", 1}, "mod_gain", juce::NormalisableRange<float>(-3.0f, 3.0f, 0.01f), 0.0f
      )
    );
    make_skewed_float("mod_tanh", 0.1f, 10.f, 0.01f, 1.f, 1.f);

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

EnginePars SpringCompressorProcessor::query_raw_parameter_values_into_EnginePars()
{
    const auto& p = raw_parameter_values;
    auto ms_to_sec = [](float ms) {
        return ms / 1000.0f;
    };
    auto attack_sec = [&](std::atomic<float>* a) -> optional<float> {
        const float ms = a->load();
        return ms > 0.0f ? optional<float>{ms_to_sec(ms)} : nullopt;
    };
    auto order = [](std::atomic<float>* o) {
        return iround<int>(o->load()) + 1;
    };
    return EnginePars{
      .input_level_method = static_cast<EnginePars::InputLevelMethod>(iround<int>(p.level_method->load())),
      .input_level_lpf =
        {
                          .rms = iround<int>(p.levellpf_mode->load()) == 1,
                          .order = order(p.levellpf_order),
                          .attack_time_sec = attack_sec(p.levellpf_attack),
                          .release_time_sec = ms_to_sec(p.levellpf_release->load()),
                          },
      .input_level_multiband =
        {
                          .freq_lo_hz = p.levelmb_freqlo->load(),
                          .freq_hi_hz = p.levelmb_freqhi->load(),
                          .crossovers_per_octave = p.levelmb_peroctave->load(),
                          .crossover_order = order(p.levelmb_order),
                          .lp_order = order(p.levelmb_lporder),
                          .lp_ratio = p.levelmb_lpratio->load(),
                          .min_release_time_sec = ms_to_sec(p.levelmb_minrelease->load()),
                          },
      .transfer_curve =
        {
                          .threshold_db = p.threshold_db->load(),
                          .ratio = p.ratio->load(),
                          .knee_width_db = p.knee_width_db->load(),
                          .makeup_gain_db = p.makeup_gain_db->load(),
                          .reference_level_db = p.reference_level_db->load(),
                          },
      .gr_filter_mode = static_cast<EnginePars::GRFilterMode>(iround<int>(p.grlp_enable->load())),
      .gr_filter =
        {
                          .order = order(p.grlp_order),
                          .attack_time_sec = attack_sec(p.grlp_attack),
                          .release_time_sec = ms_to_sec(p.grlp_release->load()),
                          },
      .mod = {
                          .enable = iround<int>(p.mod_enable->load()) == 1,
                          .tanh_k = p.mod_tanh->load(),
                          .gain = p.mod_gain->load(),
                          .lpf_order = order(p.mod_lpf_order),
                          .lpf_freq_hz = p.mod_lpf_freq->load(),
                          },
    };
}

void SpringCompressorProcessor::sync_engine_processor(bool called_from_audio_thread)
{
    Engine::SetParsResult r = Engine::SetParsResult::transfer_curve_didnt_change;

    if (parameter_changed_was_called.exchange(false)) {
        r = engine->set_pars(query_raw_parameter_values_into_EnginePars());
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

    if (name == "scope_mode" || name == "scope_freq") {
        redraw_scope();
    } else {
        parameter_changed_was_called = true;
        if (!audio_thread_running) {
            // Handle it now.
            sync_engine_processor(false);
        }
        scope_data_generator_thread.start_testing(query_raw_parameter_values_into_EnginePars());
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
                            constexpr int M = 1;
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
        // We're calling sync here to make sure we don't miss anything because the audio thread is stopped after
        // a parameterChanged call but before it could call sync_engine_processor.
        sync_engine_processor(false);
    }

    auto completed_request = scope_data_generator_thread.try_get_completed_request();
    if (completed_request && completed_request->request_id != last_scoped_data_drawn_request_id) {
        println("Received request {}", completed_request->request_id);
        last_scoped_data_drawn_request_id = completed_request->request_id;
        redraw_scope();
    }
}

void SpringCompressorProcessor::redraw_scope()
{
    auto* editor = this->getActiveEditor();
    if (!editor) {
        return;
    }

    auto* e = dynamic_cast<SpringCompressorEditor*>(editor);
    auto completed_request = scope_data_generator_thread.try_get_completed_request();
    if (!completed_request) {
        return;
    }

    auto& sd = completed_request->scope_data;

    const auto attack_release = [&sg = sd.step_graphs_by_freq, e, this](bool release) {
        if (!sg.empty()) {
            {
                vector<float> sfv;
                sfv.reserve(sg.size());
                for (auto& x : sg) {
                    sfv.push_back(x.freq_hz);
                }
                e->set_scope_freq_values(sfv);
            }
            const auto freq_percent = raw_parameter_values.scope_freq->load();
            const auto freq_ix = std::min(ifloor<size_t>(freq_percent * ifcast<float>(sg.size())), sg.size() - 1);
            auto& t = sg[freq_ix];
            const float max_ms = 50;
            e->draw_scope_grid(
              0, max_ms, release ? -t.step_graphs.back().step_db : 0, release ? 0 : t.step_graphs.back().step_db, false
            );
            for (auto& s : t.step_graphs) {
                e->add_plot_to_scope(release ? s.release_out_db_by_ms : s.attack_out_db_by_ms, juce::Colours::white);
            }
        }
    };

    const auto hdist_ihdist = [&](bool ihdist) {
        auto& hmd = sd.harmonic_distortion_matrix;
        if (!hmd.freqs_hz.empty() && !hmd.levels_db.empty()) {
            const auto& ms = ihdist ? hmd.inharmonic_distortion_db_by_freq_and_level()
                                    : hmd.harmonic_distortion_db_by_freq_and_level();
            vector<AF2> xy;
            xy.reserve(hmd.freqs_hz.size());
            e->draw_scope_grid(hmd.freqs_hz[0], hmd.freqs_hz.back(), -180, 0, true);
            for (unsigned db_ix = 0; db_ix < hmd.levels_db.size(); db_ix++) {
                xy.clear();
                for (unsigned freq_ix = 0; freq_ix < hmd.freqs_hz.size(); freq_ix++) {
                    const auto y = ms[freq_ix, db_ix];
                    xy.push_back(AF2{hmd.freqs_hz[freq_ix], y});
                }
                e->add_plot_to_scope(xy, juce::Colours::white);
            }
        }
    };
    switch (iround<int>(raw_parameter_values.scope_mode->load())) {
    case 0:
        // transfer
        {
            auto& tg = sd.transfer_graph;
            {
                vector<float> sfv;
                sfv.reserve(tg.transfers_by_freq.size());
                for (auto& x : tg.transfers_by_freq) {
                    sfv.push_back(x.freq_hz);
                }
                e->set_scope_freq_values(sfv);
            }
            if (!tg.transfers_by_freq.empty()) {
                const auto freq_percent = raw_parameter_values.scope_freq->load();
                const auto freq_ix = std::min(
                  ifloor<size_t>(freq_percent * ifcast<float>(tg.transfers_by_freq.size())),
                  tg.transfers_by_freq.size() - 1
                );
                auto& g = tg.transfers_by_freq[freq_ix];
                e->draw_scope_grid(-60, 0, -60, 0, false);
                constexpr float lowest_db = -60.0f;
                if (lowest_db < g.input_output_db[0][0]) {
                    vector<AF2> vs = {
                      {lowest_db, lowest_db},
                      g.input_output_db[0]
                    };
                    e->add_plot_to_scope(vs, juce::Colours::grey);
                }
                e->add_plot_to_scope(g.input_output_db, juce::Colours::white);
            }
        }
        break;
    case 1: // attack
        attack_release(false);
        break;
    case 2: // release
        attack_release(true);
        break;
    case 3: // hdist
        hdist_ihdist(false);
        break;
    case 4: // inhdist
        hdist_ihdist(true);
        break;
    case 5: // threshold
        break;
    default:
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpringCompressorProcessor();
}
