#include "ProbeRole.h"

#include "Command.h"
#include "CompressorProbeProcessor.h"
#include "apvts.h"
#include "juce_util/logging.h"
#include "meadow/math.h"
#include "pipes.h"
#include "sync.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>
#include <meadow/enum.h>
#include <meadow/evariant.h>
#include <meadow/matlab.h>

ProbeRole::ProbeRole(CompressorProbeProcessor& processor_arg)
    : processor(processor_arg)
{
    // The constructor is expected to be called on the message thread, but with processor.mutex locked.
    JUCE_ASSERT_MESSAGE_THREAD
    for (size_t i = 0; i < ProbeProtocol::id_bins.size(); ++i)
        at.filters[i].coeff =
          2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * ProbeProtocol::id_bins[i] / ProbeProtocol::nfft);
}

// Might be called on audio thread, but strictly before process_block.
void ProbeRole::prepare_to_play(double sample_rate, int samples_per_block)
{
    for (auto& f : at.filters) {
        f.reset();
    }
    at.sample_count = 0;
    at.last_decoded_id = -1;
    at.confirm_count = 0;
    at.confirmed_generator_id.reset();

    CHECK(processor.ts_state.prepared_to_play);

    vector<vector<float>> channels_samples(
      sucast(processor.ts_state.num_channels.load()), vector<float>(sucast(samples_per_block))
    );
    const auto N = iceil<size_t>(k_seconds_of_received_audio_blocks * sample_rate / samples_per_block);
    received_audio_blocks = vector<ReceivedAudioBlock>(N);
    for (auto& x : received_audio_blocks) {
        x.channels_samples = channels_samples;
    }
}

void ProbeRole::release_resources()
{
    processor.ts_state.generator_id_in_probe = k_invalid_generator_id;
    received_audio_blocks = vector<ReceivedAudioBlock>();
    processor.call_async_on_mt([this] {
        mt_state.pipe.reset();
    });
}

// Called on audio thread.
void ProbeRole::process_frame()
{
    const double sync_on_power = at.filters[0].power();
    const double sync_off_power = at.filters[1].power();

    int decoded_id = 0;
    for (int b = 0; b < 16; ++b) {
        if (at.filters[sucast(2 + b)].power() > ProbeProtocol::detection_threshold)
            decoded_id |= (1 << b);
    }

    for (auto& f : at.filters) {
        f.reset();
    }

    // Require sync_on present and sync_off absent to accept this frame.
    if (sync_on_power < ProbeProtocol::detection_threshold || sync_off_power > ProbeProtocol::detection_threshold) {
        at.last_decoded_id = -1;
        at.confirm_count = 0;
        return;
    }

    // Require 3 consecutive frames with the same ID before confirming.
    if (decoded_id == at.last_decoded_id) {
        if (++at.confirm_count >= 3) {
            at.confirmed_generator_id = decoded_id;
            processor.call_async_on_mt([this, decoded_id] {
                on_generator_id_decoded_mt(decoded_id);
            });
        }
    } else {
        at.last_decoded_id = decoded_id;
        at.confirm_count = 1;
    }
}

