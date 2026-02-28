#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <span>

SelfModProcessor::SelfModProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
    , engine(make_selfmod_engine())
{
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
    auto write_pointers = std::span(buffer.getArrayOfWritePointers(), static_cast<size_t>(buffer.getNumChannels()));
    engine->process_block(write_pointers, buffer.getNumSamples());
}

juce::AudioProcessorEditor* SelfModProcessor::createEditor()
{
    return new SelfModEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SelfModProcessor();
}
