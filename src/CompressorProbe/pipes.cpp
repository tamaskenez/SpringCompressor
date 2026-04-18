#include "pipes.h"

#include "juce_util/logging.h"

string get_pipe_name_for_id(int id)
{
    return format("CompressorProbe-{}", id);
}

unique_ptr<Pipe> Pipe::connect(int id, FileLogSink& file_log_sink)
{
    auto a = file_log_sink.activate();
    auto name = get_pipe_name_for_id(id);
    auto p = make_unique<Pipe>(name, file_log_sink);
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

    auto name = get_pipe_name_for_id(id);
    auto p = make_unique<Pipe>(name, file_log_sink);
    if (p->createPipe(name, -1, true)) {
        LOG(INFO) << format("Pipe::create_new({}) -> success with name \"{}\"", id, name);
        return p;
    }
    LOG(ERROR) << format("Pipe::create_new({}) -> failed with name \"{}\"", id, name);
    return nullptr;
}

Pipe::Pipe(string name_arg, FileLogSink& file_log_sink_arg)
    : juce::InterprocessConnection(/*callbacksOnMessageThread=*/true)
    , name(MOVE(name_arg))
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

    LOG(INFO) << format("Pipe::messageReceived, size: {}", message.getSize());

    if (on_message_received) {
        on_message_received(span(static_cast<const char*>(message.getData()), message.getSize()));
    }
}

bool Pipe::send_message(span<const char> memory_block)
{
    auto a = file_log_sink.activate();

    if (sendMessage(juce::MemoryBlock(memory_block.data(), memory_block.size()))) {
        return true;
    } else {
        LOG(ERROR) << format("Pipe::send_message failed to send {} bytes.", memory_block.size());
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
