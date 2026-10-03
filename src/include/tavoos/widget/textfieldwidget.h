#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/style/textfieldstyle.h>
#include <tavoos/widget/textfieldbase.h>

namespace Tavoos {

class TAVOOS_EXPORT TextFieldWidget : public TextFieldBase {
public:
    TextFieldWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_backgroundColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_borderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) focusedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_focusedBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        self.m_borderWidth.set(width);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const TextFieldStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<TextFieldStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    Paint borderColor() const { return m_borderColor; }
    Paint focusedBorderColor() const { return m_focusedBorderColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint disabledBorderColor() const { return m_disabledBorderColor; }
    int radius() const { return m_radius; }
    float borderWidth() const { return m_borderWidth; }
    float transition() const { return m_transition; }
    TextFieldStyle style() const {
        return { m_backgroundColor.get(), m_borderColor.get(), m_focusedBorderColor.get(),
                 m_disabledColor.get(), m_disabledBorderColor.get(), textColor(),
                 placeholderColor(), caretColor(), m_radius.get(),
                 m_borderWidth.get(), innerPaddingLeft(), innerPaddingTop(),
                 innerPaddingRight(), innerPaddingBottom(), font(), m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;

private:
    void updateColor(bool animate);
    void applyStyle(const TextFieldStyle& style);

    Property<Paint> m_backgroundColor{Color::White};
    Property<Paint> m_borderColor{Color::rgba(205, 208, 218)};
    Property<Paint> m_focusedBorderColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_disabledBorderColor{Color::rgba(220, 222, 230)};
    BindableState<int> m_radius{6};
    BindableState<float> m_borderWidth{1.5f};
    Property<float> m_transition{0.12f};
    BindableState<TextFieldStyle> m_style;

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_borderColorOut;

    bool m_settled{false};
};

}
