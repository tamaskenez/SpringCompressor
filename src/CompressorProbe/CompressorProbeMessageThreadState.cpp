#include "CompressorProbeMessageThreadState.h"

#include "CompressorProbeEditor.h"
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
        juce::ParameterID{"steady_curve_level_method", 1}, "Level Method", juce::StringArray{"peak", "rms"}, 0
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

Mode::E CompressorProbeMessageThreadState::get_mode() const
{
    auto* rap = apvts.getParameter("mode");
    auto e = enum_cast_from_float<Mode::E>(rap->convertFrom0to1(rap->getValue()));
    CHECK(e);
    return *e;
}
