#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/font.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>

#include <string>

namespace Tavoos {

enum class ButtonVariant { Filled, Outlined, Text };

enum class ButtonIconPosition { Left, Right };

enum class ButtonDisplay { TextAndIcon, TextOnly, IconOnly };

class TAVOOS_EXPORT ButtonWidget : public ButtonBase {
public:
    ButtonWidget(Object* parent);

    decltype(auto) text(this auto&& self, PropertyArg<std::string> text) {
        self.m_text.set(text);
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

    decltype(auto) variant(this auto&& self, PropertyArg<ButtonVariant> variant) {
        variant.applyTo(self.m_variant);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        self.m_borderWidth.set(width);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_borderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) icon(this auto&& self, PropertyArg<std::string> source) {
        self.m_icon.set(source);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) iconSize(this auto&& self, PropertyArg<int> size) {
        self.m_iconSize.set(size);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) iconPosition(this auto&& self, PropertyArg<ButtonIconPosition> position) {
        position.applyTo(self.m_iconPosition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) display(this auto&& self, PropertyArg<ButtonDisplay> display) {
        display.applyTo(self.m_display);
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

    decltype(auto) disabledTextColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledTextColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) idleColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_idleColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) hoverColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_hoverColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    std::string text() const { return m_text; }
    Font font() const { return m_font; }
    int radius() const { return m_radius; }
    ButtonVariant variant() const { return m_variant; }
    float borderWidth() const { return m_borderWidth; }
    std::string icon() const { return m_icon; }
    int iconSize() const { return m_iconSize; }
    ButtonIconPosition iconPosition() const { return m_iconPosition; }
    ButtonDisplay display() const { return m_display; }
    float transition() const { return m_transition; }

    State<Paint>& colorState() { return m_color; }

private:
    void updateColor(bool animate);
    void updateTextColor(bool animate);
    void updateBorderColor(bool animate);
    void applyVariant(ButtonVariant variant);
    void rebuildContent();

    Property<Paint> m_idleColor{Paint{Color::rgba(60, 130, 255)}};
    Property<Paint> m_hoverColor{Paint{Color::rgba(84, 148, 255)}};
    Property<Paint> m_disabledColor{Paint{Color::rgba(228, 229, 235)}};
    Property<float> m_transition{0.12f};
    AnimatedState<Paint> m_color;

    BindableState<std::string> m_text;
    BindableState<Font> m_font;
    BindableState<int> m_radius{8};
    Property<ButtonVariant> m_variant{ButtonVariant::Filled};
    BindableState<float> m_borderWidth{0.0f};
    Property<Paint> m_borderColor{Paint{Color::rgba(205, 208, 218)}};
    Property<Paint> m_disabledBorderColor{Paint{Color::rgba(220, 222, 230)}};
    AnimatedState<Paint> m_borderColorOut;
    BindableState<std::string> m_icon;
    BindableState<int> m_iconSize{16};
    Property<ButtonIconPosition> m_iconPosition{ButtonIconPosition::Left};
    Property<ButtonDisplay> m_display{ButtonDisplay::TextAndIcon};
    bool m_hasIcon{false};
    unsigned m_ownRevision{0};
    Property<Paint> m_textColor{Paint{Color::White}};
    Property<Paint> m_disabledTextColor{Paint{Color::rgba(160, 163, 175)}};
    AnimatedState<Paint> m_textColorOut;
};

}
