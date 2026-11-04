#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/menubaritemstyle.h>
#include <tavoos/widget/templates/menubaritembase.h>
#include <tavoos/widget/text.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuBarItemWidget : public MenuBarItemBase {
public:
    MenuBarItemWidget(Object* parent);

    decltype(auto) highlightColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_highlightColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) hoverColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_hoverColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) textColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_textColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledTextColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledTextColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, const Font& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, State<Font>& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const MenuBarItemStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<MenuBarItemStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint highlightColor() const { return m_highlightColor; }
    Paint hoverColor() const { return m_hoverColor; }
    Paint textColor() const { return m_textColor; }
    Paint disabledTextColor() const { return m_disabledTextColor; }
    Font font() const { return m_font; }
    int radius() const { return m_radius; }
    MenuBarItemStyle style() const {
        return { m_highlightColor.get(), m_hoverColor.get(), m_textColor.get(), m_disabledTextColor.get(),
                 m_font.get(), m_radius.get() };
    }

protected:
    void createToolTip() override;
    void bindMenu(MenuBase& menu) override;

private:
    void updateColors();
    void applyStyle(const MenuBarItemStyle& style);

    Property<Paint> m_highlightColor{Color::rgba(85, 112, 241, 40)};
    Property<Paint> m_hoverColor{Color::rgba(30, 32, 40, 14)};
    Property<Paint> m_textColor{Color::rgba(30, 32, 40)};
    Property<Paint> m_disabledTextColor{Color::rgba(160, 163, 175)};
    BindableState<Font> m_font;
    BindableState<int> m_radius{6};
    BindableState<MenuBarItemStyle> m_style;

    State<Paint> m_backgroundOut{Color::Transparent};
    State<Paint> m_textOut{Color::rgba(30, 32, 40)};
    TextWidget* m_label{nullptr};
};

}
