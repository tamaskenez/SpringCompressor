#include "TransferCurve.h"

#include <meadow/cppext.h>

#include <gtest/gtest.h>

namespace
{
template<class X, class Y>
    requires std::floating_point<X> && std::floating_point<Y>
bool equal_decimals(X x, Y y, int num_decimals)
{
    const auto factor = pow(10.0, num_decimals);
    return round(x * factor) == round(y * factor);
}
} // namespace

TEST(TransferCurve, T1)
{
    TransferCurve tc;
    TransferCurvePars tcp;
    std::println("A=[");
    for (int k = 0; k < 4; ++k) {
        switch (k) {
        case 0:
        case 1:
            break;
        case 2:
            tcp.knee_width_db = 10;
            tc.set(tcp);
            break;
        case 3:
            tcp.normalizer_db = 5;
            tc.set(tcp);
            break;
        }
        for (double i = tcp.threshold_db - 10; i <= 0; ++i) {
            switch (k) {
            case 0:
                std::print("{} ", i);
                break;
            case 1:
            case 2:
            case 3:
                std::print("{} ", i + tc.gain_db_for_input_db(i));
                break;
            default:
                assert(false);
            }
        }
        std::println();
    }
    std::println("]';");
    std::println("Slowly raising makeup gain");
    std::println("|makeupgain|reference_level|out@ref");
    for (float mg = -5; mg <= 5; mg += 0.1f) {
        mg = round(mg * 10) / 10;
        tcp.normalizer_db = mg;
        auto r = tc.set(tcp);
        assert(r->normalizer == TransferCurveNormalizer::reference_level);
        std::println("{} {} {}\n", mg, r->normalizer_db, tc.gain_db_for_input_db(r->normalizer_db));
    }
}
