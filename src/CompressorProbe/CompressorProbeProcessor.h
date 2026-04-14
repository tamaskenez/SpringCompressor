#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

enum class Role {
    Unset,
    Generator,
    Probe
};

enum class GeneratorStatus {
    Idle,
    TransmittingId,
    Connected
};

struct GoertzelFilter {
    double coeff = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;

    void reset() noexcept
    {
        s1 = s2 = 0.0;
    }

    void feed(double x) noexcept
    {
        const double s0 = x + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    // Returns |X[k]|^2 for the accumulated samples.
    double power() const noexcept
    {
        return s1 * s1 + s2 * s2 - coeff * s1 * s2;
    }
};

class CompressorProbeProcessor : public juce::AudioProcessor
{
public:
    CompressorProbeProcessor();
    ~CompressorProbeProcessor() override;

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

    int get_generator_id() const noexcept
    {
        return generator_id.load();
    }
    GeneratorStatus get_generator_status() const noexcept
    {
        return generator_status.load();
    }

    bool is_engine_running() const noexcept
    {
        return engine_running.load();
    }
    int get_probe_confirmed_id() const noexcept
    {
        return probe_confirmed_id.load();
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

    // Detection threshold for Goertzel power.
    // Expected power for a tone at amplitude 0.1 over nfft samples: (0.1 * nfft/2)^2 ≈ 41 943.
    // Threshold at ~1/10 of that handles up to 10 dB of compressor attenuation.
    static constexpr double detection_threshold = 4000.0;

    std::atomic<Role> role{Role::Unset};
    std::atomic<bool> engine_running{false};

    // ---- Generator state (audio thread only) ----

    void setup_generator();

    std::atomic<int> generator_id{-1}; // -1 until claimed; 0-65535 once a pipe is created
    std::atomic<GeneratorStatus> generator_status{GeneratorStatus::Idle};
    std::unique_ptr<juce::InterprocessConnection> generator_pipe;
    std::array<float, nfft> id_tone_buf{};
    int tone_playhead = 0;

    // ---- Probe state (audio thread only) ----

    void process_probe_frame();
    void connect_to_generator();

    // One Goertzel filter per bin; coefficients are set in the constructor.
    std::array<GoertzelFilter, 18> probe_filters{};
    int probe_sample_count = 0;
    int probe_last_decoded_id = -1;          // most recently decoded ID, -1 = none yet
    int probe_confirm_count = 0;             // consecutive frames with the same decoded ID
    std::atomic<int> probe_confirmed_id{-1}; // -1 until pairing is complete
    std::unique_ptr<juce::InterprocessConnection> probe_pipe;
    std::atomic<bool> probe_connection_pending{false};
};
