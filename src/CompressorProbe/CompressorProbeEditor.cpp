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
    const juce::String text = probe_processor.get_role() == Role::Generator ? "Generator" : "Probe";
    status_label.setText(text, juce::dontSendNotification);
    status_label.setFont(juce::FontOptions(18.0f));
    status_label.setColour(juce::Label::textColourId, juce::Colours::white);
    status_label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(status_label);
}

void CompressorProbeEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CompressorProbeEditor::resized()
{
    if (role_overlay)
        role_overlay->setBounds(getLocalBounds());
    status_label.setBounds(getLocalBounds());
}
