#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/controls/style/switchstyle.h>

namespace Tavoos {

class TAVOOS_EXPORT SwitchWidget : public ButtonBase {
public:
    SwitchWidget(Object* parent);

    decltype(auto) uncheckedColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_uncheckedColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checkedColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_checkedColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_thumbColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checkedThumbColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_checkedThumbColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledThumbColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledThumbColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        self.m_transition.set(seconds);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const SwitchStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<SwitchStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint uncheckedColor() const { return m_uncheckedColor; }
    Paint checkedColor() const { return m_checkedColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint thumbColor() const { return m_thumbColor; }
    Paint checkedThumbColor() const { return m_checkedThumbColor; }
    Paint disabledThumbColor() const { return m_disabledThumbColor; }
    float transition() const { return m_transition; }
    SwitchStyle style() const {
        return { m_uncheckedColor.get(), m_checkedColor.get(), m_disabledColor.get(),
                 m_thumbColor.get(), m_checkedThumbColor.get(), m_disabledThumbColor.get(),
                 m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;

private:
    void updateColor(bool animate);
    void updateThumbPosition();
    void updateGeometry();
    void applyStyle(const SwitchStyle& style);

    Property<Paint> m_uncheckedColor{Color::rgba(234, 237, 253)};
    Property<Paint> m_checkedColor{Color::rgba(187, 198, 249)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_thumbColor{Color::rgba(187, 197, 203)};
    Property<Paint> m_checkedThumbColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledThumbColor{Color::rgba(160, 163, 175)};
    BindableState<float> m_transition{0.12f};
    BindableState<SwitchStyle> m_style;

    AnimatedState<Paint> m_trackColorOut;
    AnimatedState<Paint> m_thumbColorOut;
    State<Alignment> m_thumbAlignment{Alignment::Left | Alignment::CenterVertical};
    State<int> m_trackRadius{10};
    State<int> m_thumbSize{14};
    State<int> m_thumbRadius{7};
    bool m_settled{false};
};

}
