#include "TransferCurveComponent.h"

#include <meadow/cppext.h>

#include <climits>

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
    const int w = rms_overlay.getWidth();
    const int h = rms_overlay.getHeight();
    if (w == 0 || h == 0)
        return;

    constexpr double k_max_dot_age_sec = 0.1;
    const float max_age_ticks = ffcast<float>(k_max_dot_age_sec / rms_sample_period_sec);

    constexpr float sigma = ifcast<float>(k_pixel_per_db);
    constexpr float two_sigma_sq = 2.f * sigma * sigma;
    constexpr int radius = 3 * k_pixel_per_db;
    constexpr int ksize = 2 * radius + 1;

    static const auto kernel = []() {
        std::array<float, ksize * ksize> k{};
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx)
                k[ucast((dy + radius) * ksize + (dx + radius))] =
                  std::exp(-ifcast<float>(dx * dx + dy * dy) / two_sigma_sq);
        return k;
    }();

    const int n = rms_matrix.extent(0);
    std::vector<float> buf(ucast(w * h), 0.f);

    for (int mx = 0; mx < n; ++mx) {
        for (int my = 0; my < n; ++my) {
            const int stamp = rms_matrix[mx, my];
            if (stamp == INT_MIN)
                continue;

            const float age = ifcast<float>(rms_matrix_clock - stamp);
            const float intensity = std::max(0.f, (max_age_ticks - age) / max_age_ticks);
            if (intensity <= 0.f)
                continue;

            const int cx = mx * k_pixel_per_db;
            const int cy = h - my * k_pixel_per_db;

            const int dy_min = std::max(-radius, -cy);
            const int dy_max = std::min(radius, h - 1 - cy);
            const int dx_min = std::max(-radius, -cx);
            const int dx_max = std::min(radius, w - 1 - cx);

            for (int dy = dy_min; dy <= dy_max; ++dy)
                for (int dx = dx_min; dx <= dx_max; ++dx)
                    buf[ucast((cy + dy) * w + cx + dx)] +=
                      intensity * kernel[ucast((dy + radius) * ksize + (dx + radius))];
        }
    }

    juce::Image::BitmapData bm(rms_overlay, juce::Image::BitmapData::writeOnly);
    for (int py = 0; py < h; ++py)
        for (int px = 0; px < w; ++px)
            bm.setPixelColour(px, py, juce::Colours::white.withAlpha(std::min(1.f, buf[ucast(py * w + px)])));

    repaint();
}
