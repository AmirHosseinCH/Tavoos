#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitions.h>
#include <tavoos/types.h>

#include <memory>

namespace Tavoos {

struct MenuStyle {
    Paint backgroundColor{Color::rgba(255, 255, 255)};
    Paint borderColor{Color::rgba(220, 222, 230)};
    float borderWidth{1.0f};
    float padding{4.0f};
    int radius{8};
    TransitionFactory enter{[] { return std::make_unique<FadeIn>(0.12f); }};
    TransitionFactory exit{[] { return std::make_unique<FadeOut>(0.1f); }};
};

}
