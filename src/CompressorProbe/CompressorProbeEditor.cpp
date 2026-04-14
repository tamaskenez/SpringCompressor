#include "CompressorProbeEditor.h"

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

CompressorProbeEditor::CompressorProbeEditor(CompressorProbeProcessor& p)
    : AudioProcessorEditor(p)
    , probe_processor(p)
{
    if (probe_processor.get_role() == Role::Unset) {
        role_overlay = std::make_unique<RoleSelectionOverlay>();
        role_overlay->on_role_selected = [this](Role r) {
            role_selected(r);
        };
        addAndMakeVisible(*role_overlay);
    } else {
        build_role_ui();
    }
    setSize(400, 300);
}

void CompressorProbeEditor::role_selected(Role r)
{
    probe_processor.set_role(r);
    role_overlay.reset();
    build_role_ui();
    resized();
}

void CompressorProbeEditor::build_role_ui()
{
    title_label.setFont(juce::FontOptions(18.0f));
    title_label.setColour(juce::Label::textColourId, juce::Colours::white);
    title_label.setJustificationType(juce::Justification::centred);

    mode_label.setFont(juce::FontOptions(13.0f));
    mode_label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    mode_label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(title_label);
    addAndMakeVisible(mode_label);

    if (probe_processor.get_role() == Role::Generator)
        refresh_generator_ui();
    else
        refresh_probe_ui();
    startTimer(100); // 10 Hz refresh
}

void CompressorProbeEditor::timerCallback()
{
    if (probe_processor.get_role() == Role::Generator)
        refresh_generator_ui();
    else
        refresh_probe_ui();
}

void CompressorProbeEditor::refresh_generator_ui()
{
    const int id = probe_processor.get_generator_id();
    if (id >= 0)
        title_label.setText("Generator #" + juce::String(id), juce::dontSendNotification);
    else
        title_label.setText("Generator", juce::dontSendNotification);

    const auto status = probe_processor.get_generator_status();
    if (status == GeneratorStatus::TransmittingId)
        mode_label.setText("Connecting to the probe", juce::dontSendNotification);
    else
        mode_label.setText({}, juce::dontSendNotification);
}

void CompressorProbeEditor::refresh_probe_ui()
{
    title_label.setText("Probe", juce::dontSendNotification);

    if (!probe_processor.is_engine_running()) {
        mode_label.setText("Start the audio engine to begin", juce::dontSendNotification);
    } else {
        const int confirmed_id = probe_processor.get_probe_confirmed_id();
        if (confirmed_id < 0)
            mode_label.setText("Connecting to the generator", juce::dontSendNotification);
        else
            mode_label.setText("Connected to generator #" + juce::String(confirmed_id), juce::dontSendNotification);
    }
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

    auto area = getLocalBounds();
    const int centre_y = area.getCentreY();
    title_label.setBounds(area.withY(centre_y - 22).withHeight(28));
    mode_label.setBounds(area.withY(centre_y + 10).withHeight(20));
}
