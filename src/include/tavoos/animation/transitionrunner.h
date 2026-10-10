#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/transition.h>
#include <tavoos/export.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace Tavoos {

class Widget;

class TAVOOS_EXPORT TransitionRunner : private AnimatableBase {
public:
    struct Entry {
        Widget* item;
        std::unique_ptr<Transition> transition;
    };

    TransitionRunner() = default;
    ~TransitionRunner() override;

    TransitionRunner(const TransitionRunner&) = delete;
    TransitionRunner& operator=(const TransitionRunner&) = delete;

    void start(std::vector<Entry> entries, float containerWidth, float containerHeight,
               std::function<bool(Widget*)> isAlive, std::function<void()> onFinished);
    void finishNow();
    bool running() const noexcept { return m_running; }

private:
    bool tick(float dt) override;
    void complete();
    void unregister();

    std::vector<Entry> m_entries;
    std::function<bool(Widget*)> m_isAlive;
    std::function<void()> m_onFinished;
    float m_elapsed{0.0f};
    float m_longest{0.0f};
    bool m_running{false};
    bool m_registered{false};
};

}
