#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "JuceTimer.h"
#include "TransferCurveComponent.h"

class RadioButtonGroup : public juce::Component
{
public:
    RadioButtonGroup(juce::AudioProcessorValueTreeState& apvts, const juce::String& param_id)
        : attachment(*apvts.getParameter(param_id), [this](float v) {
            const int idx = juce::roundToInt(v);
            for (int i = 0; i < static_cast<int>(buttons.size()); ++i)
                buttons[static_cast<size_t>(i)]->setToggleState(i == idx, juce::dontSendNotification);
        })
    {
        static int next_group_id = 1000;
        const int group_id = next_group_id++;
        auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(param_id));
        for (const auto& choice : param->choices) {
            auto& btn = *buttons.emplace_back(std::make_unique<juce::TextButton>(choice));
            btn.setRadioGroupId(group_id);
            btn.setClickingTogglesState(true);
            addAndMakeVisible(btn);
        }
        for (int i = 0; i < static_cast<int>(buttons.size()); ++i) {
            buttons[static_cast<size_t>(i)]->onClick = [this, i]() {
                if (buttons[static_cast<size_t>(i)]->getToggleState())
                    attachment.setValueAsCompleteGesture(static_cast<float>(i));
            };
        }
        attachment.sendInitialUpdate();
    }

    void resized() override
    {
        const int w = getWidth() / static_cast<int>(buttons.size());
        for (int i = 0; i < static_cast<int>(buttons.size()); ++i)
            buttons[static_cast<size_t>(i)]->setBounds(i * w, 0, w, getHeight());
    }

private:
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    juce::ParameterAttachment attachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RadioButtonGroup)
};

class SpringCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpringCompressorEditor(
      juce::AudioProcessor&, std::atomic<bool>& editor_open_arg, juce::AudioProcessorValueTreeState& apvts
    );
    ~SpringCompressorEditor() override
    {
        editor_open.store(false);
    }

    void paint(juce::Graphics&) override;
    void resized() override;

    void set_transfer_curve(const TransferCurveState& tcur);
    void update_rms_dots(
      int rms_matrix_clock, std::mdspan<int, std::dextents<int, 2>> rms_matrix, double rms_sample_period_sec
    );

private:
    std::atomic<bool>& editor_open;

    // Existing rotary sliders
    juce::Slider thresholdSlider, ratioSlider, makeupSlider, referenceLevelSlider, kneeWidthSlider;
    juce::Label thresholdLabel, ratioLabel, makeupLabel, referenceLevelLabel, kneeWidthLabel;

    // New sliders (declaration must precede their SliderAttachments below)
    juce::Slider levellpf_attack, levellpf_release, levelmb_freqlo, levelmb_freqhi, levelmb_peroctave, levelmb_lpratio,
      levelmb_minrelease, grlp_attack, grlp_release;

    // Radio button groups
    RadioButtonGroup level_method, levellpf_mode, levellpf_order, levelmb_order, levelmb_lporder, grlp_enable,
      grlp_order;

    // Labels for new controls
    juce::Label level_method_label, levellpf_mode_label, levellpf_order_label, levellpf_attack_label,
      levellpf_release_label, levelmb_freqlo_label, levelmb_freqhi_label, levelmb_peroctave_label, levelmb_order_label,
      levelmb_lporder_label, levelmb_lpratio_label, levelmb_minrelease_label, grlp_enable_label, grlp_order_label,
      grlp_attack_label, grlp_release_label;

    TransferCurveComponent transfer_curve_component;
    juce::ImageComponent image_component;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment thresholdAttachment, ratioAttachment, makeupAttachment, referenceLevelAttachment,
      kneeWidthAttachment;
    SliderAttachment levellpf_attack_attachment, levellpf_release_attachment, levelmb_freqlo_attachment,
      levelmb_freqhi_attachment, levelmb_peroctave_attachment, levelmb_lpratio_attachment,
      levelmb_minrelease_attachment, grlp_attack_attachment, grlp_release_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringCompressorEditor)
};
