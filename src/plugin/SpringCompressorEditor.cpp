#include "SpringCompressorEditor.h"

SpringCompressorEditor::SpringCompressorEditor(
  juce::AudioProcessor& p, std::atomic<bool>& editor_open_arg, juce::AudioProcessorValueTreeState& apvts
)
    : AudioProcessorEditor(&p)
    , editor_open(editor_open_arg)
    , level_method(apvts, "level_method")
    , levellpf_mode(apvts, "levellpf_mode")
    , levellpf_order(apvts, "levellpf_order")
    , levelmb_order(apvts, "levelmb_order")
    , levelmb_lporder(apvts, "levelmb_lporder")
    , grlp_enable(apvts, "grlp_enable")
    , grlp_order(apvts, "grlp_order")
    , scope_mode(apvts, "scope_mode")
    , thresholdAttachment(apvts, "threshold", thresholdSlider)
    , ratioAttachment(apvts, "ratio", ratioSlider)
    , makeupAttachment(apvts, "makeup", makeupSlider)
    , referenceLevelAttachment(apvts, "reference_level", referenceLevelSlider)
    , kneeWidthAttachment(apvts, "knee_width", kneeWidthSlider)
    , levellpf_attack_attachment(apvts, "levellpf_attack", levellpf_attack)
    , levellpf_release_attachment(apvts, "levellpf_release", levellpf_release)
    , levelmb_freqlo_attachment(apvts, "levelmb_freqlo", levelmb_freqlo)
    , levelmb_freqhi_attachment(apvts, "levelmb_freqhi", levelmb_freqhi)
    , levelmb_peroctave_attachment(apvts, "levelmb_peroctave", levelmb_peroctave)
    , levelmb_lpratio_attachment(apvts, "levelmb_lpratio", levelmb_lpratio)
    , levelmb_minrelease_attachment(apvts, "levelmb_minrelease", levelmb_minrelease)
    , grlp_attack_attachment(apvts, "grlp_attack", grlp_attack)
    , grlp_release_attachment(apvts, "grlp_release", grlp_release)
    , scope_freq_attachment(apvts, "scope_freq", scope_freq)
{
    auto setup_rotary = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };
    setup_rotary(thresholdSlider, thresholdLabel, "Threshold");
    setup_rotary(ratioSlider, ratioLabel, "Ratio");
    setup_rotary(makeupSlider, makeupLabel, "Makeup");
    setup_rotary(referenceLevelSlider, referenceLevelLabel, "Ref. Level");
    setup_rotary(kneeWidthSlider, kneeWidthLabel, "Knee width");

    auto setup_hslider = [this](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
        addAndMakeVisible(s);
    };
    setup_hslider(levellpf_attack);
    setup_hslider(levellpf_release);
    setup_hslider(levelmb_freqlo);
    setup_hslider(levelmb_freqhi);
    setup_hslider(levelmb_peroctave);
    setup_hslider(levelmb_lpratio);
    setup_hslider(levelmb_minrelease);
    setup_hslider(grlp_attack);
    setup_hslider(grlp_release);

    for (auto* rb :
         {&level_method,
          &levellpf_mode,
          &levellpf_order,
          &levelmb_order,
          &levelmb_lporder,
          &grlp_enable,
          &grlp_order,
          &scope_mode})
        addAndMakeVisible(*rb);

    setup_hslider(scope_freq);
    // Display approximate frequency in Hz: scope_freq=0..1 linearly indexes log-spaced freqs 40..12000 Hz.
    scope_freq.textFromValueFunction = [this](double v) -> juce::String {
        if (scope_freq_values.empty()) {
            return "";
        }
        const auto freq_ix =
          std::min(ifloor<size_t>(ifcast<double>(scope_freq_values.size()) * v), scope_freq_values.size() - 1);
        auto f = scope_freq_values[freq_ix];
        if (f < 100) {
            f = round(f * 100) / 100;
        } else if (f < 1000) {
            f = round(f * 10) / 10;
        } else {
            f = round(f);
        }
        return juce::String(format("{}", f));
    };

    auto setup_label = [this](juce::Label& l, const char* text) {
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(l);
    };
    setup_label(level_method_label, "level_method");
    setup_label(levellpf_mode_label, "levellpf_mode");
    setup_label(levellpf_order_label, "levellpf_order");
    setup_label(levellpf_attack_label, "levellpf_attack");
    setup_label(levellpf_release_label, "levellpf_release");
    setup_label(levelmb_freqlo_label, "levelmb_freqlo");
    setup_label(levelmb_freqhi_label, "levelmb_freqhi");
    setup_label(levelmb_peroctave_label, "levelmb_peroctave");
    setup_label(levelmb_order_label, "levelmb_order");
    setup_label(levelmb_lporder_label, "levelmb_lporder");
    setup_label(levelmb_lpratio_label, "levelmb_lpratio");
    setup_label(levelmb_minrelease_label, "levelmb_minrelease");
    setup_label(grlp_enable_label, "grlp_enable");
    setup_label(grlp_order_label, "grlp_order");
    setup_label(grlp_attack_label, "grlp_attack");
    setup_label(grlp_release_label, "grlp_release");
    setup_label(scope_mode_label, "scope_mode");
    setup_label(scope_freq_label, "scope_freq");

    addAndMakeVisible(transfer_curve_component);
    addAndMakeVisible(scope);

    setSize(800, 20 + 120 + 18 * 28);

    draw_scope_grid(0, 100, 0, 2, 10, .1f);
}

void SpringCompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SpringCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Right column: transfer curve on top, image component below
    auto right_col = area.removeFromRight(TransferCurveComponent::k_window_size);
    transfer_curve_component.setBounds(right_col.removeFromTop(TransferCurveComponent::k_window_size));
    scope.setBounds(right_col);

    // Left column: rotary knobs (labels sit above via attachToComponent)
    {
        auto rotary_area = area.removeFromTop(120).withTrimmedTop(24);
        const int w = rotary_area.getWidth() / 5;
        for (auto* s : {&thresholdSlider, &ratioSlider, &makeupSlider, &referenceLevelSlider, &kneeWidthSlider})
            s->setBounds(rotary_area.removeFromLeft(w));
    }

    // New controls: single column, label on the left
    constexpr int row_h = 24;
    constexpr int row_gap = 4;
    constexpr int label_w = 160;

    auto layout = [&](juce::Label& lbl, juce::Component& ctrl) {
        auto row = area.removeFromTop(row_h + row_gap).withHeight(row_h);
        lbl.setBounds(row.removeFromLeft(label_w));
        ctrl.setBounds(row);
    };

    layout(level_method_label, level_method);
    layout(levellpf_mode_label, levellpf_mode);
    layout(levellpf_order_label, levellpf_order);
    layout(levellpf_attack_label, levellpf_attack);
    layout(levellpf_release_label, levellpf_release);
    layout(levelmb_freqlo_label, levelmb_freqlo);
    layout(levelmb_freqhi_label, levelmb_freqhi);
    layout(levelmb_peroctave_label, levelmb_peroctave);
    layout(levelmb_order_label, levelmb_order);
    layout(levelmb_lporder_label, levelmb_lporder);
    layout(levelmb_lpratio_label, levelmb_lpratio);
    layout(levelmb_minrelease_label, levelmb_minrelease);
    layout(grlp_enable_label, grlp_enable);
    layout(grlp_order_label, grlp_order);
    layout(grlp_attack_label, grlp_attack);
    layout(grlp_release_label, grlp_release);
    layout(scope_mode_label, scope_mode);
    layout(scope_freq_label, scope_freq);
}

void SpringCompressorEditor::draw_scope_grid(
  float min_x, float max_x, float min_y, float max_y, float x_step, float y_step
)
{
    scope.draw_grid(min_x, max_x, min_y, max_y, x_step, y_step);
}

void SpringCompressorEditor::add_plot_to_scope(span<AF2> plot, const juce::Colour& color)
{
    scope.add_plot(plot, color);
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
