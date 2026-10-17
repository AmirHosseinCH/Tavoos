#pragma once

#include <tavoos/effects/graphicaleffect.h>
#include <tavoos/effects/shadowstyle.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <utility>

namespace Tavoos {

class TAVOOS_EXPORT ShadowBase : public GraphicalEffect {
public:
    decltype(auto) offsetX(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offsetX);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offsetY(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_offsetY);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) offset(this auto&& self, PropertyArg<float> x, PropertyArg<float> y) {
        x.applyTo(self.m_offsetX);
        y.applyTo(self.m_offsetY);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) spread(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_spread);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) color(this auto&& self, PropertyArg<Color> value) {
        value.applyTo(self.m_color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const ShadowStyle& value) {
        self.m_style.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<ShadowStyle>& value) {
        self.m_style.set(value);
        return std::forward<decltype(self)>(self);
    }

    ShadowStyle style() const { return { m_offsetX, m_offsetY, m_radius, m_spread, m_color }; }

    float offsetX() const { return m_offsetX; }
    float offsetY() const { return m_offsetY; }
    float radius() const { return m_radius; }
    float spread() const { return m_spread; }
    Color color() const { return m_color; }

protected:
    explicit ShadowBase(Widget& owner);

private:
    void applyStyle(const ShadowStyle& value);

    BindableState<ShadowStyle> m_style;
    Property<float> m_offsetX{0.0f};
    Property<float> m_offsetY{0.0f};
    Property<float> m_radius{0.0f};
    Property<float> m_spread{0.0f};
    Property<Color> m_color{Color::rgba(0, 0, 0, 64)};
};

}
