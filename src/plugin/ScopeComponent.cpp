#include "ScopeComponent.h"

#include <meadow/cppext.h>

#include <cmath>

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
    g.setColour(juce::Colours::white.withAlpha(0.2f));

    const float x_range = max_x - min_x;
    const float y_range = max_y - min_y;

    // Vertical lines at multiples of x_step
    if (x_step > 0 && x_range > 0) {
        const float first_x = std::ceil(min_x / x_step) * x_step;
        for (float x = first_x; x <= max_x + x_step * 0.5f; x += x_step) {
            const int px = iround<int>((x - min_x) / x_range * ifcast<float>(w - 1));
            if (px >= 0 && px < w)
                g.drawVerticalLine(px, 0.0f, ifcast<float>(h));
        }
    }

    // Horizontal lines at multiples of y_step (y increases upward, pixel y downward)
    if (y_step > 0 && y_range > 0) {
        const float first_y = std::ceil(min_y / y_step) * y_step;
        for (float y = first_y; y <= max_y + y_step * 0.5f; y += y_step) {
            const int py = iround<int>((1.0f - (y - min_y) / y_range) * ifcast<float>(h - 1));
            if (py >= 0 && py < h)
                g.drawHorizontalLine(py, 0.0f, ifcast<float>(w));
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
