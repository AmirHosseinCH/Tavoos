#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct ShadowStyle {
    float offsetX{0.0f};
    float offsetY{0.0f};
    float radius{0.0f};
    float spread{0.0f};
    Color color{Color::rgba(0, 0, 0, 64)};
};

inline ShadowStyle lerp(const ShadowStyle& a, const ShadowStyle& b, float t) {
    return {
        lerp(a.offsetX, b.offsetX, t),
        lerp(a.offsetY, b.offsetY, t),
        lerp(a.radius, b.radius, t),
        lerp(a.spread, b.spread, t),
        lerp(a.color, b.color, t),
    };
}

}
