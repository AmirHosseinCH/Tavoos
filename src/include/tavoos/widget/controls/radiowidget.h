#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/controls/style/radiostyle.h>

namespace Tavoos {

class TAVOOS_EXPORT RadioWidget : public ButtonBase {
public:
    RadioWidget(Object* parent);

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

    decltype(auto) uncheckedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_uncheckedBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checkedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_checkedBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radiusOverridden = true;
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const RadioStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<RadioStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint uncheckedColor() const { return m_uncheckedColor; }
    Paint checkedColor() const { return m_checkedColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint uncheckedBorderColor() const { return m_uncheckedBorderColor; }
    Paint checkedBorderColor() const { return m_checkedBorderColor; }
    Paint disabledBorderColor() const { return m_disabledBorderColor; }
    int radius() const { return m_radius; }
    float transition() const { return m_transition; }
    RadioStyle style() const {
        return { m_uncheckedColor.get(), m_checkedColor.get(), m_disabledColor.get(),
                 m_uncheckedBorderColor.get(), m_checkedBorderColor.get(), m_disabledBorderColor.get(),
                 m_radius.get(), m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;
    void onResolvedSizeChanged() override;

private:
    void updateColor(bool animate);
    void updateGeometry();
    void applyStyle(const RadioStyle& style);

    Property<Paint> m_uncheckedColor{Color::Transparent};
    Property<Paint> m_checkedColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_uncheckedBorderColor{Color::rgba(205, 208, 218)};
    Property<Paint> m_checkedBorderColor{Color::rgba(109, 125, 205)};
    Property<Paint> m_disabledBorderColor{Color::rgba(220, 222, 230)};
    BindableState<int> m_radius{10};
    bool m_radiusOverridden{false};
    Property<float> m_transition{0.12f};
    BindableState<RadioStyle> m_style;

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_contentColorOut;
    AnimatedState<Paint> m_borderColorOut;
    AnimatedState<float> m_contentOpacity;
    State<int> m_contentRadius{7};
    bool m_settled{false};
};

}