// Called on audio thread.
void ProbeRole::process_block(int64_t block_sample_index, juce::AudioBuffer<float>& buffer)
{
    if (at.confirmed_generator_id) {
        // Find an available ReceivedAudioBlock.
        optional<size_t> rab_index;
        for (unsigned i = 0; i < received_audio_blocks.size(); ++i) {
            const auto j = at.next_rab_to_check;
            at.next_rab_to_check = (at.next_rab_to_check + 1) % received_audio_blocks.size();
            if (!received_audio_blocks[j].allocated_for_message_thread.load()) {
                rab_index = j;
                break;
            }
        }
        if (!rab_index) {
            DCHECK(rab_index);
            LOG(ERROR) << "No available ReceivedAudioBlock";
        } else {
            auto& rab = received_audio_blocks[*rab_index];
            rab.allocated_for_message_thread.store(true);
            CHECK(cmp_equal(rab.channels_samples.size(), buffer.getNumChannels()));
            CHECK(!rab.channels_samples.empty());
            const auto N = rab.channels_samples[0].size();
            CHECK(cmp_equal(buffer.getNumSamples(), N));
            for (unsigned i = 0; i < rab.channels_samples.size(); ++i) {
                CHECK(rab.channels_samples[i].size() == N);
                std::copy_n(buffer.getReadPointer(uscast(i)), N, rab.channels_samples[i].data());
            }
            audio_to_message_queue.enqueue(RABToMt{block_sample_index, *rab_index});
        }
    } else {
        // Accumulate samples and run Goertzel detection frame by frame.
        const int num_samples = buffer.getNumSamples();
        const int num_channels = buffer.getNumChannels();
        for (int i = 0; i < num_samples; ++i) {
            double mono = 0.0;
            for (int ch = 0; ch < num_channels; ++ch)
                mono += buffer.getSample(ch, i);
            mono /= num_channels;
            for (auto& f : at.filters)
                f.feed(mono);
            if (++at.sample_count == ProbeProtocol::nfft) {
                process_frame();
                at.sample_count = 0;
            }
        }
    }
    buffer.clear();
}

void ProbeRole::on_generator_id_decoded_mt(int id)
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();
    LOG(INFO) << format("ProbeRole::on_generator_id_decode({})", id);

    {
        auto p = Pipe::connect(id, *processor.ts_state.file_log_sink);
        if (!p) {
            processor.mt_state.error = format(
              "Generator ID #{} received but failed to connect to named pipe \"{}\"", id, get_pipe_name_for_id(id)
            );
            return;
        }
        mt_state.pipe = MOVE(p);
    }
    mt_state.pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received_mt(memory_block);
    };

    processor.ts_state.generator_id_in_probe = id;

    on_mode_changed_mt();
}

void ProbeRole::on_pipe_message_received_mt(span<const char> memory_block)
{
    auto a = processor.ts_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_pipe_message_received(size: {})", memory_block.size());

    auto response_or = response_from_span(memory_block);
    if (!response_or) {
        LOG(ERROR) << format("ProbeRole::on_pipe_message_received: {}", response_or.error());
        return;
    }
    auto& response = *response_or;
    if (mt_state.pending_command && response.command_index == mt_state.pending_command->command_index) {
        LOG(INFO) << format(
          "Pending command response received, command_index: {}, delay: {} µs",
          response.command_index,
          chr::duration_cast<chr::microseconds>(chr::steady_clock::now() - response.command_received_timestamp).count()
        );
    }
    if (!processor.mt_state.sample_rate) {
        mt_state.active_command.reset();
        mt_state.pending_command.reset();
        LOG(WARNING) << "Received a response to a command before sample rate was set.";
        return;
    }

    AnalyzerWorkspace::V analyzer_workspace;
    switch (enum_of(mt_state.pending_command->mode)) {
    case Mode::E::Bypass:
        mt_state.test_signal_period_samples = 0;
        analyzer_workspace = AnalyzerWorkspace::Bypass{};
        break;
    EVARIANT_CASE(mt_state.pending_command->mode, Mode, DecibelCycle, x) {
        auto dc = AnalyzerWorkspace::DecibelCycle(x, *processor.mt_state.sample_rate);
        mt_state.test_signal_period_samples =
          iicast<unsigned>(dc.decibel_cycle_loop_generator.normalized_period.samples.size());
        analyzer_workspace = MOVE(dc);
    } break;
    }
    mt_state.active_command = ActiveCommand{
      .command = *mt_state.pending_command,
      .command_received_timestamp = response.command_received_timestamp,
      .analyzer_workspace = MOVE(analyzer_workspace)
    };
    mt_state.pending_command.reset();
}

void ProbeRole::on_mode_changed_mt()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_mode_changed({})", magic_enum::enum_name(get_mode(processor.mt_state.apvts)));

    auto new_command = Command(mt_state.next_command_index++, get_mode_v(processor.mt_state.apvts));
    if (mt_state.pipe->send_message(command_as_span(new_command))) {
        mt_state.pending_command = new_command;
        mt_state.active_command.reset();
    } else {
        processor.mt_state.error = format("Failed to send command#{} to generator.", new_command.command_index);
    }
}

