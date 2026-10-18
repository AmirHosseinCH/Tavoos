#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct BlurStyle {
    float startRadius{0.0f};
    float endRadius{0.0f};
    float direction{0.0f};
};

inline BlurStyle lerp(const BlurStyle& a, const BlurStyle& b, float t) {
    return {
        lerp(a.startRadius, b.startRadius, t),
        lerp(a.endRadius, b.endRadius, t),
        lerp(a.direction, b.direction, t),
    };
}

}
