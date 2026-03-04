#pragma once

#include <cmath>
#include <optional>

enum class TransferCurveNormalizer {
    makeup_gain,
    reference_level
};

struct TransferCurvePars {
    float threshold_db = -30;
    float ratio = 4;         // Must be >= 1.
    float knee_width_db = 0; // Must be >= 0.
    TransferCurveNormalizer normalizer = TransferCurveNormalizer::makeup_gain;
    float normalizer_db = 0;
    bool operator==(const TransferCurvePars&) const = default;
};

struct TransferCurveUpdateResult {
    TransferCurveNormalizer normalizer;
    float normalizer_db;
};

// Transfer curve of dynamic compressor which maps input levels to output levels.
// Describes a 3-part function, which, in addition to simply adding a constant (makeup_gain) to the input level.
// - when the input level is less than the threshold the curve is a straight line, no change (except adding makeup_gain)
// - from threshold to knee_width: an exponential curve leading from dout/din = 1 to dout/din = 1/ratio
// - when the input level is above the threshold + knee_width: straight line with dout/din = 1/ratio
class TransferCurve
{
public:
    TransferCurve();
    std::optional<TransferCurveUpdateResult> set(const TransferCurvePars& p);
    [[nodiscard]] float gain_db_for_input_db(float input_db) const;

private:
    TransferCurvePars pars;
    double makeup_gain_db = 0;
    double knee_A = NAN, knee_B = NAN;
    double output_db_right_of_knee = NAN;

    TransferCurveUpdateResult update();
};
