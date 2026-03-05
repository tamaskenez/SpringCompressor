#pragma once

#include "meadow/array.h"
#include "meadow/cppext.h"
#include "meadow/inplace_vector.h"

#include <cmath>
#include <optional>

// The user can normalize the gain either by specifying an explicit makeup gain or by specifying a reference input level
// which results in identical output level. It's the fixpoint of the curve.
enum class TransferCurveNormalizer {
    makeup_gain,
    reference_level
};

struct TransferCurvePars {
    float threshold_db = -30;
    float ratio = 4;         // Must be >= 1.
    float knee_width_db = 0; // Must be >= 0.
    TransferCurveNormalizer normalizer =
      TransferCurveNormalizer::makeup_gain; // Tells what the `normalizer_db` field means.
    float normalizer_db = 0;

    bool sanitize();
};

const int k_max_knee_width_db = 40;

struct TransferCurveState {
    float makeup_gain_db;
    float reference_level_db;

    // Describe the transfer curve:
    AF2 threshold; // dy/dx = 1 diagonal line until (threshold[0], threshold[1])
    // The knee is a curve, described by y coordinates for integer x coordinates.
    // The first point is (floor(threshold[0]) + 1, knee_ys[0]),
    // then (floor(threshold[0]) + 1, knee_ys[1]), ..., etc...
    std::inplace_vector<float, k_max_knee_width_db> knee_ys;
    AF2 knee_right; // A line starts at (knee_right[0], knee_right[1])
    float oo_ratio; // with dy/dx = oo_ratio
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

    // Return nullopt if there were no changes.
    // Otherwise, when the makeup gain was specified in p, return the corresponding, computed reference level.
    // If the reference level was specified in pars, return the corresponding, computed makeup gain.
    TransferCurveState set(const TransferCurvePars& pars);

    [[nodiscard]] double gain_db_for_input_db(double input_db) const;

    TransferCurveState get_state() const;

private:
    TransferCurvePars pars;
    double makeup_gain_db = 0;
    double reference_level_db = 0;
    double knee_A = NAN, knee_B = NAN;
    double output_db_without_makeup_right_of_knee = NAN;

    TransferCurveState update();
};
