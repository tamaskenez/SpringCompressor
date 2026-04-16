#pragma once

#include "Command.h"

#include "juce_events/juce_events.h"

class FileLogSink;

string get_pipe_name_for_id(int id);

class Pipe : public juce::InterprocessConnection
{
public:
    static unique_ptr<Pipe> connect(int id, FileLogSink& file_log_sink);
    static unique_ptr<Pipe> create_new(int id, FileLogSink& file_log_sink);

    function<void(span<const char> memory_block)> on_message_received;

    explicit Pipe(FileLogSink& file_log_sink_arg);
    ~Pipe() override;

    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock&) override;
    bool send_command(const Command::V& cmd);

private:
    FileLogSink& file_log_sink;
};
