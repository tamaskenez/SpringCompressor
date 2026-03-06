#include "SpringCompressorProcessor.h"

#include "SpringCompressorEditor.h"
#include "TransferCurveComponent.h"
#include "engine.h"

#include <magic_enum/magic_enum.hpp>
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
struct RmsSamples {
    // Each AF2 item contains an input and corresponding output RMS value.
    std::inplace_vector<AF2, 16> samples;
};

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
        .attack_ms = apvts.getRawParameterValue("attack"),
        .release_ms = apvts.getRawParameterValue("release"),
        .makeup_gain_db = apvts.getRawParameterValue("makeup"),
        .reference_level_db = apvts.getRawParameterValue("reference_level"),
        .knee_width_db = apvts.getRawParameterValue("knee_width"),
        .gain_control_application = apvts.getRawParameterValue("gain_filter")
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

    for (auto* id :
         {"threshold", "ratio", "attack", "release", "makeup", "reference_level", "knee_width", "gain_filter"})
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
        juce::ParameterID{"attack", 1},
        "Attack",
        juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.5f),
        10.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("ms")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1},
        "Release",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f, 0.5f),
        100.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("ms")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"makeup", 1},
        "Makeup Gain",
        juce::NormalisableRange<float>(0.0f, 32.0f, 0.1f),
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

    params.push_back(
      std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"gain_filter", 1}, "Gain filter", juce::StringArray{"Input", "GR dB", "Linear GR"}, 1
      )
    );

    return {params.begin(), params.end()};
}

void SpringCompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine->prepare_to_play(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    audio_thread_running = true;
}

void SpringCompressorProcessor::releaseResources()
{
    audio_thread_running = false;
    engine->release_resources();

    // There can be messages stuck in the ui_to_audio queue because the audio thread stopped just after we enqueued
    // something but before the audio thread could dequeue it. Process those messages here.
    TransferCurvePars tcp;
    while (ui_to_audio_queue.try_dequeue(tcp)) {
        engine_set_transfer_curve_and_update_ui(tcp);
    }
}

namespace
{
const std::vector<juce::String> k_transfer_curve_parameters = {
  "threshold", "ratio", "knee_width", "makeup", "reference_level"
};
}

void SpringCompressorProcessor::update_ui_with_transfer_curve_update_result(const TransferCurveState& tcur)
{
    ignore_parameter_changed = true;
    {
        auto* p = apvts.getParameter("makeup");
        const auto& nr = p->getNormalisableRange();
        assert(in_cc_range(tcur.makeup_gain_db, nr.start, nr.end));
        p->setValueNotifyingHost(p->convertTo0to1(std::clamp(tcur.makeup_gain_db, nr.start, nr.end)));
    }
    {
        auto* p = apvts.getParameter("reference_level");
        const auto& nr = p->getNormalisableRange();
        assert(in_cc_range(tcur.reference_level_db, nr.start, nr.end));
        p->setValueNotifyingHost(p->convertTo0to1(std::clamp(tcur.reference_level_db, nr.start, nr.end)));
    }
    ignore_parameter_changed = false;

    if (auto* editor = this->getActiveEditor()) {
        dynamic_cast<SpringCompressorEditor*>(editor)->set_transfer_curve(tcur);
        editor->repaint();
    }
}

void SpringCompressorProcessor::engine_set_transfer_curve_and_update_ui(const TransferCurvePars& tcp)
{
    if (auto tcur = engine->set_transfer_curve(tcp)) {
        update_ui_with_transfer_curve_update_result(*tcur);
    }
}

