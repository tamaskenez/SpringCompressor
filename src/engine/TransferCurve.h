#pragma once

#include <meadow/matlab.h>

#include <cmath>

class TransferCurve
{
public:
    void set_threshold_db(float threshold_db_arg)
    {
        threshold_db = threshold_db_arg;
    }
    void set_ratio(float ratio_arg)
    {
        ratio = ratio_arg;
    }
    [[nodiscard]] float apply_as_db(float input_db) const
    {
        return input_db < threshold_db ? input_db : (input_db - threshold_db) / ratio + threshold_db;
    }
    [[nodiscard]] float gain_for_input_db(float input_db) const
    {
        return input_db < threshold_db ? 1.0f
                                       : matlab::db2mag((input_db - threshold_db) / ratio + threshold_db - input_db);
    }

private:
    float threshold_db = NAN, ratio = NAN;
};
