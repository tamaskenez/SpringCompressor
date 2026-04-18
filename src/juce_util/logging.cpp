#include "logging.h"

#include <absl/base/no_destructor.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink_registry.h>
#include <juce_core/juce_core.h>

#include <absl/log/globals.h>

namespace
{
constexpr size_t k_max_instances = 24;

constexpr size_t k_fallback_instance_index = 0;
thread_local size_t s_active_instance_index = k_fallback_instance_index;

// absl::LogSink, writing to a file. Will be used as a singleton.
class StaticFileLogSink : public absl::LogSink
{
public:
    explicit StaticFileLogSink(const string& subdirectory_name)
    {
        // Add a fallback log file which will be used when the active instance is not set.
        auto juce_file_logger = std::unique_ptr<juce::FileLogger>(juce::FileLogger::createDateStampedLogger(
          juce::FileLogger::getSystemLogFileFolder().getChildFile(subdirectory_name).getFullPathName(),
          "fallback_instance",
          ".txt",
          "BEGIN LOG FOR MESSAGES WHEN INSTANCE IS NOT SET"
        ));
        instance_to_index.emplace(nullptr, k_fallback_instance_index);
        instance_log_paths[k_fallback_instance_index] =
          juce_file_logger->getLogFile().getFullPathName().toUTF8().getAddress();
        std::error_code ec;
        fs::remove(instance_log_paths[k_fallback_instance_index], ec);

        absl::InitializeLog();
        absl::AddLogSink(this);
    }

    // No destructor, we won't even let the system call the base class destructor, this object will be intentionally
    // leaked.

    void Send(const absl::LogEntry& e) override
    {
        if (s_active_instance_index >= instance_log_paths.size()) {
            assert(false);
            return;
        }
        auto& path = instance_log_paths[s_active_instance_index];
        if (path.empty()) {
            assert(false);
            return;
        }
        if (FILE* f = fopen(path.c_str(), "at")) {
            print(f, "{}\n", e.text_message_with_prefix());
            fclose(f);
        }
    }

    size_t add_instance(const void* new_instance, string path)
    {
        CHECK(new_instance);
        CHECK(!path.empty());
        std::lock_guard lock(mutex);
        if (instance_to_index.size() >= k_max_instances) {
            return k_fallback_instance_index;
        }
        vector<size_t> existing_indices;
        existing_indices.reserve(k_max_instances);
        for (auto [instance, index] : instance_to_index) {
            CHECK(instance != new_instance);
            existing_indices.push_back(index);
        }
        ra::sort(existing_indices);
        optional<size_t> new_index;
        for (size_t i = 0; i < existing_indices.size(); ++i) {
            if (existing_indices[i] != i) {
                new_index = i;
                break;
            }
        }
        if (!new_index && existing_indices.size() < k_max_instances) {
            new_index = existing_indices.size();
        }
        if (new_index) {
            auto itb = instance_to_index.emplace(new_instance, *new_index);
            CHECK(itb.second);
            instance_log_paths[*new_index] = MOVE(path);
            return *new_index;
        } else {
            return k_fallback_instance_index;
        }
    }
    void remove_instance(const void* instance)
    {
        std::lock_guard lock(mutex);
        auto it = instance_to_index.find(instance);
        if (it != instance_to_index.end()) {
            instance_log_paths[it->second].clear();
            instance_to_index.erase(it);
        }
    }

private:
    std::mutex mutex;
    unordered_map<const void*, size_t> instance_to_index;

    array<string, k_max_instances> instance_log_paths;
};

// Note that the very first call will determine the subdirectory_name which is supposed to be the company which
// should not change after the first call. In subsequent calls the parameter will be ignored.
StaticFileLogSink& get_static_file_log_sink(const string& subdirectory_name = "juce_logging")
{
    static absl::NoDestructor<StaticFileLogSink> sink(subdirectory_name);
    return *sink;
}

} // namespace

FileLogSink::FileLogSink(
  const juce::AudioProcessor* instance_arg, const string& subdirectory_name, const string& filename_root
)
    : instance(instance_arg)
{
    auto juce_file_logger = std::unique_ptr<juce::FileLogger>(juce::FileLogger::createDateStampedLogger(
      juce::FileLogger::getSystemLogFileFolder().getChildFile(subdirectory_name).getFullPathName(),
      filename_root,
      ".txt",
      format("BEGIN LOG FOR INSTANCE {}", instance)
    ));
    instance_index = get_static_file_log_sink(subdirectory_name)
                       .add_instance(instance, juce_file_logger->getLogFile().getFullPathName().toUTF8().getAddress());
}

FileLogSink::~FileLogSink()
{
    get_static_file_log_sink().remove_instance(instance);
}

detail::ScopedFileLogSinkActivator FileLogSink::activate() const
{
    return detail::ScopedFileLogSinkActivator(instance_index);
}

namespace detail
{
ScopedFileLogSinkActivator::ScopedFileLogSinkActivator(size_t instance_index_arg)
    : instance_index(instance_index_arg)
{
    if (s_active_instance_index == instance_index) {
        token.invalidate();
    } else {
        CHECK(s_active_instance_index == k_fallback_instance_index);
        s_active_instance_index = instance_index;
    }
}

ScopedFileLogSinkActivator::~ScopedFileLogSinkActivator()
{
    if (token.valid()) {
        CHECK(s_active_instance_index == instance_index);
        s_active_instance_index = k_fallback_instance_index;
    }
}
} // namespace detail
