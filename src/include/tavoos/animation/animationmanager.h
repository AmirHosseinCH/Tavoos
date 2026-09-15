#pragma once

#include <tavoos/export.hpp>

#include <vector>

namespace Tavoos {

class Application;

class TAVOOS_EXPORT AnimatableBase {
public:
    virtual ~AnimatableBase() = default;

private:
    friend class AnimationManager;
    virtual bool tick(float dt) = 0;
};

class TAVOOS_EXPORT AnimationManager {
    friend class Application;

public:
    static AnimationManager& instance();

    AnimationManager(const AnimationManager&) = delete;
    AnimationManager(AnimationManager&&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;
    AnimationManager& operator=(AnimationManager&&) = delete;

    void registerAnimation(AnimatableBase* animation);
    void unregisterAnimation(AnimatableBase* animation);
    bool hasActiveAnimations() const noexcept { return !m_active.empty(); }

private:
    AnimationManager() = default;

    void tick(float dt);

    std::vector<AnimatableBase*> m_active;
};

}
