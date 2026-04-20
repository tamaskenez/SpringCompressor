#include "DecibelCyclePanel.h"

DecibelCyclePanel::DecibelCyclePanel(juce::AudioProcessorValueTreeState& apvts)
    : freq_attachment(apvts, "steady_curve_freq", freq_slider)
    , min_dbfs_attachment(apvts, "steady_curve_min_dbfs", min_dbfs_slider)
    , max_dbfs_attachment(apvts, "steady_curve_max_dbfs", max_dbfs_slider)
    , waveform(apvts, "steady_curve_waveform", choices_for(apvts, "steady_curve_waveform"))
    , level_method(apvts, "steady_curve_level_method", choices_for(apvts, "steady_curve_level_method"))
    , length(apvts, "steady_curve_length", choices_for(apvts, "steady_curve_length"))
{
    auto setup_label = [](juce::Label& lbl, const juce::String& text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions(13.0f));
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        lbl.setJustificationType(juce::Justification::centredRight);
    };
    setup_label(freq_label, "Freq (Hz)");
    setup_label(waveform_label, "Waveform");
    setup_label(level_method_label, "Level method");
    setup_label(min_dbfs_label, "Min dBFS");
    setup_label(max_dbfs_label, "Max dBFS");
    setup_label(length_label, "Length");

    for (auto* s : {&freq_slider, &min_dbfs_slider, &max_dbfs_slider}) {
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    }

    for (auto* c : std::initializer_list<juce::Component*>{
           &freq_label,
           &freq_slider,
           &waveform_label,
           &waveform.combo,
           &level_method_label,
           &level_method.combo,
           &min_dbfs_label,
           &min_dbfs_slider,
           &max_dbfs_label,
           &max_dbfs_slider,
           &length_label,
           &length.combo
         }) {
        addAndMakeVisible(c);
    }
}

void DecibelCyclePanel::resized()
{
    const int label_w = 110;
    const int ctrl_gap = 8;
    const int row_h = 24;

    auto area = getLocalBounds();

    auto layout_row = [&](juce::Label& lbl, juce::Component& ctrl) {
        area.removeFromTop(4);
        auto row = area.removeFromTop(row_h);
        lbl.setBounds(row.removeFromLeft(label_w));
        row.removeFromLeft(ctrl_gap);
        ctrl.setBounds(row);
    };

    layout_row(freq_label, freq_slider);
    layout_row(waveform_label, waveform.combo);
    layout_row(level_method_label, level_method.combo);
    layout_row(min_dbfs_label, min_dbfs_slider);
    layout_row(max_dbfs_label, max_dbfs_slider);
    layout_row(length_label, length.combo);
}
