#pragma once

#include <juce_events/juce_events.h>

#include <functional>

class JuceTimer : public juce::Timer
{
public:
    JuceTimer() = default;
    explicit JuceTimer(std::function<void()> callback_arg);

private:
    std::function<void()> callback;
    void timerCallback() override;
};
