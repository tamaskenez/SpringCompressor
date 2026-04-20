#include "CompressorProbeEditor.h"

#include "CommonState.h"
#include "ProcessorInterface.h"

// --- RoleSelectionOverlay ---

RoleSelectionOverlay::RoleSelectionOverlay()
{
    generator_btn.onClick = [this] {
        if (on_role_selected)
            juce::MessageManager::callAsync([this] {
                on_role_selected(Role::Generator);
            });
    };
    probe_btn.onClick = [this] {
        if (on_role_selected)
            juce::MessageManager::callAsync([this] {
                on_role_selected(Role::Probe);
            });
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
  juce::AudioProcessor& p,
  juce::AudioProcessorValueTreeState& apvts,
  ProcessorInterface* processor_interface_arg,
  const CommonState& common_state_arg
)
    : AudioProcessorEditor(p)
    , processor_interface(processor_interface_arg)
    , common_state(common_state_arg)
    , mode(apvts, "mode", choices_for(apvts, "mode"))
{
    decibel_cycle_panel = make_unique<DecibelCyclePanel>(apvts);
    addChildComponent(*decibel_cycle_panel);

    if (!common_state.role) {
        role_overlay = std::make_unique<RoleSelectionOverlay>();
        role_overlay->on_role_selected = [this](Role r) {
            processor_interface->on_role_selected_by_user(r);
        };
        addAndMakeVisible(*role_overlay);
    } else {
        refresh_ui();
    }
    setSize(400, 300);
}

void CompressorProbeEditor::refresh_ui()
{
    if (auto role = common_state.role) {
        role_overlay.reset();

        title_label.setFont(juce::FontOptions(18.0f));
        title_label.setColour(juce::Label::textColourId, juce::Colours::white);
        title_label.setJustificationType(juce::Justification::centred);

        role_label.setFont(juce::FontOptions(13.0f));
        role_label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        role_label.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(title_label);
        addAndMakeVisible(role_label);

        if (common_state.error) {
            error_label.setFont(juce::FontOptions(13.0f));
            error_label.setColour(juce::Label::textColourId, juce::Colours::red);
            error_label.setJustificationType(juce::Justification::left);
            error_label.setText(*common_state.error, juce::dontSendNotification);
            addAndMakeVisible(error_label);
        }

        switch (*role) {
        case Role::Generator:
            refresh_generator_ui();
            break;
        case Role::Probe:
            refresh_probe_ui();
            break;
        }
        resized();
    }
}

void CompressorProbeEditor::refresh_generator_ui()
{
    if (common_state.generator_id)
        title_label.setText("Generator #" + juce::String(*common_state.generator_id), juce::dontSendNotification);
    else
        title_label.setText("Generator", juce::dontSendNotification);

    if (processor_interface->get_generator_status().first == GeneratorStatus::TransmittingId)
        role_label.setText("Connecting to the probe", juce::dontSendNotification);
    else {
        if (auto current_generator_command = processor_interface->get_generator_status().second) {
            role_label.setText(*current_generator_command, juce::dontSendNotification);
        } else {
            role_label.setText({}, juce::dontSendNotification);
        }
    }
}

void CompressorProbeEditor::refresh_probe_ui()
{
    title_label.setText("Probe", juce::dontSendNotification);

    if (!common_state.prepared_to_play) {
        role_label.setText("Start the audio engine to begin", juce::dontSendNotification);
    } else {
        if (common_state.generator_id) {
            role_label.setText(
              "Connected to generator #" + juce::String(*common_state.generator_id), juce::dontSendNotification
            );
        } else {
            role_label.setText("Connecting to the generator", juce::dontSendNotification);
        }
    }

    addAndMakeVisible(mode.combo);
    mode.combo.setEnabled(common_state.generator_id.has_value());

    const bool is_decibel_cycle = (mode.combo.getSelectedItemIndex() == std::to_underlying(Mode::E::DecibelCycle));
    decibel_cycle_panel->setVisible(is_decibel_cycle);
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

    // Error label pinned to the bottom.
    if (common_state.error) {
        area.removeFromBottom(8);
        error_label.setBounds(area.removeFromBottom(40));
    }

    area.removeFromTop(8);

    // Title and role label side by side at the top.
    auto top_row = area.removeFromTop(24);
    title_label.setBounds(top_row.removeFromLeft(top_row.getWidth() / 3));
    role_label.setBounds(top_row);

    if (common_state.role == Role::Probe) {
        area.removeFromTop(4);
        mode.combo.setBounds(area.removeFromTop(24));
        if (decibel_cycle_panel->isVisible()) {
            decibel_cycle_panel->setBounds(area);
        }
    }
}
