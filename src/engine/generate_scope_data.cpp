#include "generate_scope_data.h"

#include "meadow/math.h"

#include <meadow/cppext.h>
#include <meadow/matlab.h>
#include <meadow/music.h>

namespace
{
constexpr double k_semitones_per_test_freq = 2.0;
constexpr int k_dbs_per_test_level = 1;
constexpr double k_lowest_test_freq = 40.0;
constexpr double k_highest_test_freq = 12000;
constexpr double k_sample_rate = 48000;
constexpr double k_stable_gain_max_db_per_sec = 0.1;

double sum_of_squares(const vector<float>& xs)
{
    double s = 0;
    for (auto x : xs) {
        s += square(x);
    }
    return s;
}

struct StableGainTestResult {
    int input_rms_db;
    int samples_for_gain_to_stabilize;
    double gain_db; // The final, stable gain.
};
// Drive the compressor with a sine of period length T at various input levels and return data about when
// the gain stabilized.
vector<StableGainTestResult> get_lowest_input_rms_db_to_switch_on_compressor(const EnginePars& pars, int T)
{
    auto engine = make_engine();
    auto _ = engine->set_pars(pars);
    engine->prepare_to_play(k_sample_rate, T, 1);
    optional<int> prev_samples_until_compression;
    // If the compressor measures RMS we need a sine with
    //     threshold_db RMS = threshold_db+3 amplitude
    // to hit the threshold.
    // If it measures amplitude, we need
    //     threshold_db-3 RMS = threshold_db amplitude
    // We pick the smaller one.
    const auto k_3_db = matlab::pow2db(2.0);
    const auto min_rms_db = ifloor<int>(pars.transfer_curve.threshold_db - k_3_db);
    vector<StableGainTestResult> results;
    for (int rms_db = 0; rms_db >= min_rms_db; rms_db -= k_dbs_per_test_level) {
        // Generate one period of input signal.
        vector<float> input_signal_one_period;
        input_signal_one_period.reserve(sucast(T));
        const auto A = matlab::db2mag<double>(rms_db + k_3_db);
        for (int i = 0; i < T; ++i) {
            input_signal_one_period.push_back(ffcast<float>(cos(2 * num::pi * i / T) * A));
        }

        // Prepare calling engine process.
        vector<float> buf(sucast(T));
        vector<Engine::Trace> trace;
        float* const channel = buf.data();
        enum class Phase {
            waiting_for_compression,
            waiting_for_stable_gain
        } phase = Phase::waiting_for_compression;
        optional<int> samples_until_compression, samples_until_stable_gain;
        double prev_min_gain = 1.0;
        engine->reset();

        // Feed input signal into the engine and check output.
        // We need to wait for the compressor first to switch on (gain < 1) then for the gain to stabilize.
        for (int sample_ix = 0;; sample_ix += T) {
            ra::copy(input_signal_one_period, buf.begin());
            engine->process_block_with_trace(span(&channel, 1), iicast<int>(input_signal_one_period.size()), trace);

            const auto min_gain = ra::min(trace, {}, &Engine::Trace::gain).gain;
            assert(
              min_gain <= 1.0
            ); // This compressor never amplifies if makeup_gain_db is zero at -INFINITY reference_level_db.
            bool exit_loop = false;
            switch (phase) {
            case Phase::waiting_for_compression: {
                // Waiting for min_gain < 1, exit if it took too long.
                if (min_gain == 1.0) {
                    if (prev_samples_until_compression) {
                        // Wait at most twice as much.
                        if (sample_ix > 2 * *prev_samples_until_compression) {
                            exit_loop = true;
                        }
                    } else {
                        // Wait at most 1 second.
                        if (sample_ix > k_sample_rate) {
                            exit_loop = true;
                        }
                    }
                } else {
                    // min_gain < 1.0, compressor switched on, go to next phase.
                    samples_until_compression = sample_ix + T;
                    phase = Phase::waiting_for_stable_gain;
                }
            } break;
            case Phase::waiting_for_stable_gain: {
                const auto change_db_per_sec = abs(matlab::mag2db(prev_min_gain / min_gain)) / (T / k_sample_rate);
                if (change_db_per_sec <= k_stable_gain_max_db_per_sec) {
                    // Stable gain reached.
                    samples_until_stable_gain = sample_ix;
                } else {
                    // Gain must be stable within 2 sec.
                    assert(sample_ix < *samples_until_compression + 2 * k_sample_rate);
                }
            } break;
            }
            if (exit_loop || samples_until_stable_gain) {
                break;
            }
            prev_min_gain = min_gain;
        }
        if (samples_until_compression) {
            prev_samples_until_compression = *samples_until_compression;
            results.push_back(
              StableGainTestResult{
                .input_rms_db = rms_db,
                .samples_for_gain_to_stabilize = *samples_until_stable_gain,
                .gain_db = matlab::pow2db(sum_of_squares(buf) / sum_of_squares(input_signal_one_period))
              }
            );
        } else {
            break;
        }
    }
    ra::reverse(results);
    return results;
}
} // namespace

