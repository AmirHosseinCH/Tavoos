#include <tavoos/animation/transitionrunner.h>

#include <algorithm>

namespace Tavoos {

TransitionRunner::~TransitionRunner() {
    unregister();
}

void TransitionRunner::unregister() {
    if (m_registered) {
        m_registered = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
}

void TransitionRunner::start(std::vector<Entry> entries, float containerWidth, float containerHeight,
                             std::function<bool(Widget*)> isAlive, std::function<void()> onFinished) {
    finishNow();

    m_entries = std::move(entries);
    m_isAlive = std::move(isAlive);
    m_onFinished = std::move(onFinished);
    m_elapsed = 0.0f;
    m_longest = 0.0f;
    m_running = true;

    for (auto& entry : m_entries) {
        if (!entry.item || !entry.transition || (m_isAlive && !m_isAlive(entry.item))) {
            entry.transition.reset();
            continue;
        }
        entry.transition->prepare(*entry.item, containerWidth, containerHeight);
        m_longest = std::max(m_longest, entry.transition->duration());
    }

    if (m_longest <= 0.0f) {
        for (auto& entry : m_entries)
            if (entry.transition)
                entry.transition->update(1.0f);
        complete();
        return;
    }

    m_registered = true;
    AnimationManager::instance().registerAnimation(this);
}

void TransitionRunner::finishNow() {
    if (!m_running)
        return;
    for (auto& entry : m_entries) {
        if (entry.transition && (!m_isAlive || m_isAlive(entry.item)))
            entry.transition->update(1.0f);
    }
    unregister();
    complete();
}

bool TransitionRunner::tick(float dt) {
    m_elapsed += dt;
    for (auto& entry : m_entries) {
        if (!entry.transition || (m_isAlive && !m_isAlive(entry.item)))
            continue;
        const float duration = entry.transition->duration();
        const float t = duration > 0.0f ? std::min(m_elapsed / duration, 1.0f) : 1.0f;
        const EasingFn& easing = entry.transition->easing();
        entry.transition->update(easing ? easing(t) : t);
    }

    if (m_elapsed < m_longest)
        return true;

    m_registered = false;
    complete();
    return false;
}

void TransitionRunner::complete() {
    std::vector<Entry> entries = std::move(m_entries);
    std::function<void()> onFinished = std::move(m_onFinished);
    std::function<bool(Widget*)> isAlive = std::move(m_isAlive);
    m_entries.clear();
    m_onFinished = nullptr;
    m_isAlive = nullptr;
    m_running = false;

    for (auto& entry : entries) {
        if (entry.transition && (!isAlive || isAlive(entry.item)))
            entry.transition->finish(true);
    }
    if (onFinished)
        onFinished();
}

}
