#include "TransferCurveComponent.h"

#include <meadow/cppext.h>

void TransferCurveComponent::set_transfer_curve(const TransferCurveState& r)
{
    transfer_curve_state = r;
    repaint();
}

juce::Rectangle<float> TransferCurveComponent::plot_area() const
{
    return getLocalBounds().toFloat();
}

juce::Point<float> TransferCurveComponent::to_point(float input_db, float output_db) const
{
    const auto area = plot_area();
    const float x = area.getX() + (input_db - k_db_min) * TransferCurveComponent::k_pixel_per_db;
    const float y = area.getBottom() - (output_db - k_db_min) * TransferCurveComponent::k_pixel_per_db;
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
    constexpr int n = (k_db_max - k_db_min) / k_grid_spacing_db;
    constexpr int first_grid_db = k_db_max - n * k_grid_spacing_db;
    for (int db = first_grid_db; db <= k_db_max; db += k_grid_spacing_db) {
        const auto vp = to_point(ifcast<float>(db), first_grid_db);
        g.drawVerticalLine(iround<int>(vp.x), area.getY(), area.getBottom());
        const auto hp = to_point(first_grid_db, ifcast<float>(db));
        g.drawHorizontalLine(iround<int>(hp.y), area.getX(), area.getRight());
    }
}

void TransferCurveComponent::resized()
{
    rms_overlay = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
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

    g.setOpacity(1.0f);
    g.drawImageAt(rms_overlay, 0, 0);
}

void TransferCurveComponent::update_rms_dots(
  int rms_matrix_clock, std::mdspan<int, std::dextents<int, 2>> rms_matrix, double rms_sample_period_sec
)
{
    // TODO: redraw the rms_overlay from rms_matrix:
    // Each item rms_matrix[x][y] means that there is a sample with
    // `input_rms_db = k_min_db + x` and `output_rms_db = k_min_db + y` and whose age is
    // `rms_matrix_clock - rms_matrix[x][y]`
    // In the overlay, draw a dot with an intensity that is inversely proportional to age.
    // For now, the simple solution would something like this:
    // `intensity = max((max_age - age) / max_age, 0)`
    // The dot should be 2D gaussian bump.
    (void)rms_matrix_clock;
    (void)rms_matrix;
    (void)rms_sample_period_sec;
}
