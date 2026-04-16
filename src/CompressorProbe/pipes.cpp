#include "pipes.h"

#include "juce_util/logging.h"

string get_pipe_name_for_id(int id)
{
    return format("CompressorProbe-{}", id);
}

unique_ptr<Pipe> Pipe::connect(int id, FileLogSink& file_log_sink)
{
    auto a = file_log_sink.activate();
    auto p = make_unique<Pipe>(file_log_sink);
    auto name = get_pipe_name_for_id(id);
    if (p->connectToPipe(name, -1)) {
        LOG(INFO) << format("Pipe::connect({}) -> success with name \"{}\"", id, name);
        return p;
    }
    LOG(ERROR) << format("Pipe::connect({}) -> failed with name \"{}\"", id, name);
    return nullptr;
}

unique_ptr<Pipe> Pipe::create_new(int id, FileLogSink& file_log_sink)
{
    auto a = file_log_sink.activate();

    auto p = make_unique<Pipe>(file_log_sink);
    auto name = get_pipe_name_for_id(id);
    if (p->createPipe(name, -1, true)) {
        LOG(INFO) << format("Pipe::create_new({}) -> success with name \"{}\"", id, name);
        return p;
    }
    LOG(ERROR) << format("Pipe::create_new({}) -> failed with name \"{}\"", id, name);
    return nullptr;
}

Pipe::Pipe(FileLogSink& file_log_sink_arg)
    : juce::InterprocessConnection(/*callbacksOnMessageThread=*/true)
    , file_log_sink(file_log_sink_arg)
{
}

Pipe::~Pipe()
{
    disconnect(1000);
}

void Pipe::messageReceived(const juce::MemoryBlock& message)
{
    auto a = file_log_sink.activate();

    LOG_IF(FATAL, message.getSize() != sizeof(Command::V))
      << format("Pipe::messageReceived, invalid size: {} instead of {}", message.getSize(), sizeof(Command::V));
    LOG(INFO) << format("Pipe::messageReceived, size: {}", message.getSize());

    static_assert(std::is_trivially_copyable_v<Command::V>);
    Command::V cmd;
    std::memcpy(&cmd, message.getData(), sizeof(cmd));
    if (on_command) {
        on_command(MOVE(cmd));
    }
}

bool Pipe::send_command(const Command::V& cmd)
{
    auto a = file_log_sink.activate();

    if (sendMessage(juce::MemoryBlock(&cmd, sizeof(cmd)))) {
        LOG(INFO) << format("Pipe::send_command, size: {}", sizeof(cmd));
        return true;
    } else {
        LOG(ERROR) << format("Pipe::send_command, size: {}", sizeof(cmd));
        return false;
    }
}

void Pipe::connectionMade()
{
    auto a = file_log_sink.activate();

    LOG(INFO) << "Pipe::connectionMade";
}

void Pipe::connectionLost()
{
    auto a = file_log_sink.activate();

    LOG(INFO) << "Pipe::connectionLost";
}
