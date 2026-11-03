#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>

namespace Tavoos {

using EasingFn = std::function<float(float)>;

namespace Easing {

constexpr float linear(float t) { return t; }
constexpr float easeInQuad(float t) { return t * t; }
constexpr float easeOutQuad(float t) { return t * (2.0f - t); }
constexpr float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

constexpr float easeInCubic(float t) { return t * t * t; }
constexpr float easeOutCubic(float t) {
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}
constexpr float easeInOutCubic(float t) {
    if (t < 0.5f)
        return 4.0f * t * t * t;
    const float u = 2.0f - 2.0f * t;
    return 1.0f - u * u * u / 2.0f;
}

constexpr float easeInQuart(float t) { return t * t * t * t; }
constexpr float easeOutQuart(float t) {
    const float u = 1.0f - t;
    return 1.0f - u * u * u * u;
}
constexpr float easeInOutQuart(float t) {
    if (t < 0.5f)
        return 8.0f * t * t * t * t;
    const float u = 2.0f - 2.0f * t;
    return 1.0f - u * u * u * u / 2.0f;
}

constexpr float easeInQuint(float t) { return t * t * t * t * t; }
constexpr float easeOutQuint(float t) {
    const float u = 1.0f - t;
    return 1.0f - u * u * u * u * u;
}
constexpr float easeInOutQuint(float t) {
    if (t < 0.5f)
        return 16.0f * t * t * t * t * t;
    const float u = 2.0f - 2.0f * t;
    return 1.0f - u * u * u * u * u / 2.0f;
}

inline float easeInSine(float t) { return 1.0f - std::cos(t * std::numbers::pi_v<float> / 2.0f); }
inline float easeOutSine(float t) { return std::sin(t * std::numbers::pi_v<float> / 2.0f); }
inline float easeInOutSine(float t) { return -(std::cos(std::numbers::pi_v<float> * t) - 1.0f) / 2.0f; }

inline float easeInExpo(float t) { return t <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f); }
inline float easeOutExpo(float t) { return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
inline float easeInOutExpo(float t) {
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                    : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

inline float easeInCirc(float t) { return 1.0f - std::sqrt(std::max(0.0f, 1.0f - t * t)); }
inline float easeOutCirc(float t) { return std::sqrt(std::max(0.0f, 1.0f - (t - 1.0f) * (t - 1.0f))); }
inline float easeInOutCirc(float t) {
    if (t < 0.5f)
        return (1.0f - std::sqrt(std::max(0.0f, 1.0f - 4.0f * t * t))) / 2.0f;
    const float u = 2.0f - 2.0f * t;
    return (std::sqrt(std::max(0.0f, 1.0f - u * u)) + 1.0f) / 2.0f;
}

constexpr float easeInBack(float t) {
    constexpr float overshoot = 1.70158f;
    return (overshoot + 1.0f) * t * t * t - overshoot * t * t;
}
constexpr float easeOutBack(float t) {
    constexpr float overshoot = 1.70158f;
    const float u = t - 1.0f;
    return 1.0f + (overshoot + 1.0f) * u * u * u + overshoot * u * u;
}
constexpr float easeInOutBack(float t) {
    constexpr float overshoot = 1.70158f * 1.525f;
    if (t < 0.5f)
        return (2.0f * t) * (2.0f * t) * ((overshoot + 1.0f) * 2.0f * t - overshoot) / 2.0f;
    const float u = 2.0f * t - 2.0f;
    return (u * u * ((overshoot + 1.0f) * u + overshoot) + 2.0f) / 2.0f;
}

inline float easeInElastic(float t) {
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((10.0f * t - 10.75f) * 2.0f * std::numbers::pi_v<float> / 3.0f);
}
inline float easeOutElastic(float t) {
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((10.0f * t - 0.75f) * 2.0f * std::numbers::pi_v<float> / 3.0f) + 1.0f;
}
inline float easeInOutElastic(float t) {
    if (t <= 0.0f)
        return 0.0f;
    if (t >= 1.0f)
        return 1.0f;
    const float wave = std::sin((20.0f * t - 11.125f) * 2.0f * std::numbers::pi_v<float> / 4.5f);
    return t < 0.5f ? -std::pow(2.0f, 20.0f * t - 10.0f) * wave / 2.0f
                    : std::pow(2.0f, -20.0f * t + 10.0f) * wave / 2.0f + 1.0f;
}

constexpr float easeOutBounce(float t) {
    constexpr float strength = 7.5625f;
    constexpr float span = 2.75f;
    if (t < 1.0f / span)
        return strength * t * t;
    if (t < 2.0f / span) {
        t -= 1.5f / span;
        return strength * t * t + 0.75f;
    }
    if (t < 2.5f / span) {
        t -= 2.25f / span;
        return strength * t * t + 0.9375f;
    }
    t -= 2.625f / span;
    return strength * t * t + 0.984375f;
}
constexpr float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
constexpr float easeInOutBounce(float t) {
    return t < 0.5f ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f
                    : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f;
}

inline EasingFn cubicBezier(float x1, float y1, float x2, float y2) {
    x1 = std::clamp(x1, 0.0f, 1.0f);
    x2 = std::clamp(x2, 0.0f, 1.0f);
    return [=](float t) {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        const auto curve = [](float a, float b, float s) {
            const float r = 1.0f - s;
            return 3.0f * a * r * r * s + 3.0f * b * r * s * s + s * s * s;
        };
        const auto slope = [](float a, float b, float s) {
            const float r = 1.0f - s;
            return 3.0f * a * r * r + 6.0f * (b - a) * r * s + 3.0f * (1.0f - b) * s * s;
        };
        float s = t;
        for (int i = 0; i < 8; ++i) {
            const float error = curve(x1, x2, s) - t;
            if (std::abs(error) < 1e-6f)
                return curve(y1, y2, s);
            const float d = slope(x1, x2, s);
            if (std::abs(d) < 1e-6f)
                break;
            s = std::clamp(s - error / d, 0.0f, 1.0f);
        }
        float low = 0.0f;
        float high = 1.0f;
        s = t;
        for (int i = 0; i < 32; ++i) {
            if (curve(x1, x2, s) < t)
                low = s;
            else
                high = s;
            s = (low + high) / 2.0f;
        }
        return curve(y1, y2, s);
    };
}

}

}
