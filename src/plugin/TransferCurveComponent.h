#pragma once

#include "TransferCurve.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

class TransferCurveComponent : public juce::Component
{
public:
    void set_result(const TransferCurveUpdateResult& r);
    void paint(juce::Graphics&) override;

private:
    std::optional<TransferCurveUpdateResult> result;

    static constexpr float k_db_min = -60.f;
    static constexpr float k_db_max = 0.f;

    [[nodiscard]] juce::Rectangle<float> plot_area() const;
    [[nodiscard]] juce::Point<float> to_point(float input_db, float output_db) const;
    [[nodiscard]] juce::Path build_curve_path() const;
    void draw_grid(juce::Graphics& g) const;
};
