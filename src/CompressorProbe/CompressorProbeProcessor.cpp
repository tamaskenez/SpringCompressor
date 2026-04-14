#include "CompressorProbeProcessor.h"

#include "CompressorProbeEditor.h"

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
{
}

void CompressorProbeProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) {}

void CompressorProbeProcessor::releaseResources() {}

void CompressorProbeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals no_denormals;
    // Pass audio through unchanged.
    juce::ignoreUnused(buffer);
}

juce::AudioProcessorEditor* CompressorProbeProcessor::createEditor()
{
    return new CompressorProbeEditor(*this);
}

void CompressorProbeProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}

void CompressorProbeProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProbeProcessor();
}
