#pragma once

#include <tavoos/export.hpp>
#include <tavoos/events/dragevent.h>
#include <tavoos/events/keyevent.h>
#include <tavoos/events/wheelevent.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <cstdint>
#include <functional>
#include <type_traits>

namespace Tavoos {

enum class FlickDirection : std::uint32_t {
    None = 0,
    Horizontal = 1 << 0,
    Vertical = 1 << 1,
};

template<>
struct EnableBitmaskOperators<FlickDirection> : std::true_type {};

class TAVOOS_EXPORT FlickAreaBase : public Control {
public:
    FlickAreaBase(Object* parent);

    decltype(auto) flickDirection(this auto&& self, PropertyArg<FlickDirection> direction) {
        direction.applyTo(self.m_direction);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) contentX(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_contentX);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) contentY(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_contentY);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) contentWidth(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_contentWidth);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) contentHeight(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_contentHeight);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) wheelStep(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_wheelStep);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragScroll(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_dragScroll);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) keyNavigation(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_keyNavigation);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onScroll(this auto&& self, std::function<void(float, float)> callback) {
        self.m_onScroll = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    FlickDirection flickDirection() const { return m_direction; }
    float wheelStep() const { return m_wheelStep; }
    bool dragScroll() const { return m_dragScroll; }
    bool keyNavigation() const { return m_keyNavigation; }

    float contentX() const { return m_contentXState.get(); }
    float contentY() const { return m_contentYState.get(); }
    float contentWidth();
    float contentHeight();
    float viewportWidth() const;
    float viewportHeight() const;
    float maxContentX();
    float maxContentY();

    State<float>& contentXState() { return m_contentXState; }
    State<float>& contentYState() { return m_contentYState; }

    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    void scrollToTop();
    void scrollToBottom();
    void scrollToLeft();
    void scrollToRight();

protected:
    virtual Size computeContentSize();
    virtual bool isContentChild(const Widget& child) const;
    virtual void onOffsetsSynced(bool offsetsChanged) { (void)offsetsChanged; }

    void render(Renderer& renderer) override;
    bool hasHandlerFor(EventType type) override;
    void triggerWheel(WheelEvent& event) override;
    void triggerDragMove(DragEvent& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    ContentArea contentAreaFor(const Widget& child) const override;
    void onResolvedSizeChanged() override;

private:
    using Control::content;

    bool scrollable();
    void syncOffsets();

    Property<FlickDirection> m_direction{FlickDirection::Horizontal | FlickDirection::Vertical};
    Property<float> m_contentX{0.0f};
    Property<float> m_contentY{0.0f};
    Property<float> m_contentWidth{0.0f};
    Property<float> m_contentHeight{0.0f};
    Property<float> m_wheelStep{48.0f};
    Property<bool> m_dragScroll{true};
    Property<bool> m_keyNavigation{true};
    State<float> m_contentXState{0.0f};
    State<float> m_contentYState{0.0f};

    std::function<void(float, float)> m_onScroll;
};

}
