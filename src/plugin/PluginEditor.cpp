#include "PluginEditor.h"

SpringCompressorEditor::SpringCompressorEditor(SpringCompressorProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
    , thresholdAttachment(p.apvts, "threshold", thresholdSlider)
    , ratioAttachment(p.apvts, "ratio", ratioSlider)
    , attackAttachment(p.apvts, "attack", attackSlider)
    , releaseAttachment(p.apvts, "release", releaseSlider)
    , makeupAttachment(p.apvts, "makeup", makeupSlider)
    , referenceLevelAttachment(p.apvts, "reference_level", referenceLevelSlider)
    , kneeWidthAttachment(p.apvts, "knee_width", kneeWidthSlider)
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

    auto* gain_filter_param = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("gain_filter"));
    for (int i = 0; i < gain_filter_param->choices.size(); ++i)
        gainFilterComboBox.addItem(gain_filter_param->choices[i], i + 1);
    gainFilterAttachment = std::make_unique<ComboBoxAttachment>(p.apvts, "gain_filter", gainFilterComboBox);

    gainFilterLabel.setText("Gain filter", juce::dontSendNotification);
    gainFilterLabel.setJustificationType(juce::Justification::centred);
    gainFilterLabel.attachToComponent(&gainFilterComboBox, false);
    addAndMakeVisible(gainFilterComboBox);
    addAndMakeVisible(gainFilterLabel);

    setSize(800, 220);
}

void SpringCompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SpringCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced(10).withTrimmedTop(24);
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
}
