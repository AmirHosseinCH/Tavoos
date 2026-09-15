#pragma once

#include <functional>

namespace Tavoos {

using EasingFn = std::function<float(float)>;

namespace Easing {

constexpr float linear(float t) { return t; }
constexpr float easeInQuad(float t) { return t * t; }
constexpr float easeOutQuad(float t) { return t * (2.0f - t); }
constexpr float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

}

}
