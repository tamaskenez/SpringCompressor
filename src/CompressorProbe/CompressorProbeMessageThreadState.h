#pragma once

#include "GeneratorStatus.h"
#include "Modes.h"
#include "Role.h"

#include <juce_audio_processors/juce_audio_processors.h>

class CompressorProbeEditor;

// State of CompressorProbeProcessor that lives on the message thread.
// All state that is needed on the message thread must be
// - either atomic
// - or sent asynchronously to the message thread
class CompressorProbeMessageThreadState
{
public:
    juce::AudioProcessorValueTreeState apvts;
    function<CompressorProbeEditor*()> get_active_editor_fn;
    optional<string> generator_command;
    optional<string> error;
    std::vector<float> compressor_input_samples_tail, compressor_output_samples_tail;
    unsigned input_output_next_sample_index_within_period = 0; // Can be used to lock scope to period.
    optional<double> sample_rate;                              // Valid when prepared to play.
    struct AnalyzerOutput {
        struct DecibelCycle {
            struct Item {
                int64_t seq_index;
                double input_db, output_db;
            };
            deque<Item> ascending, descending;
            int64_t seq_index = 0;
            int64_t first_ascending_seq_index = 0, first_descending_seq_index = 0;
            vector<pair<double, double>> input_to_output_db() const;
            vector<pair<double, double>> input_to_gr_db() const;
        } decibel_cycle;
    } ao;

    CompressorProbeMessageThreadState(
      juce::AudioProcessor& processor_to_connect_to, function<CompressorProbeEditor*()> get_active_editor_fn_arg
    );

    void update_channels_on_editor(optional<Role> role, int num_channels, CompressorProbeEditor* e_arg = nullptr);
    void on_ui_refresh_timer_elapsed();
    void update_wave_scope();
    void update_analyzer_scope();
};
