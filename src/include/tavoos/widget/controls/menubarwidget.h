#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/menubarstyle.h>
#include <tavoos/widget/templates/menubarbase.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuBarWidget : public MenuBarBase {
public:
    MenuBarWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_backgroundColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const MenuBarStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<MenuBarStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    MenuBarStyle style() const { return { m_backgroundColor.get(), paddingLeft() }; }

protected:
    MenuBarItemBase* createItem() override;

private:
    void applyStyle(const MenuBarStyle& style);

    BindableState<Paint> m_backgroundColor{Color::rgba(248, 249, 251)};
    BindableState<MenuBarStyle> m_style;
};

}
