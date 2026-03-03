#pragma once

#include <meadow/matlab.h>

#include <cmath>

#if 0
template<size_t D>
class FixedPoint
{
public:
    explicit FixedPoint(double f_arg)
        : i(round(f_arg * pow(10.0, D)))
    {
    }
    explicit FixedPoint(float f_arg)
        : FixedPoint(ffcast<double>(f_arg))
    {
    }
    [[nodiscard]] double get_double() const
    {
        return i / pow(10.0, D);
    }
    [[nodiscard]] float get_float() const
    {
        return ffcast<float>(get_double());
    }

private:
    int64_t i = 0;
};

#endif

class TransferCurve
{
public:
    void set_threshold_db(float threshold_db_arg)
    {
        if (threshold_db_arg != threshold_db) {
            threshold_db = threshold_db_arg;
            update();
        }
    }
    void set_ratio(float ratio_arg)
    {
        if (ratio_arg != ratio) {
            ratio = ratio_arg;
            update();
        }
    }
    void set_reference_level_db(float rl)
    {
        if (reference_level_db != rl) {
            reference_level_db = rl;
            update();
        }
    }
    void set_makeup_gain_db(float m)
    {
        if (makeup_gain_db != m) {
            makeup_gain_db = m;
            update();
        }
    }
    void set_knee_width_db(float x)
    {
        if (x != knee_width_db) {
            knee_width_db = x;
            update();
        }
    }
    [[nodiscard]] float gain_db_for_input_db(float input_db) const
    {
        double result = 0;
        if (input_db <= threshold_db) {
            NOP;
        } else if (input_db <= threshold_db + knee_width_db) {
            result = knee_A * exp(knee_B * (input_db - threshold_db)) + threshold_db - knee_A;
        } else {
            result = (input_db - threshold_db) / ratio + threshold_db - input_db;
        }
        return ffcast<float>(result + makeup_gain_db);
    }

private:
    float threshold_db = NAN, ratio = NAN, knee_width_db = 0;
    float makeup_gain_db = 0, reference_level_db = 0;
    double knee_A = NAN, knee_B = NAN;

    void update()
    {
        knee_B = log(1.0 / ratio) / knee_width_db;
        knee_A = 1 / knee_B;
    }
};