void ProbeRole::on_ui_refresh_timer_elapsed_mt()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();
    RABToMt rab_msg;

    auto sample_rate_or = processor.mt_state.sample_rate;
    if (!sample_rate_or) {
        // Just give back the blocks.
        while (audio_to_message_queue.try_dequeue(rab_msg)) {
            received_audio_blocks[rab_msg.rab_index].allocated_for_message_thread.store(false);
        }
        return;
    }

    auto sample_rate = *sample_rate_or;
    const auto num_wave_scope_samples =
      iround<size_t>(k_wave_scope_duration_sec * sample_rate) + mt_state.test_signal_period_samples;

    auto& input_samples_tail = processor.mt_state.compressor_input_samples_tail;
    auto& output_samples_tail = processor.mt_state.compressor_output_samples_tail;
    if (input_samples_tail.size() != num_wave_scope_samples || output_samples_tail.size() != num_wave_scope_samples) {
        input_samples_tail.assign(num_wave_scope_samples, 0.0f);
        output_samples_tail.assign(num_wave_scope_samples, 0.0f);
    }
    auto& input_block = mt_state.compressor_input_block;
    auto& output_block = mt_state.compressor_output_block;
    while (audio_to_message_queue.try_dequeue(rab_msg)) {
        auto& rab = received_audio_blocks[rab_msg.rab_index];
        switch (rab.channels_samples.size()) {
        case 1:
            output_block = rab.channels_samples[0];
            break;
        case 2:
            CHECK(rab.channels_samples[0].size() == rab.channels_samples[1].size());
            switch (get_choice<Channels>(processor.mt_state.apvts, "channels")) {
            case Channels::left:
                output_block = rab.channels_samples[0];
                break;
            case Channels::right:
                output_block = rab.channels_samples[1];
                break;
            case Channels::sum:
                output_block.assign_range(
                  vi::zip(rab.channels_samples[0], rab.channels_samples[1]) | vi::transform([](auto&& p) {
                      return std::midpoint(std::get<0>(p), std::get<1>(p));
                  })
                );
                break;
            }
            break;
        default:
            CHECK(false);
        }
        rab.allocated_for_message_thread.store(false); // Give back to audio thread.

        const auto N = output_block.size();

        // compressed_block is ready to be analyzed.
        sync_if_needed(rab_msg.block_sample_index, output_block);
        if (mt_state.active_command && mt_state.active_command->test_signal_begin_sample_index) {
            mt_state.compressor_input_block.resize(N);
            processor.mt_state.input_output_next_sample_index_within_period =
              reproduce_compressor_input_block_mt(rab_msg.block_sample_index, mt_state.compressor_input_block);
            analyze_compressed_block_mt(mt_state.compressor_input_block, mt_state.compressor_output_block);
        } else {
            mt_state.compressor_input_block.assign(N, 0.0f);
        }
        // Send the input/output block to UI.
        const auto num_samples_to_ui = std::min(N, num_wave_scope_samples);
        const auto num_retained_samples = num_wave_scope_samples - num_samples_to_ui;
        for (auto b : {input_samples_tail.begin(), output_samples_tail.begin()}) {
            ra::copy_n(b + uscast(num_wave_scope_samples - num_retained_samples), uscast(num_retained_samples), b);
        }
        std::copy_n(
          input_block.begin() + uscast(N - num_samples_to_ui),
          num_samples_to_ui,
          input_samples_tail.begin() + uscast(num_retained_samples)
        );
        std::copy_n(
          output_block.begin() + uscast(N - num_samples_to_ui),
          num_samples_to_ui,
          output_samples_tail.begin() + uscast(num_retained_samples)
        );
    }
    processor.mt_state.update_wave_scope();
    processor.mt_state.update_analyzer_scope();
}

