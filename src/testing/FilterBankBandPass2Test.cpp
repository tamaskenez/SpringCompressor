#include "FilterBankBandPass2.h"

#include "FirstOrderIIR_TD2.h"
#include "meadow/matlab_signal.h"

#include <gtest/gtest.h>

TEST(FilterBankBandPass2, T1)
{
    const double fs = 48000;
    int fpo = 3;
    FilterBankBandPass2 fb(20 / fs * 2, 20000 / fs * 2, fpo, 1.5);
    array<double, 4> freqs;
    freqs[0] = 640; // At a center frequency
    freqs[1] = freqs[0] * pow(2, 0.25 / fpo);
    freqs[2] = freqs[0] * pow(2, 0.5 / fpo); // Halfway between two filters.
    freqs[3] = freqs[0] * pow(2, 0.75 / fpo);
    const auto N = iround<unsigned>(fs);
    constexpr double K = 4.0;
    vector<vector<double>> output_powers;
    for (unsigned j = 0; j < 4; ++j) {
        output_powers.push_back(vector<double>(N));
        auto& opb = output_powers.back();
        fb.reset();
        auto lpf = FirstOrderIIR_TDF2(matlab::butter(1, matlab::FilterType::LowPass{freqs[j] / fs * 2 / K}));
        for (unsigned i = 0; i < N; ++i) {
            const auto input = i < N / 2 ? sin(2 * num::pi * freqs[j] / fs * i) : 0.0;
            opb[i] = lpf.process(fb.process_and_get_total_power(input));
        }
    }

    FILE* f = fopen("/tmp/fb2test.m", "w");
    assert(f);
    println(f, "A = [");
    for (unsigned i = 0; i < N; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            print(f, " {}", output_powers[j][i]);
        }
        println(f, "");
    }
    println(f, "];");
    fclose(f);
}
