#include "ScopeComponent.h"

#include "meadow/math.h"

#include <meadow/cppext.h>

#include <cmath>
#include <format>

int ScopeComponent::get_plot_w() const
{
    return getWidth() - k_margin_left;
}
int ScopeComponent::get_plot_h() const
{
    return getHeight() - k_margin_bottom - k_margin_top;
}

float ScopeComponent::x_to_px(float x) const
{
    return std::lerp(
      ifcast<float>(k_margin_left), ifcast<float>(k_margin_left + get_plot_w()), (x - min_x) / (max_x - min_x)
    );
}
float ScopeComponent::y_to_py(float y) const
{
    return std::lerp(
      ifcast<float>(get_plot_h() - k_margin_bottom), ifcast<float>(k_margin_top), (y - min_y) / (max_y - min_y)
    );
}

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

    const int plot_h = get_plot_h();

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
            const int px = iround<int>(x_to_px(x));
            if (in_co_range(px, k_margin_left, w)) {
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.drawVerticalLine(px, k_margin_top, ifcast<float>(plot_h - k_margin_bottom));
                g.setColour(juce::Colours::white.withAlpha(0.6f));
                g.drawText(
                  format_val(x), px - 20, plot_h, 40, k_margin_bottom - 1, juce::Justification::centred, false
                );
            }
        }
    }

    // Horizontal lines and y-axis labels
    if (y_step > 0 && y_range > 0) {
        const float first_y = std::ceil(min_y / y_step) * y_step;
        for (float y = first_y; y <= max_y + y_step * 0.5f; y += y_step) {
            const int py = iround<int>(y_to_py(y));
            if (in_co_range(py, 0, plot_h)) {
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

void ScopeComponent::add_plot(span<const AF2> plot, const juce::Colour& color)
{
    if (plot.size() < 2)
        return;

    const int w = image.getWidth();
    const int h = image.getHeight();
    if (w == 0 || h == 0)
        return;

    juce::Graphics g(image);
    g.setColour(color);

    juce::Path path;
    bool started = false;
    for (const auto& pt : plot) {
        const float px = x_to_px(pt[0]);
        const float py = y_to_py(pt[1]);
        if (!started) {
            path.startNewSubPath(px, py);
            started = true;
        } else {
            path.lineTo(px, py);
        }
    }
    g.strokePath(path, juce::PathStrokeType(2.0f));

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