void ProbeRole::sync_if_needed(int64_t block_sample_index, span<const float> output_block)
{
    if (!mt_state.active_command || mt_state.active_command->test_signal_begin_sample_index) {
        return;
    }

    // Look for sync marker starting with the previous compressed block.
    auto& compressor_output_tail = mt_state.active_command->compressor_output_blocks_tail;
    // Keep the size of compressed_blocks_tail at a minimum.
    const auto sync_signal = get_sync_signal();
    const auto sync_size = sync_signal.size();
    // Keep compressor_output_tail short.
    if (compressor_output_tail.size() > sync_size) {
        const auto remove_count = compressor_output_tail.size() - sync_size;
        compressor_output_tail.erase(
          compressor_output_tail.begin(), compressor_output_tail.begin() + uscast(remove_count)
        );
    }

    const auto current_block_start_index_in_tail = compressor_output_tail.size();

    // Start at the point where the search overlaps with this block at 1 sample.
    const auto begin_pos = std::max<ptrdiff_t>(0, signed_subtract(current_block_start_index_in_tail, sync_size) + 1);
    compressor_output_tail.append_range(output_block);
    const auto end_pos = signed_subtract(compressor_output_tail.size(), sync_size);

    optional<ptrdiff_t> sync_detected_pos;

    auto& corrs = mt_state.active_command->sync_corrs;

    for (auto i = begin_pos; i <= end_pos; ++i) {
        const auto signal_to_test = span(compressor_output_tail.data() + i, sync_size);
        if (!test_sync_signal(signal_to_test)) {
            corrs.clear();
            continue;
        }
        const auto corr = normalized_correlation(signal_to_test, sync_signal);
        if (corr < k_min_corr) {
            corrs.clear();
            continue;
        }
        switch (corrs.size()) {
        case 0:
            corrs.push_back(corr);
            break;
        case 1:
            if (corrs.back() < corr) {
                corrs.push_back(corr);
            } else {
                corrs.assign(1, corr);
            }
            // if size == 2, it will be increasing
            break;
        case 2:
            if (corrs.back() < corr) {
                // Keeps on increasing.
                corrs = {corrs.back(), corr};
            } else {
                // Peaked at the last sample.
                sync_detected_pos = i - 1;
            }
            break;
        default:
            CHECK(false);
        }
        if (sync_detected_pos) {
            break;
        }
    }
    if (!sync_detected_pos) {
        return;
    }

    mt_state.active_command->test_signal_begin_sample_index =
      block_sample_index + signed_subtract(*sync_detected_pos + uscast(sync_size), current_block_start_index_in_tail);
    LOG(INFO) << "Sync detected";
}

unsigned ProbeRole::reproduce_compressor_input_block_mt(int64_t block_sample_index, span<float> input_block)
{
    CHECK(mt_state.active_command && mt_state.active_command->test_signal_begin_sample_index);
    auto& test_signal_begin_sample_index = *mt_state.active_command->test_signal_begin_sample_index;

    switch (enum_of(mt_state.active_command->command.mode)) {
    case Mode::E::Bypass:
        return 0;
    EVARIANT_CASE(mt_state.active_command->command.mode, Mode, DecibelCycle, x) {
        auto& aw = mt_state.active_command->analyzer_workspace;
        auto* awdc = std::get_if<AnalyzerWorkspace::DecibelCycle>(&aw);
        CHECK(awdc);
        auto& wsp = *awdc;
        // Find the start of the most recent cycle.
        while (block_sample_index - test_signal_begin_sample_index
               >= wsp.decibel_cycle_loop_generator.cycle_length_samples) {
            test_signal_begin_sample_index += wsp.decibel_cycle_loop_generator.cycle_length_samples;
        }
        size_t start_sample; // In the current block
        if (test_signal_begin_sample_index <= block_sample_index) {
            start_sample = 0;
        } else {
            start_sample = sucast(test_signal_begin_sample_index - block_sample_index);
            std::fill_n(input_block.begin(), start_sample, 0.0f);
        }
        const auto sample_index_into_loop = block_sample_index + uscast(start_sample) - test_signal_begin_sample_index;
        CHECK(in_co_range(sample_index_into_loop, 0, wsp.decibel_cycle_loop_generator.cycle_length_samples));
        wsp.decibel_cycle_loop_generator.generate_block(
          x,
          iicast<unsigned>(sample_index_into_loop),
          span(input_block.data() + start_sample, input_block.size() - start_sample)
        );
        wsp.block_sample_index_in_cycle = iicast<unsigned>(modulo(
          signed_subtract(sample_index_into_loop, start_sample), wsp.decibel_cycle_loop_generator.cycle_length_samples
        ));
        return iicast<unsigned>(
          (sucast(sample_index_into_loop) + input_block.size() - start_sample)
          % wsp.decibel_cycle_loop_generator.normalized_period.samples.size()
        );
    }
    }
}

