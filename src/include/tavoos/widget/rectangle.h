#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/widget.h>

namespace Tavoos {

class TAVOOS_EXPORT RectangleWidget : public Widget {
public:
    RectangleWidget(Object*);

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        radius.applyTo(self.m_radius.topLeftProperty());
        radius.applyTo(self.m_radius.topRightProperty());
        radius.applyTo(self.m_radius.bottomRightProperty());
        radius.applyTo(self.m_radius.bottomLeftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radiusTopLeft(this auto&& self, PropertyArg<int> radius) {
        radius.applyTo(self.m_radius.topLeftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radiusTopRight(this auto&& self, PropertyArg<int> radius) {
        radius.applyTo(self.m_radius.topRightProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radiusBottomRight(this auto&& self, PropertyArg<int> radius) {
        radius.applyTo(self.m_radius.bottomRightProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radiusBottomLeft(this auto&& self, PropertyArg<int> radius) {
        radius.applyTo(self.m_radius.bottomLeftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) color(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        width.applyTo(self.m_borderWidth);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_borderColor);
        return std::forward<decltype(self)>(self);
    }

    int radiusTopLeft() const { return m_radius.topLeft(); }
    int radiusTopRight() const { return m_radius.topRight(); }
    int radiusBottomRight() const { return m_radius.bottomRight(); }
    int radiusBottomLeft() const { return m_radius.bottomLeft(); }
    Paint color() const { return m_color; }
    float borderWidth() const { return m_borderWidth; }
    Paint borderColor() const { return m_borderColor; }

protected:
    void render(Renderer& r) override;

private:
    Property<Paint> m_color;
    CornerProperty<int> m_radius;
    Property<float> m_borderWidth{0.0f};
    Property<Paint> m_borderColor{Color::Black};
};

}