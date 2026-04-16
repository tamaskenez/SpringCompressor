#pragma once

#include <meadow/cppext.h>
#include <meadow/unique_token.h>

namespace juce
{
class AudioProcessor;
}

namespace detail
{
// ScopedFileLogSinkActivator sets the index into thread-local variable and resets it when going out of scope.
class ScopedFileLogSinkActivator
{
public:
    explicit ScopedFileLogSinkActivator(size_t instance_index_arg);
    ~ScopedFileLogSinkActivator();

private:
    unique_token token;
    size_t instance_index;
};
} // namespace detail

// FileLogSink initializes a log file which is tied to the specified AudioProcessor. The log file can be written using
// abseil LOG macros. All entry points of the AudioProcessor (called by JUCE) must call the activate() function.
// This whole mechanism is needed because hosts (DAWs) load plugin instances into the same process and without special
// care the usual logging machinery (juce::FileLogger, abseil log) is not usable since they're all built upon global
// objects, shared by different instances of the same plugin.
class FileLogSink
{
public:
    FileLogSink(const juce::AudioProcessor* instance_arg, const string& subdirectory_name, const string& filename_root);
    ~FileLogSink();

    [[nodiscard]] detail::ScopedFileLogSinkActivator activate() const;

private:
    const void* instance;
    size_t instance_index;
};
