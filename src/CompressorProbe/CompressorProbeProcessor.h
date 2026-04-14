#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

enum class Role {
    Unset,
    Generator,
    Probe
};

class CompressorProbeProcessor : public juce::AudioProcessor
{
public:
    CompressorProbeProcessor();
    ~CompressorProbeProcessor() override = default;

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override
    {
        return false;
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }

    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return {};
    }
    void changeProgramName(int, const juce::String&) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
            && !layouts.getMainInputChannelSet().isDisabled();
    }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    Role get_role() const noexcept
    {
        return role.load();
    }
    void set_role(Role r) noexcept
    {
        role.store(r);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeProcessor)

private:
    // Fixed FFT size for ID tone encoding/decoding.
    static constexpr int nfft = 4096;

    // 18 prime bin indices, all below nfft/10 (so the highest tone stays below 20 kHz at 192 kHz).
    // Bin 0 (sync_on) is always active. Bin 1 (sync_off) is never active.
    // Bins 2-17 encode the 16-bit generator ID in binary (bit 0 = bin 2, ..., bit 15 = bin 17).
    static constexpr std::array<int, 18> id_bins{
      211, 223, 233, 241, 257, 269, 281, 293, 307, 317, 331, 347, 353, 367, 379, 389, 401, 409
    };

    std::atomic<Role> role{Role::Unset};

    void setup_generator(); // called from processBlock on first block with Generator role

    // Generator state — accessed on the audio thread only.
    int generator_id = -1; // -1 until claimed; 0-65535 once a pipe is created
    std::unique_ptr<juce::InterprocessConnection> generator_pipe;
    std::array<float, nfft> id_tone_buf{};
    int tone_playhead = 0;
};
