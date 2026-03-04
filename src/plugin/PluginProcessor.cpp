#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "engine.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>

#include <cassert>
#include <ranges>
#include <span>

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
        .knee_width_db = apvts.getRawParameterValue("knee_width"),
        .gain_control_application = apvts.getRawParameterValue("gain_filter")
      }
{
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
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
        0.0f,
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
}

void SpringCompressorProcessor::releaseResources()
{
    engine->release_resources();
}

void SpringCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // In isBusesLayoutSupported we only enabled layouts which satisfy this.
    assert(getMainBusNumInputChannels() == getMainBusNumOutputChannels());
    // We don't have any sidechains for now.
    assert(getMainBusNumInputChannels() == getTotalNumInputChannels());
    assert(getMainBusNumOutputChannels() == getTotalNumOutputChannels());

    engine->set_transfer_curve(
      TransferCurvePars{
        .threshold_db = raw_parameter_values.threshold_db->load(),
        .ratio = raw_parameter_values.ratio->load(),
        .knee_width_db = raw_parameter_values.knee_width_db->load(),
        .normalizer = TransferCurveNormalizer::makeup_gain,
        .normalizer_db = raw_parameter_values.makeup_gain_db->load(),
      }
    );
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
}

juce::AudioProcessorEditor* SpringCompressorProcessor::createEditor()
{
    return new SpringCompressorEditor(*this);
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpringCompressorProcessor();
}
