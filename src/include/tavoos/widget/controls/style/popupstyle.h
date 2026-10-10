#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitions.h>
#include <tavoos/types.h>

#include <memory>

namespace Tavoos {

struct PopupStyle {
    Paint backgroundColor{Color::rgba(255, 255, 255)};
    Paint borderColor{Color::rgba(220, 222, 230)};
    Paint scrimColor{Color::rgba(0, 0, 0, 102)};
    float borderWidth{1.0f};
    float padding{8.0f};
    int radius{8};
    TransitionFactory enter{[] { return std::make_unique<FadeIn>(0.15f); }};
    TransitionFactory exit{[] { return std::make_unique<FadeOut>(0.1f); }};
};

}
