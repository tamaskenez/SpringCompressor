#include "CompressorProbeMessageThreadState.h"

#include "AnalyzerScope.h"
#include "CompressorProbeEditor.h"
#include "config.h"
#include "juce_util/misc.h"

#include <meadow/cppext.h>
#include <meadow/enum.h>

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    return {
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode", get_labels_for_enum_in_StringArray<Mode::E>(), 0
      ),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_freq", 1}, "Freq", 20, 10000, 1000),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_waveform", 1},
        "Waveform",
        juce::StringArray{"sine", "square", "hi-crest-square"},
        0
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_level_ref", 1}, "Level Ref", juce::StringArray{"Peak", "RMS"}, 0
      ),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_min_dbfs", 1}, "Min dBFS", -60, 0, -60),
      make_unique<juce::AudioParameterInt>(juce::ParameterID{"steady_curve_max_dbfs", 1}, "Max dBFS", -50, 0, -6),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"steady_curve_length", 1},
        "Length",
        juce::StringArray{"500 ms", "1000 ms", "2000 ms", "4000 ms", "8000 ms"},
        2
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"channels", 1}, "Channels", juce::StringArray{"left", "right", "(left+right)/2"}, 0
      ),
      make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"y_unit", 1}, "Y Unit", juce::StringArray{"linear", "dB"}, 0
      ),
      make_unique<juce::AudioParameterBool>(juce::ParameterID{"auto_scale", 1}, "Auto Scale", true),
    };
}
} // namespace

CompressorProbeMessageThreadState::CompressorProbeMessageThreadState(
  juce::AudioProcessor& processor_to_connect_to, function<CompressorProbeEditor*()> get_active_editor_fn_arg
)
    : apvts(processor_to_connect_to, nullptr, "apvts", createParameterLayout())
    , get_active_editor_fn(MOVE(get_active_editor_fn_arg))
{
}

void CompressorProbeMessageThreadState::update_channels_on_editor(
  optional<Role> role, int num_channels, CompressorProbeEditor* e_arg
)
{
    if (role == Role::Probe) {
        if (auto* e = e_arg ? e_arg : get_active_editor_fn()) {
            e->enable_channels(num_channels);
        }
    }
}

void CompressorProbeMessageThreadState::on_ui_refresh_timer_elapsed()
{
    if (auto* e = get_active_editor_fn()) {
        e->refresh_ui();
    }
}

void CompressorProbeMessageThreadState::update_wave_scope()
{
    if (auto* e = get_active_editor_fn(); e && sample_rate) {
        const auto num_wave_scope_samples = iround<int>(k_wave_scope_duration_sec * *sample_rate);
        // Pick a section of compressor_input_samples_tail and compressor_output_samples_tail such that the last sample
        // has always the same phase.
        const auto M = compressor_input_samples_tail.size();
        CHECK(compressor_output_samples_tail.size() == M);
        const auto te = std::max<ptrdiff_t>(0, signed_subtract(M, input_output_next_sample_index_within_period));
        const auto tb = std::max<ptrdiff_t>(0, te - num_wave_scope_samples);
        e->wave_scope.update(
          span(compressor_input_samples_tail.data() + tb, sucast(te - tb)),
          span(compressor_output_samples_tail.data() + tb, sucast(te - tb))
        );
    }
}
void CompressorProbeMessageThreadState::update_analyzer_scope()
{
    if (auto* e = get_active_editor_fn()) {
        auto& dc = ao.decibel_cycle;
        static_assert(sizeof(AnalyzerScope::Point) == sizeof(AnalyzerOutput::DecibelCycle::Item));
        e->analyzer_scope.update(
          span<const AnalyzerScope::Point>(
            reinterpret_cast<const AnalyzerScope::Point*>(dc.compressor_curve.data()), dc.compressor_curve.size()
          ),
          dc.last_item_index
        );
    }
}
