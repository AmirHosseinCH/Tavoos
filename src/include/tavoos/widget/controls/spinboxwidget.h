#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/controls/buttonwidget.h>
#include <tavoos/widget/templates/spinboxbase.h>
#include <tavoos/widget/controls/style/spinboxstyle.h>
#include <tavoos/widget/controls/textfieldwidget.h>

#include <functional>
#include <string>

namespace Tavoos {

class TAVOOS_EXPORT SpinBoxWidget : public SpinBoxBase {
public:
    SpinBoxWidget(Object* parent);

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

    decltype(auto) style(this auto&& self, const SpinBoxStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<SpinBoxStyle>& style) {
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
    SpinBoxStyle style() const {
        return { m_backgroundColor.get(), m_borderColor.get(), m_focusedBorderColor.get(),
                 m_disabledColor.get(), m_disabledBorderColor.get(), m_radius.get(),
                 m_borderWidth.get(), m_transition.get(),
                 m_field ? m_field->style() : TextFieldStyle{},
                 m_upButton ? m_upButton->style() : ButtonStyle{} };
    }

protected:
    void render(Renderer& renderer) override;

private:
    void updateEnabled();
    void updateColor(bool animate);
    void applyStyle(const SpinBoxStyle& style);

    Property<Paint> m_backgroundColor{Color::White};
    Property<Paint> m_borderColor{Color::rgba(205, 208, 218)};
    Property<Paint> m_focusedBorderColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_disabledBorderColor{Color::rgba(220, 222, 230)};
    BindableState<int> m_radius{6};
    BindableState<float> m_borderWidth{1.5f};
    Property<float> m_transition{0.12f};
    BindableState<SpinBoxStyle> m_style;

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_borderColorOut;

    TextFieldWidget* m_field{nullptr};
    ButtonWidget* m_upButton{nullptr};
    ButtonWidget* m_downButton{nullptr};
    bool m_settled{false};
};

}
