#include "ScopeComponent.h"

#include "meadow/math.h"

#include <meadow/cppext.h>

#include <cmath>
#include <format>

int ScopeComponent::get_plot_w() const
{
    return getWidth() - k_margin_left - k_margin_right;
}
int ScopeComponent::get_plot_h() const
{
    return getHeight() - k_margin_bottom - k_margin_top;
}

float ScopeComponent::x_to_px(float x) const
{
    if (log_x) {
        // min_x -> 0
        // max_x -> 1
        return std::lerp(
          ifcast<float>(k_margin_left),
          ifcast<float>(k_margin_left + get_plot_w()),
          (log(x) - log(min_x)) / (log(max_x) - log(min_x))
        );
    } else {
        return std::lerp(
          ifcast<float>(k_margin_left), ifcast<float>(k_margin_left + get_plot_w()), (x - min_x) / (max_x - min_x)
        );
    }
}
float ScopeComponent::y_to_py(float y) const
{
    return std::lerp(
      ifcast<float>(get_plot_h() - k_margin_bottom), ifcast<float>(k_margin_top), (y - min_y) / (max_y - min_y)
    );
}

void ScopeComponent::draw_grid(float min_x_arg, float max_x_arg, float min_y_arg, float max_y_arg, bool log_x_arg)
{
    assert(min_x_arg < max_x_arg);
    assert(min_y_arg < max_y_arg);

    min_x = min_x_arg;
    max_x = max_x_arg;
    min_y = min_y_arg;
    max_y = max_y_arg;
    log_x = log_x_arg;

    const int w = getWidth();
    const int h = getHeight();
    if (w == 0 || h == 0)
        return;

    image = juce::Image(juce::Image::ARGB, w, h, true);
    juce::Graphics g(image);

    g.fillAll(juce::Colours::black);

    const int plot_h = get_plot_h();

    auto format_val = [](float v) -> juce::String {
        if (std::abs(v - std::round(v)) < 1e-4f)
            return juce::String(iround<int>(v));
        return juce::String(std::format("{:.2f}", v));
    };

    constexpr float k_label_font_size = 10.0f;
    g.setFont(k_label_font_size);

    constexpr float k_max_grid_spacing = 100;
    const int k_min_num_gridlines = iround<int>(ifcast<float>(get_plot_w()) / k_max_grid_spacing) + 1;
    vector<float> xs;

    const auto generate_linear_spacing = [k_min_num_gridlines](float min_c, float max_c) {
        vector<float> cs;
        for (int e = ifloor<int>(log10(max_c - min_c)); cs.empty(); --e) {
            const float tenpower = pow(10.0f, ifcast<float>(e));
            for (int d : {5, 2, 1}) {
                const float D = tenpower * d;
                const float after_min_c = ceil(min_c / D) * D;
                const float before_max_c = floor(max_c / D) * D;
                if (after_min_c < before_max_c
                    && iround<int>((before_max_c - after_min_c) / D) + 1 >= k_min_num_gridlines) {
                    const int ds = iround<int>((before_max_c - after_min_c) / D);
                    for (int i = 0; i <= ds; ++i) {
                        cs.push_back(after_min_c + i * D);
                    }
                    break;
                }
            }
        }
        return cs;
    };

    // e <= log10(x)
    // log10(x) < e + 1
    vector<size_t> log_x_label_ixs;
    if (log_x) {
        for (int e = ifloor<int>(log10(max_x - min_x)); xs.empty(); --e) {
            float tenpower = pow(10.0f, ifcast<float>(e));
            const float after_min_x = ceil(min_x / tenpower) * tenpower;
            const float before_max_x = floor(max_x / tenpower) * tenpower;
            if (after_min_x < before_max_x) {
                xs.clear();
                if (abs(minus_round(log10(before_max_x))) < 0.01) {
                    log_x_label_ixs.push_back(0);
                }
                for (float x = before_max_x; x >= min_x; x -= tenpower) {
                    xs.push_back(x);
                    if (iround<int>(x / tenpower) == 1) {
                        tenpower /= 10;
                        log_x_label_ixs.push_back(xs.size() - 1);
                    }
                }
            }
        }
        if (log_x_label_ixs.empty()) {
            log_x_label_ixs.push_back(0);
        }
    } else {
        xs = generate_linear_spacing(min_x, max_x);
    }

    const auto ys = generate_linear_spacing(min_y, max_y);

    // Vertical lines and x-axis labels
    for (size_t i = 0, next_xs_label_ix = 0; i < xs.size(); ++i) {
        const float x = xs[i];
        const int px = iround<int>(x_to_px(x));
        if (in_co_range(px - k_margin_left, 0, get_plot_w())) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawVerticalLine(px, k_margin_top, ifcast<float>(plot_h - k_margin_bottom));
            if (!log_x || (next_xs_label_ix < log_x_label_ixs.size() && i == log_x_label_ixs[next_xs_label_ix])) {
                ++next_xs_label_ix;
                g.setColour(juce::Colours::white.withAlpha(0.6f));
                g.drawText(
                  format_val(x), px - 20, plot_h, 40, k_margin_bottom - 1, juce::Justification::centred, false
                );
            }
        }
    }

    // Horizontal lines and y-axis labels
    for (float y : ys) {
        const int py = iround<int>(y_to_py(y));
        if (in_co_range(py, 0, plot_h)) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawHorizontalLine(py, ifcast<float>(k_margin_left), ifcast<float>(k_margin_left + get_plot_w()));
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText(
              format_val(y),
              0,
              py - iround<int>(k_label_font_size / 2.0f),
              k_margin_left - 2,
              iround<int>(k_label_font_size),
              juce::Justification::centredRight,
              false
            );
        }
    }

    repaint();
}

void ScopeComponent::add_plot(span<const AF2> plot, const juce::Colour& color)
{
    if (plot.size() < 2)
        return;

    const int w = image.getWidth();
    const int h = image.getHeight();
    if (w == 0 || h == 0)
        return;

    juce::Graphics g(image);
    g.setColour(color);

    juce::Path path;
    bool started = false;
    for (const auto& pt : plot) {
        const float px = x_to_px(pt[0]);
        const float py = y_to_py(pt[1]);
        if (!started) {
            path.startNewSubPath(px, py);
            started = true;
        } else {
            path.lineTo(px, py);
        }
    }
    g.strokePath(path, juce::PathStrokeType(2.0f));

    repaint();
}

void ScopeComponent::paint(juce::Graphics& g)
{
    g.drawImageAt(image, 0, 0);
}

void ScopeComponent::resized()
{
    image = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
}
