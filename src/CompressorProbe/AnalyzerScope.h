#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <meadow/cppext.h>

class AnalyzerScope : public juce::Component
{
public:
    struct Point {
        double input_db, output_db;
    };

    AnalyzerScope();
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Plot (input_db, output_db) pairs. Ascending sweep drawn in green, descending in red.
    void update(span<const Point> ascending, span<const Point> descending);

    juce::Image plot_image;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScope)
};
