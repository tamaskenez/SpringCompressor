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
constexpr int k_num_periods_to_wait_until_announced_stable = 12;
constexpr double k_safety_margin_db = 0.1;
const auto k_3_db = matlab::pow2db<double>(2);

double sum_of_squares(const vector<float>& xs)
{
    double s = 0;
    for (auto x : xs) {
        s += square(x);
    }
    return s;
}

// Generate `num_periods` periods of a cosine wave, `length` samples in total.
// The length of one period in samples will be `length / num_periods`.
vector<float> make_test_signal(int num_periods, int length, double A)
{
    assert(num_periods * 2 <= length);
    vector<float> xs;
    xs.reserve(sucast(length));
    const double T = ifcast<double>(length) / num_periods;
    const double omega = 2 * num::pi / T;
    for (int i = 0; i < length; ++i) {
        xs.push_back(ffcast<float>(cos(omega * i) * A));
    }
    return xs;
}

auto new_engine_ready_to_process (const EnginePars& pars, int T) {
    auto engine = make_engine();
    engine->set_debug_mode(true);
    auto _ = engine->set_pars(pars);
    engine->prepare_to_play(k_sample_rate, T, 1);
    return engine;
}

struct StableGainTestResult {
    double input_rms_db;
    int samples_for_gain_to_stabilize;
    double gain_db; // The final, stable gain.
};
// Drive the compressor with a sine of period length T at various input levels and return data about when
// the gain stabilized.
vector<StableGainTestResult> get_lowest_input_rms_db_to_switch_on_compressor(const EnginePars& pars, int T)
{
    auto engine = new_engine_ready_to_process(pars,T);

    optional<int> prev_samples_until_compression;
    // If the compressor measures RMS we need a sine with
    //     threshold_db RMS = threshold_db+3 amplitude
    // to hit the threshold.
    // If it measures amplitude, we need
    //     threshold_db-3 RMS = threshold_db amplitude
    // We pick the smaller one.
    const auto min_amp_db = ifloor<int>(pars.transfer_curve.threshold_db);
    vector<StableGainTestResult> results;
    for (int amp_db = 0; amp_db >= min_amp_db; amp_db -= k_dbs_per_test_level) {
        // Generate one period of input signal.
        // Subtracting 0.1 dB to leave room for numeric errors and still have all samples -1 .. 1
        const auto input_signal_A = matlab::db2mag<double>(amp_db - k_safety_margin_db);
        const auto input_signal_one_period = make_test_signal(1, T, input_signal_A);

        // Prepare calling engine process.
        vector<float> buf(sucast(T));
        vector<Engine::Trace> trace;
        enum class Phase {
            waiting_for_compression,
            waiting_for_stable_gain
        } phase = Phase::waiting_for_compression;
        optional<int> samples_until_compression, samples_until_stable_gain;
        optional<int> samples_until_gain_stopped_decreasing;
        double min_gain_so_far = 1.0;
        engine->reset();
        // Feed input signal into the engine and check output.
        // We need to wait for the compressor first to switch on (gain < 1) then for the gain to stabilize.
        for (int sample_ix = 0;; sample_ix += T) {
            buf = input_signal_one_period;
            float* const channel = buf.data();
            engine->process_block_with_trace(span(&channel, 1), iicast<int>(buf.size()), trace);

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
                        // Wait at most 2 seconds.
                        if (sample_ix > 2 * k_sample_rate) {
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
                const auto change_db_per_sec = matlab::mag2db(min_gain / min_gain_so_far) / (T / k_sample_rate);
                if (change_db_per_sec < -k_stable_gain_max_db_per_sec) {

                    samples_until_gain_stopped_decreasing = nullopt;

                    // Gain must be stable within 2 sec.
                    assert(sample_ix < *samples_until_compression + 2 * k_sample_rate);

                } else {
                    if (!samples_until_gain_stopped_decreasing) {
                        samples_until_gain_stopped_decreasing = sample_ix;

                    } else if (sample_ix - *samples_until_gain_stopped_decreasing
                               > T * k_num_periods_to_wait_until_announced_stable) { // Is it stable for long enough?
                        samples_until_stable_gain = sample_ix;
                    }
                }
            } break;
            }
            if (exit_loop || samples_until_stable_gain) {
                break;
            }
            min_gain_so_far = std::min(min_gain_so_far, min_gain);
        }
        if (samples_until_compression) {
            prev_samples_until_compression = *samples_until_compression;
            const auto output_energy = sum_of_squares(buf);
            const auto input_energy = sum_of_squares(input_signal_one_period);
            const auto input_rms_db = matlab::pow2db(input_energy / T);
            results.push_back(
              StableGainTestResult{
                .input_rms_db = input_rms_db,
                .samples_for_gain_to_stabilize = *samples_until_stable_gain,
                .gain_db = matlab::pow2db(output_energy / input_energy)
              }
            );
        } else {
            break;
        }
    }
    ra::reverse(results);

    return results;
}

