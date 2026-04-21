#pragma once

#include "misc.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <meadow/cppext.h>

/*
Remaining virtual functions.

public:
    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void memoryWarningReceived();
    virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) = 0;
    virtual void processBlock(AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed(AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    virtual bool canAddBus(bool isInput) const;
    virtual bool canRemoveBus(bool isInput) const;
    virtual bool supportsDoublePrecisionProcessing() const;
    virtual bool supportsMPE() const;
    virtual bool isMidiEffect() const;
    virtual void reset();
    virtual AudioProcessorParameter* getBypassParameter() const;
    virtual void setNonRealtime(bool isNonRealtime) noexcept;
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual void refreshParameterList();
    virtual int getNumPrograms() = 0;
    virtual int getCurrentProgram() = 0;
    virtual void setCurrentProgram(int index) = 0;
    virtual const String getProgramName(int index) = 0;
    virtual void changeProgramName(int index, const String& newName) = 0;
    virtual void getStateInformation(MemoryBlock& destData) = 0;
    virtual void getCurrentProgramStateInformation(MemoryBlock& destData);
    virtual void setStateInformation(const void* data, int sizeInBytes) = 0;
    virtual void setCurrentProgramStateInformation(const void* data, int sizeInBytes);
    virtual void numChannelsChanged();
    virtual void numBusesChanged();
    virtual void processorLayoutsChanged();
    virtual void addListener(AudioProcessorListener* newListener);
    virtual void removeListener(AudioProcessorListener* listenerToRemove);
    virtual void setPlayHead(AudioPlayHead* newPlayHead);
    virtual void audioWorkgroupContextChanged([[maybe_unused]] const AudioWorkgroup& workgroup);
    virtual AAXClientExtensions& getAAXClientExtensions();
    virtual VST2ClientExtensions* getVST2ClientExtensions();
    virtual VST3ClientExtensions* getVST3ClientExtensions();
    virtual CurveData getResponseCurve(CurveData::Type curveType) const;
    virtual void updateTrackProperties(const TrackProperties& properties);
    virtual std::optional<String> getNameForMidiNoteNumber(int note, int midiChannel);
protected:
    virtual bool isBusesLayoutSupported(const BusesLayout&) const
    virtual bool canApplyBusesLayout(const BusesLayout& layouts) const
    virtual bool applyBusLayouts(const BusesLayout& layouts);
    virtual bool canApplyBusCountChange(bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
*/

struct AudioProcessorProperties {
    string name;
    vector<string> alternate_display_names;
    double tail_length_seconds = 0.0;
    bool accepts_midi = false;
    bool produces_midi = false;
    bool has_editor = false;
};

class AudioProcessorWithDefaults : public juce::AudioProcessor
{
    juce::String name;
    juce::StringArray alternate_display_names;
    double tail_length_seconds;
    bool accepts_midi = false;
    bool produces_midi = false;
    bool has_editor = false;

protected:
    AudioProcessorWithDefaults(const BusesProperties& ioLayouts, const AudioProcessorProperties& props)
        : AudioProcessor(ioLayouts)
        , name(to_juce_string(props.name))
        , alternate_display_names(to_juce_string_array(props.alternate_display_names))
        , tail_length_seconds(props.tail_length_seconds)
        , accepts_midi(props.accepts_midi)
        , produces_midi(props.produces_midi)
        , has_editor(props.has_editor)
    {
    }
    AudioProcessorWithDefaults(
      const std::initializer_list<const short[2]>& channelLayoutList, const AudioProcessorProperties& props
    )
        : AudioProcessor(channelLayoutList)
        , name(to_juce_string(props.name))
        , alternate_display_names(to_juce_string_array(props.alternate_display_names))
        , tail_length_seconds(props.tail_length_seconds)
        , accepts_midi(props.accepts_midi)
        , produces_midi(props.produces_midi)
        , has_editor(props.has_editor)
    {
    }

public:
    const juce::String getName() const override
    {
        return name;
    }
    juce::StringArray getAlternateDisplayNames() const override
    {
        return alternate_display_names;
    }

    double getTailLengthSeconds() const override
    {
        return tail_length_seconds;
    }
    bool acceptsMidi() const override
    {
        return accepts_midi;
    }
    bool producesMidi() const override
    {
        return produces_midi;
    }
    bool hasEditor() const override
    {
        return has_editor;
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
};
