#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/font.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>

#include <string>

namespace Tavoos {

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
    float transition() const { return m_transition; }

    State<Paint>& colorState() { return m_color; }

private:
    void updateColor(bool animate);
    void updateTextColor(bool animate);

    Property<Paint> m_idleColor{Paint{Color::rgba(60, 130, 255)}};
    Property<Paint> m_hoverColor{Paint{Color::rgba(84, 148, 255)}};
    Property<Paint> m_disabledColor{Paint{Color::rgba(228, 229, 235)}};
    Property<float> m_transition{0.12f};
    AnimatedState<Paint> m_color;

    BindableState<std::string> m_text;
    BindableState<Font> m_font;
    Property<Paint> m_textColor{Paint{Color::White}};
    Property<Paint> m_disabledTextColor{Paint{Color::rgba(160, 163, 175)}};
    AnimatedState<Paint> m_textColorOut;
};

}
