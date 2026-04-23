#include "CompressorProbeEditor.h"

#include "CompressorProbeMessageThreadState.h"
#include "CompressorProbeThreadSafeState.h"
#include "ProcessorInterface.h"
#include "RoleInterface.h"

// --- RoleSelectionOverlay ---

RoleSelectionOverlay::RoleSelectionOverlay()
{
    generator_btn.onClick = [this] {
        if (on_role_selected) {
            on_role_selected(Role::Generator);
        }
    };
    probe_btn.onClick = [this] {
        if (on_role_selected) {
            on_role_selected(Role::Probe);
        }
    };

    auto setup_hint = [](juce::Label& lbl, const juce::String& text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions(12.0f));
        lbl.setColour(juce::Label::textColourId, juce::Colours::grey);
        lbl.setJustificationType(juce::Justification::centred);
    };
    setup_hint(generator_hint, "place before the compressor");
    setup_hint(probe_hint, "place after the compressor");

    addAndMakeVisible(generator_btn);
    addAndMakeVisible(probe_btn);
    addAndMakeVisible(generator_hint);
    addAndMakeVisible(probe_hint);
}

void RoleSelectionOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e2e));

    auto area = getLocalBounds();

    g.setColour(juce::Colours::white);
    g.setFont(22.0f);
    g.drawFittedText("CompressorProbe", area.removeFromTop(64).withTrimmedTop(16), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawFittedText(
      "\n\nYou need two instances of CompressorProbe:\nthe generator and the actual probe\n\nPlease, turn down "
      "the volume and select the role of this instance:\n",
      area.removeFromTop(28),
      juce::Justification::centred,
      1
    );
}

void RoleSelectionOverlay::resized()
{
    const int margin = 40;
    const int gap = 16;
    const int btn_h = 52;
    const int hint_h = 20;

    auto area = getLocalBounds().reduced(margin, 0);
    area.removeFromTop(92 + 48 + 16); // title + instructions + spacing

    const int col_w = (area.getWidth() - gap) / 2;

    auto left = area.removeFromLeft(col_w);
    area.removeFromLeft(gap);
    auto right = area.removeFromLeft(col_w);

    generator_btn.setBounds(left.removeFromTop(btn_h));
    left.removeFromTop(4);
    generator_hint.setBounds(left.removeFromTop(hint_h));

    probe_btn.setBounds(right.removeFromTop(btn_h));
    right.removeFromTop(4);
    probe_hint.setBounds(right.removeFromTop(hint_h));
}

// --- CompressorProbeEditor ---

CompressorProbeEditor::CompressorProbeEditor(
  juce::AudioProcessor& processor_arg,
  CompressorProbeThreadSafeState& ts_state_arg,
  CompressorProbeMessageThreadState& state_mt_arg,
  ProcessorInterface* processor_interface_arg
)
    : AudioProcessorEditor(processor_arg)
    , ts_state(ts_state_arg)
    , state_mt(state_mt_arg)
    , processor_interface(processor_interface_arg)
    , mode(state_mt.apvts, "mode", choices_for(state_mt.apvts, "mode"))
    , wave_scope(state_mt.apvts)
    , decibel_cycle_panel(make_unique<DecibelCyclePanel>(state_mt.apvts))
    , envelope_filter_panel(make_unique<EnvelopeFilterPanel>(state_mt.apvts))
{
    addChildComponent(*decibel_cycle_panel);
    addChildComponent(*envelope_filter_panel);
    addChildComponent(wave_scope);
    addChildComponent(analyzer_scope);

    if (!ts_state.role_impl.load()) {
        role_overlay = std::make_unique<RoleSelectionOverlay>();
        role_overlay->on_role_selected = [this](Role role) {
            processor_interface->on_role_selected_by_user_mt(role);
        };
        addAndMakeVisible(*role_overlay);
    } else {
        refresh_ui();
    }
    setSize(400, 300);
}

void CompressorProbeEditor::enable_channels(bool b)
{
    wave_scope.channels.combo.setEnabled(b);
}

