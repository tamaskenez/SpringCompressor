#include "SpringCompressorEditor.h"

SpringCompressorEditor::SpringCompressorEditor(
  juce::AudioProcessor& p, std::atomic<bool>& editor_open_arg, juce::AudioProcessorValueTreeState& apvts
)
    : AudioProcessorEditor(&p)
    , editor_open(editor_open_arg)
    , thresholdAttachment(apvts, "threshold", thresholdSlider)
    , ratioAttachment(apvts, "ratio", ratioSlider)
    , attackAttachment(apvts, "attack", attackSlider)
    , releaseAttachment(apvts, "release", releaseSlider)
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
    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(releaseSlider, releaseLabel, "Release");
    setupSlider(makeupSlider, makeupLabel, "Makeup");
    setupSlider(referenceLevelSlider, referenceLevelLabel, "Ref. Level");
    setupSlider(kneeWidthSlider, kneeWidthLabel, "Knee width");

    addAndMakeVisible(transfer_curve_component);

    auto* gain_filter_param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("gain_filter"));
    for (int i = 0; i < gain_filter_param->choices.size(); ++i)
        gainFilterComboBox.addItem(gain_filter_param->choices[i], i + 1);
    gainFilterAttachment = std::make_unique<ComboBoxAttachment>(apvts, "gain_filter", gainFilterComboBox);

    gainFilterLabel.setText("Gain filter", juce::dontSendNotification);
    gainFilterLabel.setJustificationType(juce::Justification::centred);
    gainFilterLabel.attachToComponent(&gainFilterComboBox, false);
    addAndMakeVisible(gainFilterComboBox);
    addAndMakeVisible(gainFilterLabel);

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
    const int sliderWidth = area.getWidth() / 8;

    for (auto* slider :
         {&thresholdSlider,
          &ratioSlider,
          &attackSlider,
          &releaseSlider,
          &makeupSlider,
          &referenceLevelSlider,
          &kneeWidthSlider})
        slider->setBounds(area.removeFromLeft(sliderWidth));

    auto col = area.removeFromLeft(sliderWidth);
    gainFilterComboBox.setBounds(col.withSizeKeepingCentre(sliderWidth - 4, 24));

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
