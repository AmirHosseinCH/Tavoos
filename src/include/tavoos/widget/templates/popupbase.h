#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <cstdint>
#include <functional>

namespace Tavoos {

enum class ClosePolicy : std::uint32_t {
    None = 0,
    ClickOutside = 1 << 0,
    Escape = 1 << 1,
};

template<>
struct EnableBitmaskOperators<ClosePolicy> : std::true_type {};

enum class Placement {
    Bottom,
    Top,
    Left,
    Right,
    Center,
};

enum class PlacementTarget {
    Parent,
    Window,
};

class TAVOOS_EXPORT PopupBase : public Control {
public:
    PopupBase(Object* parent);

    decltype(auto) opened(this auto&& self, PropertyArg<bool> opened) {
        opened.applyTo(self.m_opened);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) closePolicy(this auto&& self, PropertyArg<ClosePolicy> policy) {
        policy.applyTo(self.m_closePolicy);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) modal(this auto&& self, PropertyArg<bool> modal) {
        modal.applyTo(self.m_modal);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) placement(this auto&& self, PropertyArg<Placement> placement) {
        placement.applyTo(self.m_placement);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) target(this auto&& self, PropertyArg<PlacementTarget> target) {
        target.applyTo(self.m_target);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offset(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offset.leftProperty());
        value.applyTo(self.m_offset.topProperty());
        value.applyTo(self.m_offset.rightProperty());
        value.applyTo(self.m_offset.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offsetLeft(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offset.leftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offsetTop(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offset.topProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offsetRight(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offset.rightProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offsetBottom(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offset.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) x(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_requestedX);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) y(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_requestedY);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onOpen(this auto&& self, std::function<void()> callback) {
        self.m_onOpen = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onClose(this auto&& self, std::function<void()> callback) {
        self.m_onClose = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    bool opened() const { return m_opened; }
    ClosePolicy closePolicy() const { return m_closePolicy; }
    bool modal() const { return m_modal; }
    Placement placement() const { return m_placement; }
    PlacementTarget target() const { return m_target; }
    float offsetLeft() const { return m_offset.left(); }
    float offsetTop() const { return m_offset.top(); }
    float offsetRight() const { return m_offset.right(); }
    float offsetBottom() const { return m_offset.bottom(); }
    int x() const { return m_requestedX; }
    int y() const { return m_requestedY; }

    State<bool>& openedState() { return m_openedState; }

    void open();
    void close();

    bool isOverlay() const override { return true; }

protected:
    bool hasHandlerFor(EventType type) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;
    void placeOverlay() override;

private:
    using Control::content;

    void applyOpened(bool opened);

    Property<bool> m_opened{false};
    State<bool> m_openedState{false};
    Property<ClosePolicy> m_closePolicy{ClosePolicy::ClickOutside | ClosePolicy::Escape};
    Property<bool> m_modal{false};
    Property<Placement> m_placement{Placement::Bottom};
    Property<PlacementTarget> m_target{PlacementTarget::Parent};
    SidedProperty<float> m_offset{0.0f};
    Property<int> m_requestedX{0};
    Property<int> m_requestedY{0};
    bool m_hasRequestedX{false};
    bool m_hasRequestedY{false};

    std::function<void()> m_onOpen;
    std::function<void()> m_onClose;
};

}