void ProbeRole::analyze_compressed_block_mt(span<const float> input_block, span<const float> output_block)
{
    const auto N = input_block.size();
    CHECK(output_block.size() == N);

    CHECK(mt_state.active_command);
    auto& aw = mt_state.active_command->analyzer_workspace;
    switch (enum_of(aw)) {
    case AnalyzerWorkspace::E::Bypass:
        break;
    EVARIANT_CASE(aw, AnalyzerWorkspace, DecibelCycle, x) {
        // Find the energies of periods in the input/output samples.
        const auto P = x.decibel_cycle_loop_generator.normalized_period.samples.size();
        auto& aodc = processor.mt_state.ao.decibel_cycle;
        for (unsigned i = 0; i < N; ++i) {
            x.input_period_sum2 += square(input_block[i]);
            x.output_period_sum2 += square(output_block[i]);
            const bool finished_last_sample_of_period = (x.block_sample_index_in_cycle + i + 1) % P == 0;
            if (finished_last_sample_of_period) {
                const auto input_db = matlab::pow2db(x.input_period_sum2 / ifcast<double>(P));
                const auto output_db = matlab::pow2db(x.output_period_sum2 / ifcast<double>(P));
                x.input_period_sum2 = 0.0;
                x.output_period_sum2 = 0.0;
                if (x.previous_input_db) {
                    const bool ascending = input_db > *x.previous_input_db;
                    if (ascending) {
                        auto& xs = aodc.ascending;
                        if (xs.empty() || xs.back().input_db >= input_db) {
                            // Changed direction, this is the first ascending item in the current cycle.
                            aodc.first_ascending_seq_index = aodc.seq_index;
                            // Remove descending items before their first.
                            auto& ys = aodc.descending;
                            while (!ys.empty() && ys.front().seq_index < aodc.first_descending_seq_index) {
                                ys.pop_front();
                            }
                        }
                        // Remove old items less than this.
                        while (!xs.empty() && xs.front().input_db <= input_db
                               && xs.front().seq_index < aodc.first_ascending_seq_index) {
                            xs.pop_front();
                        }
                        xs.push_back(
                          CompressorProbeMessageThreadState::AnalyzerOutput::DecibelCycle::Item{
                            aodc.seq_index++, input_db, output_db
                          }
                        );
                    } else {
                        // Descending.
                        auto& xs = aodc.descending;
                        if (xs.empty() || xs.back().input_db <= input_db) {
                            // Changed direction, this is the first descending item in the current cycle.
                            aodc.first_descending_seq_index = aodc.seq_index;
                            // Remove ascending items before their first.
                            auto& ys = aodc.ascending;
                            while (!ys.empty() && ys.front().seq_index < aodc.first_ascending_seq_index) {
                                ys.pop_front();
                            }
                        }
                        // Remove old items greater than this.
                        while (!xs.empty() && xs.front().input_db >= input_db
                               && xs.front().seq_index < aodc.first_descending_seq_index) {
                            xs.pop_front();
                        }
                        xs.push_back(
                          CompressorProbeMessageThreadState::AnalyzerOutput::DecibelCycle::Item{
                            aodc.seq_index++, input_db, output_db
                          }
                        );
                    }
                }
                x.previous_input_db = input_db;
            }
        }
    } break;
    }
}
