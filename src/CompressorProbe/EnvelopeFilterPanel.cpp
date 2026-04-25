#include "RatioByFreqPanel.h"

RatioByFreqPanel::RatioByFreqPanel(juce::AudioProcessorValueTreeState& apvts)
    : carrier_freq_attachment(apvts, "ratio_by_freq_carrier_freq", carrier_freq_slider)
    , max_carrier_amp_attachment(apvts, "ratio_by_freq_max_carrier_db", max_carrier_amp_slider)
    , min_mod_freq_attachment(apvts, "ratio_by_freq_min_mod_freq", min_mod_freq_slider)
    , max_mod_freq_attachment(apvts, "ratio_by_freq_max_mod_freq", max_mod_freq_slider)
    , mod_amp_attachment(apvts, "ratio_by_freq_mod_amp_db", mod_amp_slider)
    , length(apvts, "ratio_by_freq_length", choices_for(apvts, "ratio_by_freq_length"))
{
    auto setup_label = [](juce::Label& lbl, const juce::String& text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions(13.0f));
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        lbl.setJustificationType(juce::Justification::centredRight);
    };
    setup_label(carrier_freq_label, "Carrier (Hz)");
    setup_label(max_carrier_amp_label, "Max Amp (dBFS)");
    setup_label(min_mod_freq_label, "Min Mod (Hz)");
    setup_label(max_mod_freq_label, "Max Mod (Hz)");
    setup_label(mod_amp_label, "Mod Amp (dB)");
    setup_label(length_label, "Length");

    for (auto* s :
         {&carrier_freq_slider, &max_carrier_amp_slider, &min_mod_freq_slider, &max_mod_freq_slider, &mod_amp_slider}) {
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    }

    for (auto* c : std::initializer_list<juce::Component*>{
           &carrier_freq_label,
           &carrier_freq_slider,
           &max_carrier_amp_label,
           &max_carrier_amp_slider,
           &min_mod_freq_label,
           &min_mod_freq_slider,
           &max_mod_freq_label,
           &max_mod_freq_slider,
           &mod_amp_label,
           &mod_amp_slider,
           &length_label,
           &length.combo
         }) {
        addAndMakeVisible(c);
    }
}

void RatioByFreqPanel::resized()
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

    layout_row(carrier_freq_label, carrier_freq_slider);
    layout_row(max_carrier_amp_label, max_carrier_amp_slider);
    layout_row(min_mod_freq_label, min_mod_freq_slider);
    layout_row(max_mod_freq_label, max_mod_freq_slider);
    layout_row(mod_amp_label, mod_amp_slider);
    layout_row(length_label, length.combo);
}
