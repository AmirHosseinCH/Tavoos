#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/animation/easing.h>
#include <tavoos/events/dragevent.h>
#include <tavoos/events/keyevent.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace Tavoos {

enum class Orientation {
    Horizontal,
    Vertical,
};

class TAVOOS_EXPORT SwipeViewBase : public Control, private AnimatableBase {
public:
    SwipeViewBase(Object* parent);
    ~SwipeViewBase() override;

    decltype(auto) orientation(this auto&& self, PropertyArg<Orientation> value) {
        value.applyTo(self.m_orientation);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) currentIndex(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_currentIndex);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) interactive(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_interactive);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) keyNavigation(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_keyNavigation);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) duration(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_duration);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) easing(this auto&& self, PropertyArg<EasingFn> value) {
        value.applyTo(self.m_easing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onCurrentIndexChange(this auto&& self, std::function<void(int)> callback) {
        self.m_onCurrentIndexChange = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    Orientation orientation() const { return m_orientation; }
    int currentIndex() const { return m_currentIndexState.get(); }
    bool interactive() const { return m_interactive; }
    bool keyNavigation() const { return m_keyNavigation; }
    float duration() const { return m_duration; }
    const EasingFn& easing() const { return m_easing.get(); }

    int count() const;
    float position() const noexcept { return m_position; }
    Widget* page(int index) const;

    State<int>& currentIndexState() { return m_currentIndexState; }
    State<int>& countState() { return m_countState; }

    void setCurrentIndex(int index, bool animated = true);
    void next();
    void previous();

protected:
    void render(Renderer& renderer) override;
    bool hasHandlerFor(EventType type) override;
    void triggerDragStart(DragEvent& event) override;
    void triggerDragMove(DragEvent& event) override;
    void triggerDragEnd(DragEvent& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    ContentArea contentAreaFor(const Widget& child) const override;
    void onResolvedSizeChanged() override;

private:
    using Control::content;

    struct Sample {
        std::chrono::steady_clock::time_point time;
        float pixels;
    };

    bool tick(float dt) override;

    std::vector<Widget*> pages() const;
    bool horizontal() const;
    float pageSize() const;
    void refreshPages();
    void applyIndex(bool animated);
    void animateTo(int index);
    void stopAnimation();
    void setPosition(float value);
    void syncVisibility();
    void moveFocus(Widget* from, Widget* to);
    void beginDrag();
    void recordSample();
    float releaseVelocity() const;

    Property<Orientation> m_orientation{Orientation::Horizontal};
    Property<int> m_currentIndex{0};
    Property<bool> m_interactive{true};
    Property<bool> m_keyNavigation{true};
    Property<float> m_duration{0.25f};
    Property<EasingFn> m_easing{Easing::easeInOutQuad};
    State<int> m_currentIndexState{0};
    State<int> m_countState{0};
    std::function<void(int)> m_onCurrentIndexChange;

    std::vector<Widget*> m_pages;
    Widget* m_currentPage{nullptr};
    std::vector<std::pair<Widget*, Widget*>> m_focusMemory;

    float m_position{0.0f};
    float m_from{0.0f};
    float m_to{0.0f};
    float m_elapsed{0.0f};
    bool m_animating{false};
    bool m_dragging{false};
    bool m_jumping{false};
    std::vector<Sample> m_samples;
};

}
