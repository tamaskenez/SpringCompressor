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

void CompressorProbeProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    generator_role.prepare();
    probe_role.prepare();
    engine_running.store(true);
}

void CompressorProbeProcessor::releaseResources()
{
    engine_running.store(false);
}

void CompressorProbeProcessor::processBlock(
  juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/
)
{
    juce::ScopedNoDenormals no_denormals;

    switch (role.load()) {
    case Role::Probe:
        probe_role.process_block(buffer);
        break;
    case Role::Generator:
        generator_role.process_block(buffer);
        break;
    case Role::Unset:
        break; // pass through
    }
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
