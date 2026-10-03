#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/buttonwidget.h>
#include <tavoos/widget/control.h>
#include <tavoos/widget/style/spinboxstyle.h>
#include <tavoos/widget/textfieldwidget.h>

#include <functional>
#include <string>

namespace Tavoos {

class TAVOOS_EXPORT SpinBoxWidget : public Control {
public:
    SpinBoxWidget(Object* parent);

    decltype(auto) value(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) minValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_minValue);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) maxValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_maxValue);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) step(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_step);
        return std::forward<decltype(self)>(self);
    }

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

    decltype(auto) onValueChange(this auto&& self, std::function<void(int)> callback) {
        self.m_onValueChange = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    int value() const { return m_value; }
    State<int>& valueState() { return m_valueState; }
    int minValue() const { return m_minValue; }
    int maxValue() const { return m_maxValue; }
    int step() const { return m_step; }
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
    void setValue(int newValue);
    void adjustValue(int delta);
    void commitTypedValue(const std::string& text);
    void updateDisplayedValue();
    void updateEnabled();
    void updateColor(bool animate);
    void applyStyle(const SpinBoxStyle& style);

    Property<int> m_value{0};
    State<int> m_valueState{0};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    Property<int> m_step{1};

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

    std::function<void(int)> m_onValueChange;

    TextFieldWidget* m_field{nullptr};
    ButtonWidget* m_upButton{nullptr};
    ButtonWidget* m_downButton{nullptr};
    bool m_settled{false};
};

}
