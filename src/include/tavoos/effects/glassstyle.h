#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct GlassStyle {
    float lightAngle{-45.0f};
    float lightIntensity{0.8f};
    float refraction{0.8f};
    float depth{20.0f};
    float dispersion{0.2f};
    float frost{4.0f};
    float splay{0.0f};
    float zoom{1.0f};
    float thickness{1.0f};
    float bevel{1.0f};
    float rimWidth{1.5f};
    Color tint{Color::rgba(255, 255, 255, 0)};
    float saturation{1.0f};
    float brightness{1.0f};
};

inline GlassStyle lerp(const GlassStyle& a, const GlassStyle& b, float t) {
    return {
        lerp(a.lightAngle, b.lightAngle, t),
        lerp(a.lightIntensity, b.lightIntensity, t),
        lerp(a.refraction, b.refraction, t),
        lerp(a.depth, b.depth, t),
        lerp(a.dispersion, b.dispersion, t),
        lerp(a.frost, b.frost, t),
        lerp(a.splay, b.splay, t),
        lerp(a.zoom, b.zoom, t),
        lerp(a.thickness, b.thickness, t),
        lerp(a.bevel, b.bevel, t),
        lerp(a.rimWidth, b.rimWidth, t),
        lerp(a.tint, b.tint, t),
        lerp(a.saturation, b.saturation, t),
        lerp(a.brightness, b.brightness, t),
    };
}

}