ScopeData generate_scope_data(EnginePars pars, int64_t request_id, std::atomic<int64_t>* current_request_id)
{
    if (pars.transfer_curve.ratio == 1.0f) {
        return {};
    }
    // Clear makeup gain.
    pars.transfer_curve.makeup_gain_db = 0;
    pars.transfer_curve.reference_level_db = -INFINITY;

    const int highest_freq_T = std::max(2, iceil<int>(k_sample_rate / k_highest_test_freq));
    vector<int> Ts = {highest_freq_T};
    const double freq_beta = semitones2ratio(k_semitones_per_test_freq);
    for (;;) {
        const int T = Ts.back();
        const int next_T = std::max(T + 1, ifloor<int>(T * freq_beta));
        if (k_sample_rate / next_T < k_lowest_test_freq) {
            break;
        }
        Ts.push_back(next_T);
    }
    ra::reverse(Ts);

    struct TestResultsAtFreq {
        vector<StableGainTestResult> stable_gain_at_input_levels;
    };
    std::map<double, TestResultsAtFreq> test_results;
    ScopeData scope_data;
    for (auto T : Ts) {
        if (current_request_id && current_request_id->load() != request_id)
            return {};
        auto r = get_lowest_input_rms_db_to_switch_on_compressor(pars, T);
        const double freq_hz = k_sample_rate / T;
        auto& outputs_at_freq = scope_data.transfer_graph.transfers_by_freq.emplace_back();
        outputs_at_freq.freq_hz = ffcast<float>(freq_hz);
        outputs_at_freq.input_output_db.reserve(r.size());
        for (auto x : r) {
            outputs_at_freq.input_output_db.push_back(
              AF2{ifcast<float>(x.input_rms_db), ffcast<float>(x.input_rms_db + x.gain_db)}
            );
        }
        test_results[freq_hz].stable_gain_at_input_levels = MOVE(r);
    }

    return scope_data;
}

ScopeDataGeneratorThread::ScopeDataGeneratorThread()
{
    thread = std::jthread([this]() {
        thread_function();
    });
}

ScopeDataGeneratorThread::~ScopeDataGeneratorThread()
{
    last_request_id = -1;
    thread_should_exit = true;
}

void ScopeDataGeneratorThread::start_testing(const EnginePars& pars)
{
    const auto request_id = next_request_id++;
    last_request_id = request_id;
    ui_scope_data_generator_queue.enqueue(TestRequest{.request_id = request_id, .pars = pars});
}

void ScopeDataGeneratorThread::thread_function()
{
    // vector<std::jthread> threads(std::thread::hardware_concurrency());
    TestRequest test_request;
    while (!thread_should_exit.load()) {
        if (!ui_scope_data_generator_queue.wait_dequeue_timed(test_request, chr::milliseconds(20))) {
            continue;
        }
        // Process pars.
        println("Starting scope data generation for request {}", test_request.request_id);
        auto r = generate_scope_data(test_request.pars, test_request.request_id, &last_request_id);
        if (last_request_id.load() != test_request.request_id) {
            println("Request {} aborted", test_request.request_id);
            continue;
        } else {
            println("Request {} completed", test_request.request_id);
            completed_requests_queue.enqueue(
              CompletedRequest{.request_id = test_request.request_id, .pars = test_request.pars, .scope_data = MOVE(r)}
            );
        }
    }
}

ScopeDataGeneratorThread::CompletedRequest* ScopeDataGeneratorThread::try_get_completed_request()
{
    CompletedRequest completed_request;
    // Exhaust queue to get the latest one.
    while (completed_requests_queue.try_dequeue(completed_request)) {
        last_completed_request = MOVE(completed_request);
    }
    if (last_completed_request) {
        return &*last_completed_request;
    }
    return nullptr;
}
