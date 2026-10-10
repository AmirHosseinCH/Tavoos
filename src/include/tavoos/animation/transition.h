#pragma once

#include <tavoos/animation/easing.h>
#include <tavoos/export.hpp>

#include <functional>
#include <memory>

namespace Tavoos {

class Widget;

class TAVOOS_EXPORT Transition {
public:
    explicit Transition(float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : m_duration{duration}, m_easing{std::move(easing)} {}
    virtual ~Transition() = default;

    float duration() const noexcept { return m_duration; }
    const EasingFn& easing() const noexcept { return m_easing; }

    virtual void prepare(Widget& item, float containerWidth, float containerHeight) = 0;
    virtual void update(float progress) = 0;
    virtual void finish(bool completed) { (void)completed; }

private:
    float m_duration;
    EasingFn m_easing;
};

using TransitionFactory = std::function<std::unique_ptr<Transition>()>;

}
