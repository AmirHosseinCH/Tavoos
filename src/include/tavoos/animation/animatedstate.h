#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/easing.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <algorithm>
#include <functional>
#include <utility>

namespace Tavoos {

template<Interpolatable T>
class AnimatedState : public State<T>, public AnimatableBase {
public:
    using State<T>::State;

    ~AnimatedState() override {
        if (m_registered)
            AnimationManager::instance().unregisterAnimation(this);
    }

    void set(const T& value) {
        const bool interrupted = halt();
        State<T>::set(value);
        if (interrupted)
            notifyFinished(false);
    }

    void animateTo(const T& target, float durationSeconds, EasingFn easing = Easing::linear) {
        if (halt())
            notifyFinished(false);

        if (durationSeconds <= 0.0f) {
            State<T>::set(target);
            notifyFinished(true);
            return;
        }

        m_from = this->get();
        m_to = target;
        m_duration = durationSeconds;
        m_elapsed = 0.0f;
        m_easing = std::move(easing);
        m_active = true;
        setRegistered(true);
        m_running.setIfChanged(true);
    }

    void stop() {
        if (halt())
            notifyFinished(false);
    }

    void pause() {
        if (!m_active || m_paused)
            return;
        m_paused = true;
        setRegistered(false);
        m_running.setIfChanged(false);
    }

    void resume() {
        if (!m_active || !m_paused)
            return;
        m_paused = false;
        setRegistered(true);
        m_running.setIfChanged(true);
    }

    void onFinished(std::function<void(bool)> callback) { m_onFinished = std::move(callback); }

    bool isAnimating() const noexcept { return m_active && !m_paused; }
    bool isPaused() const noexcept { return m_paused; }
    State<bool>& runningState() { return m_running; }

private:
    bool tick(float dt) override {
        m_elapsed += dt;
        const float t = std::min(m_elapsed / m_duration, 1.0f);
        const float eased = m_easing ? m_easing(t) : t;
        State<T>::set(lerp(m_from, m_to, eased));

        if (t < 1.0f)
            return true;

        m_active = false;
        m_registered = false;
        m_running.setIfChanged(false);
        notifyFinished(true);
        return m_registered;
    }

    bool halt() {
        if (!m_active)
            return false;
        m_active = false;
        m_paused = false;
        setRegistered(false);
        m_running.setIfChanged(false);
        return true;
    }

    void setRegistered(bool registered) {
        if (registered == m_registered)
            return;
        m_registered = registered;
        if (registered)
            AnimationManager::instance().registerAnimation(this);
        else
            AnimationManager::instance().unregisterAnimation(this);
    }

    void notifyFinished(bool completed) {
        if (!m_onFinished)
            return;
        const auto callback = m_onFinished;
        callback(completed);
    }

    T m_from{};
    T m_to{};
    float m_duration{0.0f};
    float m_elapsed{0.0f};
    EasingFn m_easing{Easing::linear};
    bool m_active{false};
    bool m_paused{false};
    bool m_registered{false};
    State<bool> m_running{false};
    std::function<void(bool)> m_onFinished;
};

}
