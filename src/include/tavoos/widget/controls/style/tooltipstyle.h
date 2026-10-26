#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitions.h>
#include <tavoos/text/font.h>
#include <tavoos/types.h>

#include <memory>

namespace Tavoos {

struct ToolTipStyle {
    Paint backgroundColor{Color::rgba(40, 42, 50)};
    Paint borderColor{Color::Transparent};
    float borderWidth{0.0f};
    float padding{6.0f};
    int radius{6};
    Paint textColor{Color::White};
    Font font;
    TransitionFactory enter{[] { return std::make_unique<FadeIn>(0.12f); }};
    TransitionFactory exit{[] { return std::make_unique<FadeOut>(0.08f); }};
};

}
