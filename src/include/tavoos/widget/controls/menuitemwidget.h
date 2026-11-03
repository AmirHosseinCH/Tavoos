#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/menuitemstyle.h>
#include <tavoos/widget/templates/menuitembase.h>

#include <string>

namespace Tavoos {

class TAVOOS_EXPORT MenuItemWidget : public MenuItemBase {
public:
    MenuItemWidget(Object* parent);

    decltype(auto) text(this auto&& self, PropertyArg<std::string> text) {
        self.m_text.set(text);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) highlightColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_highlightColor);
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

    decltype(auto) shortcutColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_shortcutColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checkColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_checkColor);
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

    decltype(auto) style(this auto&& self, const MenuItemStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<MenuItemStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    std::string text() const { return m_text.get(); }
    Paint highlightColor() const { return m_highlightColor; }
    Paint textColor() const { return m_textColor; }
    Paint disabledTextColor() const { return m_disabledTextColor; }
    Paint shortcutColor() const { return m_shortcutColor; }
    Paint checkColor() const { return m_checkColor; }
    Font font() const { return m_font; }
    int radius() const { return m_radius; }
    MenuItemStyle style() const {
        return { m_highlightColor.get(), m_textColor.get(), m_disabledTextColor.get(), m_shortcutColor.get(),
                 m_checkColor.get(), m_font.get(), m_radius.get() };
    }

protected:
    void createToolTip() override;

private:
    void updateColors();
    void applyStyle(const MenuItemStyle& style);

    BindableState<std::string> m_text;
    Property<Paint> m_highlightColor{Color::rgba(85, 112, 241, 40)};
    Property<Paint> m_textColor{Color::rgba(30, 32, 40)};
    Property<Paint> m_disabledTextColor{Color::rgba(160, 163, 175)};
    Property<Paint> m_shortcutColor{Color::rgba(130, 133, 145)};
    Property<Paint> m_checkColor{Color::rgba(85, 112, 241)};
    BindableState<Font> m_font;
    BindableState<int> m_radius{6};
    BindableState<MenuItemStyle> m_style;

    State<Paint> m_backgroundOut{Color::Transparent};
    State<Paint> m_textOut{Color::rgba(30, 32, 40)};
    State<Paint> m_shortcutOut{Color::rgba(130, 133, 145)};
    State<Paint> m_checkOut{Color::Transparent};
    State<std::string> m_shortcutText{""};
    State<bool> m_hasShortcut{false};
};

}
