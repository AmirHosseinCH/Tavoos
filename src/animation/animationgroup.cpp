#include <tavoos/animation/animationgroup.h>

#include <algorithm>

namespace Tavoos {

AnimationGroup::~AnimationGroup() {
    if (m_registered)
        AnimationManager::instance().unregisterAnimation(this);
}

void AnimationGroup::start() {
    if (m_root != this)
        return;
    if (halt())
        notifyFinished(false);

    const auto current = ++m_generation;
    m_active = true;
    setRegistered(true);
    m_running.setIfChanged(true);
    if (interrupted(current))
        return;

    begin();
    if (interrupted(current))
        return;

    run(0.0f);
}

void AnimationGroup::stop() {
    if (m_root != this)
        return;
    if (halt())
        notifyFinished(false);
}

void AnimationGroup::pause() {
    if (m_root != this || !m_active || m_paused)
        return;
    m_paused = true;
    setRegistered(false);
    m_running.setIfChanged(false);
}

void AnimationGroup::resume() {
    if (m_root != this || !m_active || !m_paused)
        return;
    m_paused = false;
    setRegistered(true);
    m_running.setIfChanged(true);
}

bool AnimationGroup::tick(float dt) {
    run(dt);
    return m_registered;
}

void AnimationGroup::run(float dt) {
    const auto current = m_generation;
    const auto leftover = advance(dt);
    if (interrupted(current) || !leftover)
        return;

    m_active = false;
    m_paused = false;
    setRegistered(false);
    m_running.setIfChanged(false);
    notifyFinished(true);
}

bool AnimationGroup::halt() {
    if (!m_active)
        return false;
    ++m_generation;
    m_active = false;
    m_paused = false;
    setRegistered(false);
    m_running.setIfChanged(false);
    return true;
}

void AnimationGroup::setRegistered(bool registered) {
    if (registered == m_registered)
        return;
    m_registered = registered;
    if (registered)
        AnimationManager::instance().registerAnimation(this);
    else
        AnimationManager::instance().unregisterAnimation(this);
}

void AnimationGroup::notifyFinished(bool completed) {
    if (!m_onFinished)
        return;
    const auto callback = m_onFinished;
    callback(completed);
}

SequentialAnimation& AnimationGroup::addSequential() {
    auto group = std::make_unique<SequentialAnimation>();
    group->m_root = m_root;
    SequentialAnimation& result = *group;
    m_steps.push_back(std::move(group));
    return result;
}

ParallelAnimation& AnimationGroup::addParallel() {
    auto group = std::make_unique<ParallelAnimation>();
    group->m_root = m_root;
    ParallelAnimation& result = *group;
    m_steps.push_back(std::move(group));
    return result;
}

void SequentialAnimation::begin() {
    m_index = 0;
    if (!m_steps.empty())
        m_steps.front()->begin();
}

std::optional<float> SequentialAnimation::advance(float dt) {
    const auto current = generation();
    while (m_index < m_steps.size()) {
        const auto leftover = m_steps[m_index]->advance(dt);
        if (interrupted(current) || !leftover)
            return std::nullopt;
        dt = *leftover;
        if (++m_index < m_steps.size()) {
            m_steps[m_index]->begin();
            if (interrupted(current))
                return std::nullopt;
        }
    }
    return dt;
}

void ParallelAnimation::begin() {
    const auto current = generation();
    m_finished.assign(m_steps.size(), false);
    for (auto& step : m_steps) {
        step->begin();
        if (interrupted(current))
            return;
    }
}

std::optional<float> ParallelAnimation::advance(float dt) {
    const auto current = generation();
    float leftover = dt;
    bool finished = true;
    for (std::size_t i = 0; i < m_finished.size(); ++i) {
        if (m_finished[i])
            continue;
        const auto stepLeftover = m_steps[i]->advance(dt);
        if (interrupted(current))
            return std::nullopt;
        if (stepLeftover) {
            m_finished[i] = true;
            leftover = std::min(leftover, *stepLeftover);
        } else {
            finished = false;
        }
    }
    if (!finished)
        return std::nullopt;
    return leftover;
}

}