struct TestAttackReleaseResult {
    vector<AF2> attack_db_by_ms;
    vector<AF2> release_db_by_ms;
};
TestAttackReleaseResult
test_attack_and_release(const EnginePars& pars, int T, double step_db, const StableGainTestResult& lo_input)
{
    const auto engine = new_engine_ready_to_process(pars,T);

    const double A_lo = matlab::db2mag(lo_input.input_rms_db + k_3_db);
    const double A_hi = matlab::db2mag(lo_input.input_rms_db + k_3_db + step_db);
    const auto ts_lo = make_test_signal(1, T, A_lo);
    const auto ts_hi = make_test_signal(1, T, A_hi);

    // Start at a sine at lo_input, wait until stable.
    vector<Engine::Trace> trace;
    vector<float> buf(sucast(T));
    float* const channel = buf.data();
    for (int sample_ix = 0; sample_ix < lo_input.samples_for_gain_to_stabilize; sample_ix += T) {
        buf = ts_lo;
        engine->process_block_with_trace(span(&channel, 1), iicast<int>(buf.size()), trace);
    }

    const auto out_db_when_lo = matlab::pow2db(sum_of_squares(buf));

    // Raise input with step_db, wait until stable.
    struct MinGainAtSampleIx {
        double min_gain_db;
        int sample_ix;
    };
    optional<MinGainAtSampleIx> assumed_stable_since;
    bool stable = false;
    const auto two_seconds_samples = iround<int>(k_sample_rate * 2);

    vector<AF2> extra_out_db_by_ms;
    for (int sample_ix = 0; !stable && sample_ix < two_seconds_samples; sample_ix += T) {
        buf = ts_hi;
        engine->process_block_with_trace(span(&channel, 1), iicast<int>(buf.size()), trace);
        const auto min_gain_db = matlab::mag2db(ra::min(trace, {}, &Engine::Trace::gain).gain);
        extra_out_db_by_ms.push_back(
          AF2{
            ffcast<float>(1000.0 * sample_ix / k_sample_rate),
            ffcast<float>(matlab::pow2db(sum_of_squares(buf)) - out_db_when_lo)
          }
        );
        if (assumed_stable_since) {
            const auto dsamples = sample_ix - assumed_stable_since->sample_ix;
            const auto change_db_per_sec =
              (min_gain_db - assumed_stable_since->min_gain_db) / (dsamples / k_sample_rate);
            if (abs(change_db_per_sec) <= k_stable_gain_max_db_per_sec) {
                if (dsamples > T * k_num_periods_to_wait_until_announced_stable) {
                    stable = true;
                }
            } else {
                assumed_stable_since = nullopt;
            }
        }
        if (!assumed_stable_since) {
            assumed_stable_since = MinGainAtSampleIx{.min_gain_db = min_gain_db, .sample_ix = sample_ix};
        }
    }
    assert(stable);

    // Drop to lo_input, wait until stable.

    return TestAttackReleaseResult{.attack_db_by_ms = MOVE(extra_out_db_by_ms), .release_db_by_ms = vector<AF2>()};
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

    // Create test frequencies.
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

    // Test RMS transfer characteristics with steady signals.
    struct TestResultsAtFreq {
        vector<StableGainTestResult> stable_gain_at_input_levels;
    };
    std::map<int, TestResultsAtFreq> test_results;
    ScopeData scope_data;
    for (auto T : Ts) {
        if (current_request_id && current_request_id->load() != request_id)
            return {};
        // Test with signal with period T.
        auto r = get_lowest_input_rms_db_to_switch_on_compressor(pars, T);
        // Put the result in scope_data and test_results.
        const double freq_hz = k_sample_rate / T;
        auto& outputs_at_freq = scope_data.transfer_graph.transfers_by_freq.emplace_back();
        outputs_at_freq.freq_hz = ffcast<float>(freq_hz);
        outputs_at_freq.input_output_db.reserve(r.size());
        for (auto x : r) {
            outputs_at_freq.input_output_db.push_back(
              AF2{ffcast<float>(x.input_rms_db), ffcast<float>(x.input_rms_db + x.gain_db)}
            );
        }
        test_results[T].stable_gain_at_input_levels = MOVE(r);
    }

    // Test attack and release.
    vector<double> step_dbs = {6, 12, 18, 24};
    for (auto T : Ts) {
        auto& step_graph_at_freq = scope_data.step_graphs_by_freq.emplace_back(
          ScopeData::StepGraphsAtFreq{.freq_hz = ffcast<float>(k_sample_rate / T)}
        );
        if (current_request_id && current_request_id->load() != request_id) {
            return {};
        }
        auto& tr = test_results.at(T);

        for (auto step_db : step_dbs) {
            if (tr.stable_gain_at_input_levels[0].input_rms_db + step_db + k_safety_margin_db > -k_3_db) {
                break;
            }
            auto r = test_attack_and_release(pars, T, step_db, tr.stable_gain_at_input_levels[0]);
            step_graph_at_freq.step_graphs.emplace_back(
              ScopeData::StepGraph{
                .step_db = ffcast<float>(step_db),
                .attack_out_db_by_ms = MOVE(r.attack_db_by_ms),
                .release_out_db_by_ms = MOVE(r.release_db_by_ms)
              }
            );
        }
#if 0
        auto r = get_lowest_input_rms_db_to_switch_on_compressor(pars, T);
        const double freq_hz = k_sample_rate / T;
        auto& outputs_at_freq = scope_data.transfer_graph.transfers_by_freq.emplace_back();
        outputs_at_freq.freq_hz = ffcast<float>(freq_hz);
        outputs_at_freq.input_output_db.reserve(r.size());
        for (auto x : r) {
            outputs_at_freq.input_output_db.push_back(
              AF2{ffcast<float>(x.input_rms_db), ffcast<float>(x.input_rms_db + x.gain_db)}
            );
        }
        test_results[freq_hz].stable_gain_at_input_levels = MOVE(r);
#endif
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
