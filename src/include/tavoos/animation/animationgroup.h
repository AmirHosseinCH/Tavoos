#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/easing.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace Tavoos {

namespace detail {

class AnimationStep {
public:
    virtual ~AnimationStep() = default;
    virtual void begin() = 0;
    virtual std::optional<float> advance(float dt) = 0;
};

template<Interpolatable T>
class AnimateStep : public AnimationStep {
public:
    AnimateStep(AnimatedState<T>& state, std::optional<T> from, const T& to, float durationSeconds, EasingFn easing)
        : m_state{state}, m_start{std::move(from)}, m_to{to}, m_duration{std::max(durationSeconds, 0.0f)},
          m_easing{std::move(easing)} {}

    void begin() override {
        if (m_state.halt())
            m_state.notifyFinished(false);
        m_from = m_start ? *m_start : m_state.get();
        m_elapsed = 0.0f;
    }

    std::optional<float> advance(float dt) override {
        State<T>& output = m_state;
        m_elapsed += dt;
        if (m_elapsed < m_duration) {
            const float progress = m_elapsed / m_duration;
            output.set(lerp(m_from, m_to, m_easing ? m_easing(progress) : progress));
            return std::nullopt;
        }
        const float leftover = m_elapsed - m_duration;
        output.set(m_to);
        return leftover;
    }

private:
    AnimatedState<T>& m_state;
    std::optional<T> m_start;
    T m_from{};
    T m_to;
    float m_duration;
    float m_elapsed{0.0f};
    EasingFn m_easing;
};

class WaitStep : public AnimationStep {
public:
    explicit WaitStep(float durationSeconds) : m_duration{std::max(durationSeconds, 0.0f)} {}

    void begin() override { m_elapsed = 0.0f; }

    std::optional<float> advance(float dt) override {
        m_elapsed += dt;
        if (m_elapsed < m_duration)
            return std::nullopt;
        return m_elapsed - m_duration;
    }

private:
    float m_duration;
    float m_elapsed{0.0f};
};

class ScriptStep : public AnimationStep {
public:
    explicit ScriptStep(std::function<void()> action) : m_action{std::move(action)} {}

    void begin() override {}

    std::optional<float> advance(float dt) override {
        if (m_action)
            m_action();
        return dt;
    }

private:
    std::function<void()> m_action;
};

}

class SequentialAnimation;
class ParallelAnimation;

class TAVOOS_EXPORT AnimationGroup : public detail::AnimationStep, private AnimatableBase {
public:
    static constexpr int Infinite = -1;

    AnimationGroup() = default;
    ~AnimationGroup() override;

    AnimationGroup(const AnimationGroup&) = delete;
    AnimationGroup& operator=(const AnimationGroup&) = delete;

    template<Interpolatable T>
    decltype(auto) animateTo(this auto&& self, AnimatedState<T>& state, const std::type_identity_t<T>& to,
                             float durationSeconds, EasingFn easing = Easing::linear) {
        self.m_steps.push_back(
            std::make_unique<detail::AnimateStep<T>>(state, std::nullopt, to, durationSeconds, std::move(easing)));
        return std::forward<decltype(self)>(self);
    }

    template<Interpolatable T>
    decltype(auto) animateFrom(this auto&& self, AnimatedState<T>& state, const std::type_identity_t<T>& from,
                               const std::type_identity_t<T>& to, float durationSeconds,
                               EasingFn easing = Easing::linear) {
        self.m_steps.push_back(
            std::make_unique<detail::AnimateStep<T>>(state, from, to, durationSeconds, std::move(easing)));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) wait(this auto&& self, float durationSeconds) {
        self.m_steps.push_back(std::make_unique<detail::WaitStep>(durationSeconds));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) script(this auto&& self, std::function<void()> action) {
        self.m_steps.push_back(std::make_unique<detail::ScriptStep>(std::move(action)));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) sequential(this auto&& self, std::function<void(SequentialAnimation&)> build) {
        build(self.addSequential());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) parallel(this auto&& self, std::function<void(ParallelAnimation&)> build) {
        build(self.addParallel());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) loops(this auto&& self, int count) {
        self.m_loops = count == Infinite ? Infinite : std::max(count, 1);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) alwaysRunToEnd(this auto&& self, bool enabled) {
        self.m_alwaysRunToEnd = enabled;
        return std::forward<decltype(self)>(self);
    }

    int loops() const noexcept { return m_loops; }
    bool alwaysRunToEnd() const noexcept { return m_alwaysRunToEnd; }

    void start();
    void stop();
    void pause();
    void resume();
    void onFinished(std::function<void(bool)> callback) { m_onFinished = std::move(callback); }

    bool isAnimating() const noexcept { return m_active && !m_paused; }
    bool isPaused() const noexcept { return m_paused; }
    State<bool>& runningState() { return m_running; }

protected:
    std::uint64_t generation() const noexcept { return m_root->m_generation; }
    bool interrupted(std::uint64_t generation) const noexcept { return m_root->m_generation != generation; }
    bool rootPaused() const noexcept { return m_root->m_paused; }

    std::vector<std::unique_ptr<detail::AnimationStep>> m_steps;

private:
    void begin() override;
    std::optional<float> advance(float dt) override;
    virtual void beginRound() = 0;
    virtual std::optional<float> advanceRound(float dt) = 0;
    bool tick(float dt) override;
    void run(float dt);
    bool halt();
    void setRegistered(bool registered);
    void notifyFinished(bool completed);
    SequentialAnimation& addSequential();
    ParallelAnimation& addParallel();

    AnimationGroup* m_root{this};
    std::uint64_t m_generation{0};
    int m_loops{1};
    long long m_round{0};
    bool m_alwaysRunToEnd{false};
    bool m_stopping{false};
    bool m_active{false};
    bool m_paused{false};
    bool m_registered{false};
    State<bool> m_running{false};
    std::function<void(bool)> m_onFinished;
};

class TAVOOS_EXPORT SequentialAnimation : public AnimationGroup {
private:
    void beginRound() override;
    std::optional<float> advanceRound(float dt) override;

    std::size_t m_index{0};
};

class TAVOOS_EXPORT ParallelAnimation : public AnimationGroup {
private:
    void beginRound() override;
    std::optional<float> advanceRound(float dt) override;

    std::vector<bool> m_finished;
};

}
