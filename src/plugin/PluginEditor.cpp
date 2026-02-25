#include "PluginEditor.h"

SpringCompressorEditor::SpringCompressorEditor(SpringCompressorProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      thresholdAttachment(p.apvts, "threshold", thresholdSlider),
      ratioAttachment(p.apvts, "ratio", ratioSlider),
      attackAttachment(p.apvts, "attack", attackSlider),
      releaseAttachment(p.apvts, "release", releaseSlider),
      makeupAttachment(p.apvts, "makeup", makeupSlider)
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

    setSize(500, 220);
}

void SpringCompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SpringCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced(10).withTrimmedTop(24);
    const int sliderWidth = area.getWidth() / 5;

    for (auto* slider : {&thresholdSlider, &ratioSlider, &attackSlider, &releaseSlider, &makeupSlider})
        slider->setBounds(area.removeFromLeft(sliderWidth));
}