void SpringCompressorProcessor::parameterChanged(const juce::String& name, float f)
{
    if (ignore_parameter_changed) {
        return;
    }
    if (std::ranges::find(k_transfer_curve_parameters, name) == k_transfer_curve_parameters.end()) {
        // The engine never changes non-transfer curve parameters, they don't need special handling. They will be read
        // in processBlock directly.
        return;
    }

    // Use the last normalizer or the one that is being changed now.
    std::optional<float> normalizer_db;
    if (name == "makeup") {
        last_normalizer = TransferCurveNormalizer::makeup_gain;
        normalizer_db = f;
    } else if (name == "reference_level") {
        last_normalizer = TransferCurveNormalizer::reference_level;
        normalizer_db = f;
    } else {
        switch (last_normalizer) {
        case TransferCurveNormalizer::makeup_gain:
            normalizer_db = raw_parameter_values.makeup_gain_db->load();
            break;
        case TransferCurveNormalizer::reference_level:
            normalizer_db = raw_parameter_values.reference_level_db->load();
            break;
        }
    }
    const auto tcp = TransferCurvePars{
      .threshold_db = raw_parameter_values.threshold_db->load(),
      .ratio = raw_parameter_values.ratio->load(),
      .knee_width_db = raw_parameter_values.knee_width_db->load(),
      .normalizer = last_normalizer,
      .normalizer_db = normalizer_db.value()
    };

    if (audio_thread_running) {
        // Handle it on audio thread, in processBlock.
        ui_to_audio_queue.enqueue(tcp);
    } else {
        // Handle it now.
        engine_set_transfer_curve_and_update_ui(tcp);
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

    TransferCurvePars tcp;
    while (ui_to_audio_queue.try_dequeue(tcp)) {
        if (auto tcur = engine->set_transfer_curve(tcp)) {
            audio_to_ui_queue.enqueue(TransferCurveState{*tcur});
        }
    }

    engine->set_attack_ms(raw_parameter_values.attack_ms->load());
    engine->set_release_ms(raw_parameter_values.release_ms->load());
    {
        const auto e = magic_enum::enum_cast<GainControlApplication>(
          iround<int>(raw_parameter_values.gain_control_application->load())
        );
        assert(e);
        engine->set_gain_control_application(e.value_or(GainControlApplication::on_gr_db));
    }

    auto input_buffer = getBusBuffer(buffer, true, 0);
    auto output_buffer = getBusBuffer(buffer, false, 0);
    auto num_channels = sucast(input_buffer.getNumChannels());
    assert(std::cmp_equal(output_buffer.getNumChannels(), num_channels));
    // In practice, these will be the same pointers.
    auto read_pointers = std::span(input_buffer.getArrayOfReadPointers(), num_channels);
    auto write_pointers = std::span(output_buffer.getArrayOfWritePointers(), num_channels);
    assert(std::ranges::equal(read_pointers, write_pointers));

    engine->process_block(write_pointers, buffer.getNumSamples());

    if (editor_open) {
        RmsSamples msg;
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
    auto* editor = new SpringCompressorEditor(*this);
    editor->set_transfer_curve(engine->get_transfer_curve_state());
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

void SpringCompressorProcessor::on_ui_refresh_timer_elapsed()
{
    std::any msg;
    bool rms_matrix_updated = false;
    while (audio_to_ui_queue.try_dequeue(msg)) {
        if (auto* tcur = std::any_cast<TransferCurveState>(&msg)) {
            update_ui_with_transfer_curve_update_result(*tcur);
        } else if (auto* rms_samples = std::any_cast<RmsSamples>(&msg)) {
            rms_matrix_updated |= !rms_samples->samples.empty();
            for (auto rs : rms_samples->samples) {
                const auto input_db = matlab::mag2db(rs[0]);
                const auto output_db = matlab::mag2db(rs[1]);
                if (in_cc_range(input_db, TransferCurveComponent::k_db_min - 1, TransferCurveComponent::k_db_max + 1)
                    && in_cc_range(
                      output_db, TransferCurveComponent::k_db_min - 1, TransferCurveComponent::k_db_max + 1
                    )) {
                    const auto xy = AI2{
                      iround<int>(input_db) - TransferCurveComponent::k_db_min,
                      iround<int>(output_db) - TransferCurveComponent::k_db_min
                    };
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
        } else {
            // We got a message with an unexpected type.
            assert(false);
        }
    }
    if (rms_matrix_updated) {
        if (auto* editor = this->getActiveEditor()) {
            dynamic_cast<SpringCompressorEditor*>(editor)->update_rms_dots(
              rms_matrix_clock, rms_matrix_as_mdspan, k_rms_sample_period_sec
            );
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpringCompressorProcessor();
}
