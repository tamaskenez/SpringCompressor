#pragma once

#include "engine.h"

#include <readerwriterqueue/readerwriterqueue.h>

struct ScopeData {
    // Input-output dB steady signals of different frequencies.
    struct TransferGraph {
        struct TransferAtFreq {
            float freq_hz;
            vector<AF2> input_output_db;
        };
        vector<TransferAtFreq> transfers_by_freq;
    } transfer_graph;

    // Attack and release (step-up, step-down) for different levels.
    struct StepGraph {
        float step_db; // Starting at threshold, we apply a step up, then down.
        // Extra output RMS above output at threshold.
        // First coordinate is milliseconds.
        vector<AF2> attack_out_db_by_ms;
        vector<AF2> release_out_db_by_ms;
    };
    struct StepGraphsAtFreq {
        float freq_hz;
        vector<StepGraph> step_graphs;
    };
    vector<StepGraphsAtFreq> step_graphs_by_freq;

    // Harmonic distortion for steady signals of different frequencies and levels.
    struct HarmonicDistortionGraph {
        vector<float> freqs_hz;
        vector<float> levels_db; // Above threshold.
        vector<float> harmonic_distortion_data, inharmonic_distortion_data;
        std::mdspan<float, std::dextents<int, 2>> harmonic_distortion_db_by_freq_and_level,
          inharmonic_distortion_db_by_freq_and_level;
    };
};

ScopeData
generate_scope_data(EnginePars pars, int64_t request_id = 0, std::atomic<int64_t>* current_request_id = nullptr);

class ScopeDataGeneratorThread
{
public:
    struct CompletedRequest {
        int64_t request_id;
        EnginePars pars;
        ScopeData scope_data;
    };

    ScopeDataGeneratorThread();
    ~ScopeDataGeneratorThread();

    void start_testing(const EnginePars& pars);
    CompletedRequest* try_get_completed_request();

private:
    struct TestRequest {
        int64_t request_id;
        EnginePars pars;
    };
    moodycamel::BlockingReaderWriterQueue<TestRequest> ui_scope_data_generator_queue;
    moodycamel::ReaderWriterQueue<CompletedRequest> completed_requests_queue;
    std::atomic<bool> scope_data_generator_thread_running = false;
    std::jthread thread;
    optional<ScopeData> scope_data;
    std::atomic<bool> thread_should_exit = false;
    std::atomic<int64_t> last_request_id = 0;
    int64_t next_request_id = 1;
    optional<CompletedRequest> last_completed_request;

    void thread_function();
};