void CompressorProbeEditor::refresh_ui()
{
    if (auto* role_impl = ts_state.role_impl.load()) {
        role_overlay.reset();

        title_label.setFont(juce::FontOptions(18.0f));
        title_label.setColour(juce::Label::textColourId, juce::Colours::white);
        title_label.setJustificationType(juce::Justification::centred);

        role_label.setFont(juce::FontOptions(13.0f));
        role_label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        role_label.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(title_label);
        addAndMakeVisible(role_label);

        if (state_mt.error) {
            error_label.setFont(juce::FontOptions(13.0f));
            error_label.setColour(juce::Label::textColourId, juce::Colours::red);
            error_label.setJustificationType(juce::Justification::left);
            error_label.setText(*state_mt.error, juce::dontSendNotification);
            addAndMakeVisible(error_label);
        }

        switch (role_impl->get_role()) {
        case Role::Generator:
            refresh_generator_ui();
            setSize(400, 300);
            break;
        case Role::Probe:
            refresh_probe_ui();
            setSize(800, 608);
            break;
        }
        resized();
    }
}

void CompressorProbeEditor::refresh_generator_ui()
{
    const auto generator_id = ts_state.generator_id_in_generator.load();
    if (generator_id != k_invalid_generator_id)
        title_label.setText("Generator #" + juce::String(generator_id), juce::dontSendNotification);
    else
        title_label.setText("Generator", juce::dontSendNotification);

    if (ts_state.generator_status.load() == GeneratorStatus::TransmittingId)
        role_label.setText("Connecting to the probe", juce::dontSendNotification);
    else {
        if (state_mt.generator_command) {
            role_label.setText(*state_mt.generator_command, juce::dontSendNotification);
        } else {
            role_label.setText({}, juce::dontSendNotification);
        }
    }
}

void CompressorProbeEditor::refresh_probe_ui()
{
    title_label.setText("Probe", juce::dontSendNotification);

    if (!ts_state.prepared_to_play) {
        role_label.setText("Start the audio engine to begin", juce::dontSendNotification);
    } else {
        if (auto generator_id = ts_state.generator_id_in_probe.load(); generator_id != k_invalid_generator_id) {
            role_label.setText("Connected to generator #" + juce::String(generator_id), juce::dontSendNotification);
        } else {
            role_label.setText("Connecting to the generator", juce::dontSendNotification);
        }
    }

    addAndMakeVisible(mode.combo);
    mode.combo.setEnabled(ts_state.generator_id_in_probe.load() != k_invalid_generator_id);

    const bool is_decibel_cycle = (mode.combo.getSelectedItemIndex() == std::to_underlying(Mode::E::DecibelCycle));
    const bool is_envelope_filter = (mode.combo.getSelectedItemIndex() == std::to_underlying(Mode::E::EnvelopeFilter));
    decibel_cycle_panel->setVisible(is_decibel_cycle);
    envelope_filter_panel->setVisible(is_envelope_filter);
    wave_scope.setVisible(true);
    analyzer_scope.setVisible(true);
}

void CompressorProbeEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CompressorProbeEditor::resized()
{
    if (role_overlay) {
        role_overlay->setBounds(getLocalBounds());
        return;
    }

    auto area = getLocalBounds().reduced(8, 0);

    if (ts_state.get_role() == Role::Generator && state_mt.error) {
        area.removeFromBottom(8);
        error_label.setBounds(area.removeFromBottom(40));
    }

    area.removeFromTop(8);
    auto top_row = area.removeFromTop(24);
    title_label.setBounds(top_row.removeFromLeft(top_row.getWidth() / 3));
    role_label.setBounds(top_row);

    if (ts_state.get_role() == Role::Probe) {
        if (state_mt.error) {
            area.removeFromTop(4);
            error_label.setBounds(area.removeFromTop(20));
        }

        auto left = area.removeFromLeft(area.getWidth() / 2);
        area.removeFromLeft(8);

        left.removeFromTop(4);
        mode.combo.setBounds(left.removeFromTop(24));
        if (decibel_cycle_panel->isVisible()) {
            decibel_cycle_panel->setBounds(left);
        }
        if (envelope_filter_panel->isVisible()) {
            envelope_filter_panel->setBounds(left);
        }

        wave_scope.setBounds(area.removeFromTop(area.getWidth() / 2));
        analyzer_scope.setBounds(area);
    }
}
