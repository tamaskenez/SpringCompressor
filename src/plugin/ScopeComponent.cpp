#include "ScopeComponent.h"

#include <meadow/cppext.h>

#include <cmath>
#include <format>

void ScopeComponent::draw_grid(
  float min_x_arg, float max_x_arg, float min_y_arg, float max_y_arg, float x_step, float y_step
)
{
    min_x = min_x_arg;
    max_x = max_x_arg;
    min_y = min_y_arg;
    max_y = max_y_arg;

    const int w = getWidth();
    const int h = getHeight();
    if (w == 0 || h == 0)
        return;

    image = juce::Image(juce::Image::ARGB, w, h, true);
    juce::Graphics g(image);

    g.fillAll(juce::Colours::black);

    const int plot_w = w - k_margin_left;
    const int plot_h = h - k_margin_bottom;

    auto x_to_px = [&](float x) {
        return k_margin_left + iround<int>((x - min_x) / (max_x - min_x) * ifcast<float>(plot_w - 1));
    };
    auto y_to_py = [&](float y) {
        return iround<int>((1.0f - (y - min_y) / (max_y - min_y)) * ifcast<float>(plot_h - 1));
    };

    auto format_val = [](float v) -> juce::String {
        if (std::abs(v - std::round(v)) < 1e-4f)
            return juce::String(iround<int>(v));
        return juce::String(std::format("{:.2f}", v));
    };

    constexpr float k_label_font_size = 10.0f;
    g.setFont(k_label_font_size);

    const float x_range = max_x - min_x;
    const float y_range = max_y - min_y;

    // Vertical lines and x-axis labels
    if (x_step > 0 && x_range > 0) {
        const float first_x = std::ceil(min_x / x_step) * x_step;
        for (float x = first_x; x <= max_x + x_step * 0.5f; x += x_step) {
            const int px = x_to_px(x);
            if (px >= k_margin_left && px < w) {
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.drawVerticalLine(px, 0.0f, ifcast<float>(plot_h));
                g.setColour(juce::Colours::white.withAlpha(0.6f));
                g.drawText(
                  format_val(x), px - 20, plot_h + 1, 40, k_margin_bottom - 1, juce::Justification::centred, false
                );
            }
        }
    }

    // Horizontal lines and y-axis labels
    if (y_step > 0 && y_range > 0) {
        const float first_y = std::ceil(min_y / y_step) * y_step;
        for (float y = first_y; y <= max_y + y_step * 0.5f; y += y_step) {
            const int py = y_to_py(y);
            if (py >= 0 && py < plot_h) {
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.drawHorizontalLine(py, ifcast<float>(k_margin_left), ifcast<float>(w));
                g.setColour(juce::Colours::white.withAlpha(0.6f));
                g.drawText(
                  format_val(y),
                  0,
                  py - iround<int>(k_label_font_size / 2.0f),
                  k_margin_left - 2,
                  iround<int>(k_label_font_size),
                  juce::Justification::centredRight,
                  false
                );
            }
        }
    }

    repaint();
}

void ScopeComponent::paint(juce::Graphics& g)
{
    g.drawImageAt(image, 0, 0);
}

void ScopeComponent::resized()
{
    image = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
}
