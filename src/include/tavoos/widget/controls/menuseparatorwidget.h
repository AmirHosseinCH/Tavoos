#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/menuseparatorstyle.h>
#include <tavoos/widget/templates/control.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuSeparatorWidget : public Control {
public:
    MenuSeparatorWidget(Object* parent);

    decltype(auto) color(this auto&& self, PropertyArg<Paint> color) {
        self.m_color.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thickness(this auto&& self, PropertyArg<int> thickness) {
        self.m_thickness.set(thickness);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) spacing(this auto&& self, PropertyArg<float> spacing) {
        self.m_spacing.set(spacing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const MenuSeparatorStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<MenuSeparatorStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint color() const { return m_color; }
    int thickness() const { return m_thickness; }
    float spacing() const { return m_spacing; }
    MenuSeparatorStyle style() const { return { m_color.get(), m_thickness.get(), m_spacing.get() }; }

private:
    void applyStyle(const MenuSeparatorStyle& style);

    BindableState<Paint> m_color{Color::rgba(228, 229, 235)};
    BindableState<int> m_thickness{1};
    BindableState<float> m_spacing{4.0f};
    BindableState<MenuSeparatorStyle> m_style;
};

}
