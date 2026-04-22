#pragma once

#include <meadow/cppext.h>

vector<float> make_sync_signal();
double normalized_correlation(span<const float> x, span<const float> y);
bool test_sync_signal(span<const float> x);
