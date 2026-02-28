#include "PluginEditor.h"

SelfModEditor::SelfModEditor(SelfModProcessor& p)
    : AudioProcessorEditor(&p)
{
    setSize(400, 200);
}

void SelfModEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("SelfMod", getLocalBounds(), juce::Justification::centred, 1);
}

void SelfModEditor::resized() {}
