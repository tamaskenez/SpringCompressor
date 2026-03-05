#include "TransferCurve.h"

#include <meadow/cppext.h>

#include <cassert>

bool TransferCurvePars::sanitize()
{
    bool changed = false;
    if (ratio < 1) {
        ratio = 1;
        changed = true;
    }
    auto original_knee_width_db = knee_width_db;
    knee_width_db = std::clamp<float>(knee_width_db, 0, k_max_knee_width_db);
    if (abs(knee_width_db - original_knee_width_db) > 1e-3) {
        changed = true;
    }
    return changed;
}

bool TransferCurvePars::semantically_equals(const TransferCurvePars& y) const
{
    return threshold_db == y.threshold_db && std::max(1.0f, ratio) == std::max(1.0f, y.ratio)
        && (ratio <= 1.0f ? 0.0f : std::max(0.0f, knee_width_db))
             == (y.ratio <= 1.0f ? 0.0f : std::max(0.0f, y.knee_width_db))
        && normalizer == y.normalizer && normalizer_db == y.normalizer_db;
}

TransferCurve::TransferCurve()
{
    update();
}

std::optional<TransferCurveUpdateResult> TransferCurve::set(const TransferCurvePars& p)
{
    if (pars.semantically_equals(p)) {
        return std::nullopt;
    }
    pars = p;
    bool sanitize_changed = pars.sanitize();
    assert(!sanitize_changed);
    if (pars.ratio == 1) {
        pars.knee_width_db = 0;
    }
    return update();
}
double TransferCurve::gain_db_for_input_db(double input_db) const
{
    double result = 0;
    if (input_db <= pars.threshold_db) {
        NOP;
    } else if (input_db <= pars.threshold_db + pars.knee_width_db) {
        result = knee_A * exp(knee_B * (input_db - pars.threshold_db)) + pars.threshold_db - knee_A - input_db;
    } else {
        result = (input_db - pars.threshold_db - pars.knee_width_db) / pars.ratio
               + output_db_without_makeup_right_of_knee - input_db;
    }
    return result + makeup_gain_db;
}

TransferCurveUpdateResult TransferCurve::make_TransferCurveUpdateResult(TransferCurveNormalizer n, float ndb)
{
    std::inplace_vector<float, k_max_knee_width_db> knee_ys;

    for (int x = ifloor<int>(pars.threshold_db) + 1; ifcast<float>(x) - pars.threshold_db < pars.knee_width_db; ++x) {
        knee_ys.push_back(ffcast<float>(x + gain_db_for_input_db(x)));
    }

    return TransferCurveUpdateResult{
      .normalizer = n,
      .normalizer_db = ndb,
      .threshold = AF2{pars.threshold_db,                      ffcast<float>(pars.threshold_db + makeup_gain_db)                     },
      .knee_ys = knee_ys,
      .knee_right =
        AF2{
                       pars.threshold_db + pars.knee_width_db, ffcast<float>(output_db_without_makeup_right_of_knee + makeup_gain_db)
        },
      .oo_ratio = 1 / pars.ratio
    };
}

TransferCurveUpdateResult TransferCurve::update()
{
    if (pars.knee_width_db == 0) {
        knee_A = NAN;
        knee_B = NAN;
        output_db_without_makeup_right_of_knee = pars.threshold_db;
    } else {
        knee_B = log(1.0 / pars.ratio) / pars.knee_width_db;
        knee_A = 1 / knee_B;
        output_db_without_makeup_right_of_knee = knee_A * exp(knee_B * pars.knee_width_db) + pars.threshold_db - knee_A;
    }
    switch (pars.normalizer) {
    case TransferCurveNormalizer::makeup_gain:
        makeup_gain_db = pars.normalizer_db;
        // Calculate reference_level.
        if (makeup_gain_db < 0) {
            // No reference level, output is always less.
            return make_TransferCurveUpdateResult(TransferCurveNormalizer::reference_level, NAN);
        } else if (makeup_gain_db == 0) {
            // No reference level, output is equal to input at least up to threshold.
            return make_TransferCurveUpdateResult(TransferCurveNormalizer::reference_level, -INFINITY);
        } else {
            // makeup_gain_db > 0
            if (pars.ratio == 1) {
                // Output is always greater.
                return make_TransferCurveUpdateResult(TransferCurveNormalizer::reference_level, NAN);
            }
            auto gain_db_right_of_knee = gain_db_for_input_db(pars.threshold_db + pars.knee_width_db);
            if (gain_db_right_of_knee <= 0) {
                // Knee must have crossed the gain = 0 line.
                // knee_A * exp(knee_B * (input_db - pars.threshold_db)) + pars.threshold_db - knee_A - input_db = 0;
                double left = pars.threshold_db;
                double right = pars.threshold_db + pars.knee_width_db;
                for (;;) {
                    const auto m = (left + right) / 2;
                    if (right - left < 1e-5) {
                        return make_TransferCurveUpdateResult(
                          TransferCurveNormalizer::reference_level, ffcast<float>(m)
                        );
                    }
                    (gain_db_for_input_db(m) <= 0 ? right : left) = m;
                }
            } else {
                // (input_db - threshold_db - knee_width_db) / ratio + output_db_without_makeup_right_of_knee - input_db
                // + makeup_gain = 0
                return make_TransferCurveUpdateResult(
                  TransferCurveNormalizer::reference_level,
                  ffcast<float>(
                    ((output_db_without_makeup_right_of_knee + makeup_gain_db) * pars.ratio - pars.threshold_db
                     - pars.knee_width_db)
                    / (pars.ratio - 1)
                  )
                );
            }
        }
    case TransferCurveNormalizer::reference_level:
        makeup_gain_db = 0;
        makeup_gain_db = -gain_db_for_input_db(pars.normalizer_db);
        assert(makeup_gain_db >= 0);
        makeup_gain_db = std::max(0.0, makeup_gain_db);
        return make_TransferCurveUpdateResult(TransferCurveNormalizer::makeup_gain, ffcast<float>(makeup_gain_db));
    }
}
