#include "AnalyzerScope.h"

AnalyzerScope::AnalyzerScope() = default;

void AnalyzerScope::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.5f));
    if (plot_image.isValid()) {
        g.drawImage(plot_image, plot_image.getBounds().toFloat());
    }
}

void AnalyzerScope::resized()
{
    auto area = getLocalBounds().reduced(4);
    plot_image = juce::Image(juce::Image::ARGB, juce::jmax(1, area.getWidth()), juce::jmax(1, area.getHeight()), true);
}

void AnalyzerScope::update(span<const Point> compressor_curve, size_t last_item_index)
{
    if (!plot_image.isValid() || compressor_curve.empty()) {
        return;
    }

    constexpr double k_db_min = -60.0;
    constexpr double k_db_max = 0.0;
    constexpr double k_db_range = k_db_max - k_db_min;

    const int w = plot_image.getWidth();
    const int h = plot_image.getHeight();

    auto to_px = [&](double input_db, double output_db) -> juce::Point<float> {
        const float x = ifcast<float>(w) * ffcast<float>((input_db - k_db_min) / k_db_range);
        const float y = ifcast<float>(h) * ffcast<float>(1.0 - (output_db - k_db_min) / k_db_range);
        return {x, y};
    };

    auto is_valid = [&](const Point& p) {
        return p.input_db >= k_db_min && p.input_db <= k_db_max && p.output_db >= k_db_min && p.output_db <= k_db_max;
    };

    auto make_path = [&](span<const Point> pts) {
        juce::Path path;
        bool started = false;
        for (auto& p : pts) {
            if (!is_valid(p)) {
                started = false;
                continue;
            }
            auto px = to_px(p.input_db, p.output_db);
            if (!started) {
                path.startNewSubPath(px.x, px.y);
                started = true;
            } else {
                path.lineTo(px.x, px.y);
            }
        }
        return path;
    };

    juce::Graphics g(plot_image);
    g.fillAll(juce::Colours::black);

    // Unity gain reference line (no compression).
    g.setColour(juce::Colours::darkgrey);
    {
        auto p0 = to_px(k_db_min, k_db_min);
        auto p1 = to_px(k_db_max, k_db_max);
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.0f);
    }

    juce::PathStrokeType stroke(1.5f);
    const auto mid = compressor_curve.size() / 2;
    g.setColour(juce::Colours::green);
    g.strokePath(make_path(compressor_curve.subspan(0, mid)), stroke);
    g.setColour(juce::Colours::red);
    g.strokePath(make_path(compressor_curve.subspan(mid)), stroke);

    if (last_item_index < compressor_curve.size()) {
        const auto& last_pt = compressor_curve[last_item_index];
        if (is_valid(last_pt)) {
            constexpr float dot_r = 3.0f;
            auto px = to_px(last_pt.input_db, last_pt.output_db);
            g.setColour(juce::Colours::white);
            g.fillEllipse(px.x - dot_r, px.y - dot_r, 2.0f * dot_r, 2.0f * dot_r);
        }
    }

    repaint();
}
