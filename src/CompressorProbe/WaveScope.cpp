#include "WaveScope.h"

WaveScope::WaveScope(juce::AudioProcessorValueTreeState& apvts)
    : channels(apvts, "channels", choices_for(apvts, "channels"))
    , y_unit(apvts, "y_unit", choices_for(apvts, "y_unit"))
    , auto_scale_attachment(apvts, "auto_scale", auto_scale_btn)
{
    addAndMakeVisible(channels.combo);
    addAndMakeVisible(y_unit.combo);
    addAndMakeVisible(auto_scale_btn);
}

void WaveScope::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.5f));
    if (waveform_image.isValid()) {
        g.drawImage(waveform_image, waveform_image.getBounds().toFloat());
    }
}

void WaveScope::resized()
{
    auto area = getLocalBounds().reduced(4);

    auto controls = area.removeFromBottom(24);
    area.removeFromBottom(4);

    const int combo_w = 110;
    const int gap = 8;
    channels.combo.setBounds(controls.removeFromLeft(combo_w));
    controls.removeFromLeft(gap);
    y_unit.combo.setBounds(controls.removeFromLeft(combo_w));
    controls.removeFromLeft(gap);
    auto_scale_btn.setBounds(controls.removeFromLeft(100));

    waveform_image =
      juce::Image(juce::Image::ARGB, juce::jmax(1, area.getWidth()), juce::jmax(1, area.getHeight()), true);
}

void WaveScope::update(span<const float> samples_in, span<const float> samples_out)
{
    if (!waveform_image.isValid() || samples_in.empty()) {
        return;
    }

    const bool is_db = (y_unit.combo.getSelectedItemIndex() == 1);
    const bool is_auto = auto_scale_btn.getToggleState();

    auto to_db = [](float s) {
        return 20.0f * ffcast<float>(std::log10(static_cast<double>(std::abs(s)) + 1e-30));
    };

    float y_min, y_max;
    if (is_db) {
        if (is_auto) {
            float peak_db = -200.0f;
            for (auto s : samples_in) {
                peak_db = std::max(peak_db, to_db(s));
            }
            for (auto s : samples_out) {
                peak_db = std::max(peak_db, to_db(s));
            }
            y_min = peak_db - 50.0f;
            y_max = peak_db;
        } else {
            y_min = -100.0f;
            y_max = 0.0f;
        }
    } else {
        if (is_auto) {
            float peak = 0.0f;
            for (auto s : samples_in) {
                peak = std::max(peak, std::abs(s));
            }
            for (auto s : samples_out) {
                peak = std::max(peak, std::abs(s));
            }
            y_min = -peak;
            y_max = peak;
        } else {
            y_min = -1.0f;
            y_max = 1.0f;
        }
    }

    const float y_range = (y_max - y_min);
    const float safe_range = (y_range > 0.0f) ? y_range : 1.0f;

    const int w = waveform_image.getWidth();
    const int h = waveform_image.getHeight();
    const int n = iicast<int>(samples_in.size());

    auto sample_to_y = [&](float s) -> float {
        const float val = is_db ? to_db(s) : s;
        return ifcast<float>(h) * (1.0f - (val - y_min) / safe_range);
    };

    auto make_path = [&](span<const float> samples) {
        juce::Path path;
        for (int i = 0; i < n; ++i) {
            const float x = ifcast<float>(w) * ifcast<float>(i) / ifcast<float>(n);
            const float y = sample_to_y(samples[iicast<size_t>(i)]);
            if (i == 0) {
                path.startNewSubPath(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        return path;
    };

    juce::Graphics g(waveform_image);
    g.fillAll(juce::Colours::black);

    const float zero_y = ifcast<float>(h) * (1.0f - (0.0f - y_min) / safe_range);
    if (zero_y >= 0.0f && zero_y <= ifcast<float>(h)) {
        g.setColour(juce::Colours::darkgrey);
        g.drawHorizontalLine(iround<int>(zero_y), 0.0f, ifcast<float>(w));
    }

    juce::PathStrokeType stroke(1.5f);
    g.setColour(juce::Colours::red);
    g.strokePath(make_path(samples_out), stroke);
    g.setColour(juce::Colours::green);
    g.strokePath(make_path(samples_in), stroke);

    repaint();
}
