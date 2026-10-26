#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/controls/style/comboboxstyle.h>
#include <tavoos/widget/templates/comboboxbase.h>

namespace Tavoos {

class TAVOOS_EXPORT ComboBoxWidget : public ComboBoxBase {
public:
    ComboBoxWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_backgroundColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) hoverColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_hoverColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) pressedColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_pressedColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
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

    decltype(auto) textColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_textColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) placeholderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_placeholderColor);
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

    decltype(auto) indicatorColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_indicatorColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) highlightColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_highlightColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const ComboBoxStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<ComboBoxStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    Paint hoverColor() const { return m_hoverColor; }
    Paint pressedColor() const { return m_pressedColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint borderColor() const { return m_borderColor; }
    float borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }
    Paint textColor() const { return m_textColor; }
    Paint placeholderColor() const { return m_placeholderColor; }
    Paint disabledTextColor() const { return m_disabledTextColor; }
    Font font() const { return m_font; }
    Paint indicatorColor() const { return m_indicatorColor; }
    Paint highlightColor() const { return m_highlightColor; }
    float transition() const { return m_transition; }
    ComboBoxStyle style() const {
        return { m_backgroundColor.get(), m_hoverColor.get(), m_pressedColor.get(), m_disabledColor.get(),
                 m_borderColor.get(), m_borderWidth.get(), m_radius.get(),
                 m_textColor.get(), m_placeholderColor.get(), m_disabledTextColor.get(), m_font.get(),
                 m_indicatorColor.get(), m_highlightColor.get(), m_transition.get() };
    }

protected:
    void createToolTip() override;
    void render(Renderer& renderer) override;
    void styleRowText(TextWidget& label) override;

private:
    void updateColor(bool animate);
    void updateTextColor(bool animate);
    void applyStyle(const ComboBoxStyle& style);

    Property<Paint> m_backgroundColor{Color::White};
    Property<Paint> m_hoverColor{Color::rgba(246, 247, 250)};
    Property<Paint> m_pressedColor{Color::rgba(238, 240, 245)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    BindableState<Paint> m_borderColor{Color::rgba(205, 208, 218)};
    BindableState<float> m_borderWidth{1.5f};
    BindableState<int> m_radius{6};
    Property<Paint> m_textColor{Color::rgba(30, 32, 40)};
    Property<Paint> m_placeholderColor{Color::rgba(160, 163, 175)};
    Property<Paint> m_disabledTextColor{Color::rgba(160, 163, 175)};
    BindableState<Font> m_font;
    BindableState<Paint> m_indicatorColor{Color::rgba(110, 113, 125)};
    BindableState<Paint> m_highlightColor{Color::rgba(85, 112, 241, 40)};
    Property<float> m_transition{0.12f};
    BindableState<ComboBoxStyle> m_style;

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_textColorOut;
    State<Paint> m_rowTextColor{Color::rgba(30, 32, 40)};

    bool m_settled{false};
};

}
