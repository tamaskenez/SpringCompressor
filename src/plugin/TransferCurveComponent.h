#pragma once

#include "TransferCurve.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

class TransferCurveComponent : public juce::Component
{
public:
    static constexpr int k_db_min = -60;
    static constexpr int k_db_max = 0;
    static constexpr int k_grid_spacing_db = 10;
    static constexpr int k_pixel_per_db = 4;
    static constexpr int k_grid_line_width = 1;
    static constexpr int k_window_size = (k_db_max - k_db_min) * k_pixel_per_db + k_grid_line_width;

    void set_transfer_curve(const TransferCurveState& r);
    void paint(juce::Graphics&) override;
    void resized() override;
    void update_rms_dots(
      int rms_matrix_clock, std::mdspan<int, std::dextents<int, 2>> rms_matrix, double rms_sample_period_sec
    );

private:
    std::optional<TransferCurveState> transfer_curve_state;
    juce::Image rms_overlay;

    [[nodiscard]] juce::Rectangle<float> plot_area() const;
    [[nodiscard]] juce::Point<float> to_point(float input_db, float output_db) const;
    [[nodiscard]] juce::Path build_curve_path() const;
    void draw_grid(juce::Graphics& g) const;
};
