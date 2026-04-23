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

void AnalyzerScope::update(span<const Point> ascending, span<const Point> descending)
{
    if (!plot_image.isValid()) {
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

    juce::Graphics g(plot_image);
    g.fillAll(juce::Colours::black);

    // Unity gain reference line (no compression).
    g.setColour(juce::Colours::darkgrey);
    {
        auto p0 = to_px(k_db_min, k_db_min);
        auto p1 = to_px(k_db_max, k_db_max);
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.0f);
    }

    constexpr float dot_r = 2.0f;
    auto draw_points = [&](span<const Point> pts, juce::Colour colour) {
        g.setColour(colour);
        for (auto& p : pts) {
            if (p.input_db >= k_db_min && p.input_db <= k_db_max && p.output_db >= k_db_min
                && p.output_db <= k_db_max) {
                auto px = to_px(p.input_db, p.output_db);
                g.fillEllipse(px.x - dot_r, px.y - dot_r, 2.0f * dot_r, 2.0f * dot_r);
            }
        }
    };

    draw_points(descending, juce::Colours::red);
    draw_points(ascending, juce::Colours::green);

    repaint();
}
