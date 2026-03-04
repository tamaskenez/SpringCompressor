#include "JuceTimer.h"

JuceTimer::JuceTimer(std::function<void()> callback_arg)
    : callback(std::move(callback_arg))
{
}

void JuceTimer::timerCallback()
{
    if (callback) {
        callback();
    }
}
