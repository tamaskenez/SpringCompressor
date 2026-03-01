#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <span>

SelfModProcessor::SelfModProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , engine(make_selfmod_engine())
    , lp_freq_param(apvts.getRawParameterValue("lp_freq"))
    , intensity_param(apvts.getRawParameterValue("intensity"))
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SelfModProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Logarithmic range: normalised value maps linearly to log(freq).
    juce::NormalisableRange<float> log_range(
      1.0f,
      20000.0f,
      [](float start, float end, float norm) {
          return start * std::pow(end / start, norm);
      },
      [](float start, float end, float value) {
          return std::log(value / start) / std::log(end / start);
      }
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lp_freq", 1},
        "LP Frequency",
        log_range,
        100.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("Hz")
      )
    );

    params.push_back(
      std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"intensity", 1}, "Intensity", 1.0f, 10.0f, 1.0f)
    );

    return {params.begin(), params.end()};
}

void SelfModProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine->prepare_to_play(sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void SelfModProcessor::releaseResources()
{
    engine->release_resources();
}

void SelfModProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    engine->set_lp_freq(lp_freq_param->load());
    engine->set_intensity(intensity_param->load());
    auto write_pointers = std::span(buffer.getArrayOfWritePointers(), static_cast<size_t>(buffer.getNumChannels()));
    engine->process_block(write_pointers, buffer.getNumSamples());
}

juce::AudioProcessorEditor* SelfModProcessor::createEditor()
{
    return new SelfModEditor(*this);
}

void SelfModProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SelfModProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SelfModProcessor();
}
