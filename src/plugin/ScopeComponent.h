#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// A component that renders into an internal juce::Image.
// Call draw_grid() to set the coordinate system and paint a black background
// with gray grid lines; the coordinate bounds are then available for use by
// other draw functions.
class ScopeComponent : public juce::Component
{
public:
    // Margins reserved for axis labels.
    static constexpr int k_margin_left = 30;
    static constexpr int k_margin_bottom = 14;

    void draw_grid(float min_x, float max_x, float min_y, float max_y, float x_step, float y_step);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Stored coordinate range set by the last draw_grid() call.
    float min_x = 0, max_x = 1, min_y = 0, max_y = 1;

    juce::Image image;
};
