#include "PluginEditor.h"

SelfModEditor::SelfModEditor(SelfModProcessor& p)
    : AudioProcessorEditor(&p)
    , audio_processor(p)
    , lp_freq_attachment(p.apvts, "lp_freq", lp_freq_slider)
    , intensity_attachment(p.apvts, "intensity", intensity_slider)
{
    lp_freq_label.setText("LP Freq", juce::dontSendNotification);
    lp_freq_label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lp_freq_slider);
    addAndMakeVisible(lp_freq_label);

    intensity_label.setText("Intensity", juce::dontSendNotification);
    intensity_label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(intensity_slider);
    addAndMakeVisible(intensity_label);

    setSize(300, 200);
}

void SelfModEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SelfModEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const int label_height = 20;
    const int col_width = bounds.getWidth() / 2;

    auto lp_col = bounds.removeFromLeft(col_width);
    lp_freq_label.setBounds(lp_col.removeFromBottom(label_height));
    lp_freq_slider.setBounds(lp_col);

    intensity_label.setBounds(bounds.removeFromBottom(label_height));
    intensity_slider.setBounds(bounds);
}
