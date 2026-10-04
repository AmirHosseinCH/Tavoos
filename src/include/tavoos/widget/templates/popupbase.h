#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/overlaybase.h>

namespace Tavoos {

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

class TAVOOS_EXPORT PopupBase : public OverlayBase {
public:
    PopupBase(Object* parent);

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

    Placement placement() const { return m_placement; }
    PlacementTarget target() const { return m_target; }
    float offsetLeft() const { return m_offset.left(); }
    float offsetTop() const { return m_offset.top(); }
    float offsetRight() const { return m_offset.right(); }
    float offsetBottom() const { return m_offset.bottom(); }
    int x() const { return m_requestedX; }
    int y() const { return m_requestedY; }

    void place() override;

private:
    Property<Placement> m_placement{Placement::Bottom};
    Property<PlacementTarget> m_target{PlacementTarget::Parent};
    SidedProperty<float> m_offset{0.0f};
    Property<int> m_requestedX{0};
    Property<int> m_requestedY{0};
    bool m_hasRequestedX{false};
    bool m_hasRequestedY{false};
};

}
