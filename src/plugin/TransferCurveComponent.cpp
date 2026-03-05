#include "TransferCurveComponent.h"

#include <meadow/cppext.h>

#include <cmath>

void TransferCurveComponent::set_transfer_curve(const TransferCurveState& r)
{
    transfer_curve_state = r;
    repaint();
}

juce::Rectangle<float> TransferCurveComponent::plot_area() const
{
    return getLocalBounds().toFloat().reduced(1.f);
}

juce::Point<float> TransferCurveComponent::to_point(float input_db, float output_db) const
{
    const auto area = plot_area();
    const float range = k_db_max - k_db_min;
    const float x = area.getX() + (input_db - k_db_min) / range * area.getWidth();
    const float y = area.getBottom() - (output_db - k_db_min) / range * area.getHeight();
    return {x, y};
}

juce::Path TransferCurveComponent::build_curve_path() const
{
    if (!transfer_curve_state)
        return {};

    const auto& r = *transfer_curve_state;

    juce::Path path;
    auto t = std::min(r.threshold[0] - k_db_min, r.threshold[1] - k_db_min);
    if (t >= 0) {
        path.startNewSubPath(to_point(r.threshold[0] - t, r.threshold[1] - t));
        path.lineTo(to_point(r.threshold[0], r.threshold[1]));
    } else {
        path.startNewSubPath(to_point(r.threshold[0], r.threshold[1]));
    }

    if (!r.knee_ys.empty()) {
        const float knee_start_x = std::floor(r.threshold[0]) + 1;
        for (size_t i = 0; i < r.knee_ys.size(); ++i)
            path.lineTo(to_point(knee_start_x + ifcast<float>(i), r.knee_ys[i]));
    }

    path.lineTo(to_point(r.knee_right[0], r.knee_right[1]));
    t = std::min(k_db_max - r.knee_right[0], (k_db_max - r.knee_right[1]) / r.oo_ratio);
    path.lineTo(to_point(r.knee_right[0] + t, r.knee_right[1] + r.oo_ratio * t));

    return path;
}

void TransferCurveComponent::draw_grid(juce::Graphics& g) const
{
    const auto area = plot_area();
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (float db = k_db_min; db <= k_db_max; db += 10.f) {
        const auto vp = to_point(db, k_db_min);
        g.drawVerticalLine(iround<int>(vp.x), area.getY(), area.getBottom());
        const auto hp = to_point(k_db_min, db);
        g.drawHorizontalLine(iround<int>(hp.y), area.getX(), area.getRight());
    }
}

void TransferCurveComponent::paint(juce::Graphics& g)
{
    const auto area = plot_area();

    g.fillAll(juce::Colour(0xff1a1a1a));
    draw_grid(g);

    // 1:1 reference diagonal
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(juce::Line<float>{to_point(k_db_min, k_db_min), to_point(k_db_max, k_db_max)}, 1.f);

    if (transfer_curve_state) {
        g.setColour(juce::Colour(0xff00d080));
        g.strokePath(build_curve_path(), juce::PathStrokeType(2.f));
    }

    // Border
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(area, 1.f);
}
