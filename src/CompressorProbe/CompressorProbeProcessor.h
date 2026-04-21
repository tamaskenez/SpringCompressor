#pragma once

#include "CommonState.h"
#include "CompressorProbeMessageThreadState.h"
#include "CompressorProbeThreadSafeState.h"
#include "GeneratorRole.h"
#include "ProbeRole.h"
#include "ProcessorInterface.h"
#include "juce_util/AudioProcessorWithDefaults.h"
#include "juce_util/JuceTimer.h"

class CompressorProbeEditor;
class RoleInterface;
class FileLogSink;
class ProbeRole;
class GeneratorRole;

class CompressorProbeProcessor
    : public AudioProcessorWithDefaults
    , public ProcessorInterface
    , juce::AudioProcessorValueTreeState::Listener
{
    friend class ProbeRole;
    friend class GeneratorRole;

public:
    CompressorProbeProcessor();
    ~CompressorProbeProcessor() override;

    // AudioProcessor callbacks on some non-audio thread.
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void numChannelsChanged() override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

protected:
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

public:
    // AudioProcessor callbacks on the audio thread.
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // AudioProcessor callbacks on the message thread.
    juce::AudioProcessorEditor* createEditor() override;

    // AudioProcessorValueTreeState::Listener callback (any thread, including audio).
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // The _mt postfix means: to be called on message-thread.
    void on_ui_refresh_timer_elapsed_mt();

    // ProcessorInterface functions
    void on_role_selected_by_user_mt(Role role) override;

private:
    template<class Fn>
    void call_async_on_mt(Fn&& fn_arg)
    {
        bool result = juce::MessageManager::callAsync([fn = MOVE(fn_arg),
                                                       this_lifetime_token_mutex_copy = this_lifetime_token_mutex,
                                                       weak_token = weak_ptr(this_lifetime_token)] {
            auto lock = std::scoped_lock(*this_lifetime_token_mutex_copy);
            if (auto locked_token = weak_token.lock()) {
                fn();
            }
        });
        DCHECK(result);
    }

    CompressorProbeThreadSafeState ts_state;
    CompressorProbeMessageThreadState mt_state;
    JuceTimer ui_refresh_timer;
    CommonState common_state;
    std::mutex mutex;
    unique_ptr<RoleInterface> role_impl_storage;

    // this_lifetime_token_mutex is intentionally not a std::shared_mutex, because we need to keep it
    // alive even when the object is destructed.
    shared_ptr<std::mutex> this_lifetime_token_mutex = make_shared<std::mutex>();
    shared_ptr<monostate> this_lifetime_token = make_shared<monostate>();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProbeProcessor)
};
