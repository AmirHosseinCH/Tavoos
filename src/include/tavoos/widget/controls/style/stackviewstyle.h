#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitions.h>

#include <memory>

namespace Tavoos {

struct StackViewStyle {
    TransitionFactory pushEnter{[] { return std::make_unique<SlideIn>(Edge::Right); }};
    TransitionFactory pushExit{[] { return std::make_unique<SlideOut>(Edge::Left); }};
    TransitionFactory popEnter{[] { return std::make_unique<SlideIn>(Edge::Left); }};
    TransitionFactory popExit{[] { return std::make_unique<SlideOut>(Edge::Right); }};
    TransitionFactory replaceEnter{[] { return std::make_unique<FadeIn>(); }};
    TransitionFactory replaceExit{[] { return std::make_unique<FadeOut>(); }};
};

}
