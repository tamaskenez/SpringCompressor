#pragma once

#include <meadow/cppext.h>

span<const float> get_sync_signal();
double normalized_correlation(span<const float> x, span<const float> y);
bool test_sync_signal(span<const float> x);
