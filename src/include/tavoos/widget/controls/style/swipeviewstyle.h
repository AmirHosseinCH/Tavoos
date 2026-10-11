#pragma once

#include <tavoos/animation/easing.h>

namespace Tavoos {

struct SwipeViewStyle {
    float duration{0.25f};
    EasingFn easing{Easing::easeInOutQuad};
};

}
