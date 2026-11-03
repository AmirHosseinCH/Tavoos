#include <tavoos/animation/animationmanager.h>

#include <algorithm>

namespace Tavoos {

AnimationManager& AnimationManager::instance() {
    static AnimationManager manager;
    return manager;
}

void AnimationManager::registerAnimation(AnimatableBase* animation) {
    if (std::find(m_active.begin(), m_active.end(), animation) == m_active.end())
        m_active.push_back(animation);
}

void AnimationManager::unregisterAnimation(AnimatableBase* animation) {
    std::erase(m_active, animation);
}

void AnimationManager::tick(float dt) {
    const auto activeSnapshot = m_active;
    for (AnimatableBase* animation : activeSnapshot) {
        if (std::find(m_active.begin(), m_active.end(), animation) == m_active.end())
            continue;
        if (!animation->tick(dt))
            unregisterAnimation(animation);
    }
}

}
