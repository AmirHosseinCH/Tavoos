#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/menustyle.h>
#include <tavoos/widget/templates/menubase.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuWidget : public MenuBase {
public:
    MenuWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_backgroundColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_borderColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        self.m_borderWidth.set(width);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const MenuStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<MenuStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    Paint borderColor() const { return m_borderColor; }
    float borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }
    MenuStyle style() const {
        return { m_backgroundColor.get(), m_borderColor.get(), m_borderWidth.get(), paddingLeft(), m_radius.get(),
                 enter(), exit() };
    }

private:
    void applyStyle(const MenuStyle& style);

    BindableState<Paint> m_backgroundColor{Color::rgba(255, 255, 255)};
    BindableState<Paint> m_borderColor{Color::rgba(220, 222, 230)};
    BindableState<float> m_borderWidth{1.0f};
    BindableState<int> m_radius{8};
    BindableState<MenuStyle> m_style;
};

}
