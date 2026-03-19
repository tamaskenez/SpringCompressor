#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <meadow/array.h>
#include <meadow/cppext.h>

// A component that renders into an internal juce::Image.
// Call draw_grid() to set the coordinate system and paint a black background
// with gray grid lines; the coordinate bounds are then available for use by
// other draw functions.
class ScopeComponent : public juce::Component
{
public:
    // Margins reserved for axis labels.
    static constexpr int k_margin_left = 20;
    static constexpr int k_margin_right = 20;
    static constexpr int k_margin_bottom = 14;
    static constexpr int k_margin_top = 14;

    // Stored coordinate range set by the last draw_grid() call.
    float min_x = 0, max_x = 1, min_y = 0, max_y = 1;
    bool log_x = false;

    juce::Image image;

    void draw_grid(float min_x, float max_x, float min_y, float max_y, bool log_x);

    // Draw a polyline through the given (x, y) points in logical coordinates onto the image.
    // Points outside the plot area are clipped. Calls repaint().
    void add_plot(span<const AF2> plot, const juce::Colour& color);

    void paint(juce::Graphics& g) override;
    void resized() override;

    [[nodiscard]] float x_to_px(float x) const;
    [[nodiscard]] float y_to_py(float y) const;
    [[nodiscard]] int get_plot_w() const;
    [[nodiscard]] int get_plot_h() const;
};
