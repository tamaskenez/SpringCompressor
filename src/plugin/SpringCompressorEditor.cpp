#include "SpringCompressorEditor.h"

SpringCompressorEditor::SpringCompressorEditor(
  juce::AudioProcessor& p, std::atomic<bool>& editor_open_arg, juce::AudioProcessorValueTreeState& apvts
)
    : AudioProcessorEditor(&p)
    , editor_open(editor_open_arg)
    , thresholdAttachment(apvts, "threshold", thresholdSlider)
    , ratioAttachment(apvts, "ratio", ratioSlider)
    , makeupAttachment(apvts, "makeup", makeupSlider)
    , referenceLevelAttachment(apvts, "reference_level", referenceLevelSlider)
    , kneeWidthAttachment(apvts, "knee_width", kneeWidthSlider)
{
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupSlider(thresholdSlider, thresholdLabel, "Threshold");
    setupSlider(ratioSlider, ratioLabel, "Ratio");
    setupSlider(makeupSlider, makeupLabel, "Makeup");
    setupSlider(referenceLevelSlider, referenceLevelLabel, "Ref. Level");
    setupSlider(kneeWidthSlider, kneeWidthLabel, "Knee width");

    addAndMakeVisible(transfer_curve_component);

    setSize(800, 450);
}

void SpringCompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SpringCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    const auto curve_area = area.removeFromBottom(TransferCurveComponent::k_window_size);
    area = area.withTrimmedTop(24);
    const int sliderWidth = area.getWidth() / 5;

    for (auto* slider : {&thresholdSlider, &ratioSlider, &makeupSlider, &referenceLevelSlider, &kneeWidthSlider})
        slider->setBounds(area.removeFromLeft(sliderWidth));

    const int square = std::min(curve_area.getWidth(), curve_area.getHeight());
    transfer_curve_component.setBounds(curve_area.withSizeKeepingCentre(square, square));
}

void SpringCompressorEditor::set_transfer_curve(const TransferCurveState& tcur)
{
    transfer_curve_component.set_transfer_curve(tcur);
}

void SpringCompressorEditor::update_rms_dots(
  int rms_matrix_clock, std::mdspan<int, std::dextents<int, 2>> rms_matrix, double rms_sample_period_sec
)
{
    transfer_curve_component.update_rms_dots(rms_matrix_clock, rms_matrix, rms_sample_period_sec);
}
