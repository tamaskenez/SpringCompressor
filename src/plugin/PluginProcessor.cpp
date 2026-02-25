#include "PluginProcessor.h"

#include "PluginEditor.h"

SpringCompressorProcessor::SpringCompressorProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , engine(make_engine())
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

    return {params.begin(), params.end()};
}

void SpringCompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine->prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void SpringCompressorProcessor::releaseResources()
{
    engine->reset();
}

void SpringCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    engine->setThresholdDb(apvts.getRawParameterValue("threshold")->load());
    engine->setRatio(apvts.getRawParameterValue("ratio")->load());
    engine->setAttackMs(apvts.getRawParameterValue("attack")->load());
    engine->setReleaseMs(apvts.getRawParameterValue("release")->load());
    engine->setMakeupGainDb(apvts.getRawParameterValue("makeup")->load());

    engine->process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
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
