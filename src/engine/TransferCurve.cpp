#include "TransferCurve.h"

#include <meadow/cppext.h>

#include <cassert>

TransferCurve::TransferCurve()
{
    update();
}

std::optional<TransferCurveUpdateResult> TransferCurve::set(const TransferCurvePars& p)
{
    if (pars == p) {
        return std::nullopt;
    }
    pars = p;
    if (std::isnan(p.ratio) || p.ratio < 1) {
        assert(false);
        pars.ratio = 1;
    }
    if (std::isnan(p.knee_width_db) || p.knee_width_db < 0) {
        assert(false);
        pars.knee_width_db = 0;
    }
    return update();
}
float TransferCurve::gain_db_for_input_db(float input_db) const
{
    double result = 0;
    if (input_db <= pars.threshold_db) {
        NOP;
    } else if (input_db <= pars.threshold_db + pars.knee_width_db) {
        result = knee_A * exp(knee_B * (input_db - pars.threshold_db)) + pars.threshold_db - knee_A;
    } else {
        result = (input_db - pars.threshold_db) / pars.ratio + pars.threshold_db - input_db;
    }
    return ffcast<float>(result + makeup_gain_db);
}

TransferCurveUpdateResult TransferCurve::update()
{
    knee_B = log(1.0 / pars.ratio) / pars.knee_width_db;
    knee_A = 1 / knee_B;
    switch (pars.normalizer) {
    case TransferCurveNormalizer::makeup_gain:
        makeup_gain_db = pars.normalizer_db;
        // Calculate reference_level.
        if (makeup_gain_db < 0) {
            // No reference level, output is always less.
            return TransferCurveUpdateResult{TransferCurveNormalizer::reference_level, NAN};
        } else if (makeup_gain_db == 0) {
            // No reference level, output is equal to input at least up to threshold.
            return TransferCurveUpdateResult{TransferCurveNormalizer::reference_level, -INFINITY};
        } else {
            // makeup_gain_db > 0
            auto gain_db_right_of_knee = gain_db_for_input_db(pars.threshold_db + pars.knee_width_db);
            if (gain_db_right_of_knee <= 0) {
                // Knee must have crossed the gain = 0 line.
                // knee_A * exp(knee_B * (input_db - threshold_db)) + threshold_db - knee_A + makeup_gain_db = 0
                return TransferCurveUpdateResult{
                  TransferCurveNormalizer::reference_level,
                  ffcast<float>(
                    log((knee_A - makeup_gain_db - pars.threshold_db) / knee_A) / knee_B + pars.threshold_db
                  )
                };
            } else {
                return TransferCurveUpdateResult{
                  TransferCurveNormalizer::reference_level,
                  ffcast<float>(
                    ((makeup_gain_db + pars.threshold_db) * pars.ratio - pars.threshold_db) / (pars.ratio - 1)
                  )
                };
            }
        }
    case TransferCurveNormalizer::reference_level:
        makeup_gain_db = 0;
        makeup_gain_db = -gain_db_for_input_db(pars.normalizer_db);
        assert(makeup_gain_db >= 0);
        makeup_gain_db = std::max(0.0, makeup_gain_db);
        return TransferCurveUpdateResult{TransferCurveNormalizer::makeup_gain, ffcast<float>(makeup_gain_db)};
    }
}
