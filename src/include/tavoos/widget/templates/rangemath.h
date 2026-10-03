#pragma once

#include <algorithm>

namespace Tavoos::detail {

inline long long clampToRange(long long value, long long minValue, long long maxValue) {
    return std::clamp(value, minValue, std::max(minValue, maxValue));
}

inline float fractionInRange(long long value, long long minValue, long long maxValue) {
    const long long hi = std::max(minValue + 1, maxValue);
    const long long clamped = std::clamp(value, minValue, hi);
    return static_cast<float>(static_cast<double>(clamped - minValue) / static_cast<double>(hi - minValue));
}

inline long long spanOf(long long minValue, long long maxValue) {
    return std::max(1LL, maxValue - minValue);
}

}
