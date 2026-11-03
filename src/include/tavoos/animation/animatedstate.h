#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/easing.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <algorithm>
#include <functional>
#include <utility>

namespace Tavoos {

namespace detail {
template<Interpolatable T>
class AnimateStep;
}

template<Interpolatable T>
class AnimatedState : public State<T>, public AnimatableBase {
public:
    static constexpr int Infinite = -1;

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
        m_loop = 0;
        m_active = true;
        setRegistered(true);
        m_running.setIfChanged(true);
    }

    void stop() {
        if (m_active && m_alwaysRunToEnd) {
            m_stopping = true;
            resume();
            return;
        }
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

    void loops(int count) { m_loops = count == Infinite ? Infinite : std::max(count, 1); }
    void alternate(bool enabled) { m_alternate = enabled; }
    void alwaysRunToEnd(bool enabled) { m_alwaysRunToEnd = enabled; }
    int loops() const noexcept { return m_loops; }
    bool alternate() const noexcept { return m_alternate; }
    bool alwaysRunToEnd() const noexcept { return m_alwaysRunToEnd; }

    bool isAnimating() const noexcept { return m_active && !m_paused; }
    bool isPaused() const noexcept { return m_paused; }
    State<bool>& runningState() { return m_running; }

private:
    friend class detail::AnimateStep<T>;

    bool tick(float dt) override {
        m_elapsed += dt;
        if (m_elapsed >= m_duration) {
            const auto passed = static_cast<long long>(m_elapsed / m_duration);
            if (m_stopping || (m_loops != Infinite && m_loop + passed >= m_loops)) {
                const bool completed = !m_stopping;
                if (completed)
                    m_loop = m_loops - 1;
                m_active = false;
                m_stopping = false;
                m_registered = false;
                State<T>::set(valueAt(1.0f));
                if (m_active)
                    return m_registered;
                m_running.setIfChanged(false);
                if (!m_active)
                    notifyFinished(completed);
                return m_registered;
            }
            m_loop += passed;
            m_elapsed -= static_cast<float>(passed) * m_duration;
        }
        State<T>::set(valueAt(m_elapsed / m_duration));
        return true;
    }

    T valueAt(float progress) const {
        const float p = m_alternate && m_loop % 2 == 1 ? 1.0f - progress : progress;
        return lerp(m_from, m_to, m_easing ? m_easing(p) : p);
    }

    bool halt() {
        if (!m_active)
            return false;
        m_active = false;
        m_paused = false;
        m_stopping = false;
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
    int m_loops{1};
    long long m_loop{0};
    bool m_alternate{false};
    bool m_alwaysRunToEnd{false};
    bool m_stopping{false};
    bool m_active{false};
    bool m_paused{false};
    bool m_registered{false};
    State<bool> m_running{false};
    std::function<void(bool)> m_onFinished;
};

}
