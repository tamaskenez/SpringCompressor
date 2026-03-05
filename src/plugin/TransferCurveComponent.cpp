#include "TransferCurveComponent.h"

#include <meadow/cppext.h>

#include <cmath>

void TransferCurveComponent::set_result(const TransferCurveUpdateResult& r)
{
    result = r;
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
    if (!result)
        return {};

    const auto& r = *result;
    const float threshold_x = r.threshold[0];
    const float threshold_y = r.threshold[1];
    const float slope1_offset = threshold_y - threshold_x;

    juce::Path path;
    path.startNewSubPath(to_point(k_db_min, k_db_min + slope1_offset));
    path.lineTo(to_point(threshold_x, threshold_y));

    if (!r.knee_ys.empty()) {
        const float knee_start_x = std::ceil(threshold_x);
        const int knee_size = iicast<int>(r.knee_ys.size());
        for (int i = 0; i < knee_size; ++i)
            path.lineTo(to_point(knee_start_x + ifcast<float>(i), r.knee_ys[sucast(i)]));
    }

    const float x1 = r.knee_right[0];
    const float y1 = r.knee_right[1];
    path.lineTo(to_point(k_db_max, y1 + r.oo_ratio * (k_db_max - x1)));

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

    // Border
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(area, 1.f);

    if (!result)
        return;

    g.setColour(juce::Colour(0xff00d080));
    g.strokePath(build_curve_path(), juce::PathStrokeType(2.f));
}
