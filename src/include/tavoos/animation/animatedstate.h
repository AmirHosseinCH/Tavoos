#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/easing.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <algorithm>
#include <utility>

namespace Tavoos {

template<Interpolatable T>
class AnimatedState : public State<T>, public AnimatableBase {
public:
    using State<T>::State;

    ~AnimatedState() override {
        if (m_animating)
            AnimationManager::instance().unregisterAnimation(this);
    }

    void animateTo(const T& target, float durationSeconds, EasingFn easing = Easing::linear) {
        if (durationSeconds <= 0.0f) {
            if (m_animating) {
                m_animating = false;
                AnimationManager::instance().unregisterAnimation(this);
            }
            this->set(target);
            return;
        }

        m_from = this->get();
        m_to = target;
        m_duration = durationSeconds;
        m_elapsed = 0.0f;
        m_easing = std::move(easing);

        if (!m_animating) {
            m_animating = true;
            AnimationManager::instance().registerAnimation(this);
        }
    }

    bool isAnimating() const noexcept { return m_animating; }

private:
    bool tick(float dt) override {
        m_elapsed += dt;
        const float t = std::min(m_elapsed / m_duration, 1.0f);
        const float eased = m_easing ? m_easing(t) : t;
        this->set(lerp(m_from, m_to, eased));

        if (t >= 1.0f) {
            m_animating = false;
            return false;
        }
        return true;
    }

    T m_from{};
    T m_to{};
    float m_duration{0.0f};
    float m_elapsed{0.0f};
    bool m_animating{false};
    EasingFn m_easing{Easing::linear};
};

}
