#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/scrollareastyle.h>
#include <tavoos/widget/templates/scrollareabase.h>

namespace Tavoos {

class TAVOOS_EXPORT ScrollAreaWidget : public ScrollAreaBase {
public:
    ScrollAreaWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_backgroundColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) trackColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_trackColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_thumbColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbHoverColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_thumbHoverColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbPressedColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_thumbPressedColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbRadius(this auto&& self, PropertyArg<int> radius) {
        self.m_thumbRadius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) barThickness(this auto&& self, PropertyArg<int> thickness) {
        self.m_barThickness.set(thickness);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) barMargin(this auto&& self, PropertyArg<float> margin) {
        self.m_barMargin.set(margin);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        self.m_transition.set(seconds);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const ScrollAreaStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<ScrollAreaStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    Paint trackColor() const { return m_trackColor; }
    Paint thumbColor() const { return m_thumbColor; }
    Paint thumbHoverColor() const { return m_thumbHoverColor; }
    Paint thumbPressedColor() const { return m_thumbPressedColor; }
    int radius() const { return m_radius; }
    int thumbRadius() const { return m_thumbRadius; }
    int barThickness() const { return m_barThickness; }
    float barMargin() const { return m_barMargin; }
    float transition() const { return m_transition; }
    ScrollAreaStyle style() const {
        return { m_backgroundColor.get(), m_trackColor.get(), m_thumbColor.get(), m_thumbHoverColor.get(),
                 m_thumbPressedColor.get(), m_radius.get(), m_thumbRadius.get(), m_barThickness.get(),
                 m_barMargin.get(), minThumbSize(), barHideDelay(), barFadeDuration(), m_transition.get() };
    }

private:
    void applyStyle(const ScrollAreaStyle& style);
    void updateThumbColors(bool animate);

    BindableState<Paint> m_backgroundColor{Color::Transparent};
    BindableState<Paint> m_trackColor{Color::Transparent};
    BindableState<Paint> m_thumbColor{Color::rgba(0, 0, 0, 90)};
    BindableState<Paint> m_thumbHoverColor{Color::rgba(0, 0, 0, 140)};
    BindableState<Paint> m_thumbPressedColor{Color::rgba(0, 0, 0, 190)};
    BindableState<int> m_radius{0};
    BindableState<int> m_thumbRadius{4};
    BindableState<int> m_barThickness{8};
    BindableState<float> m_barMargin{2.0f};
    BindableState<float> m_transition{0.12f};
    BindableState<ScrollAreaStyle> m_style;

    AnimatedState<Paint> m_verticalThumbColor{Color::rgba(0, 0, 0, 90)};
    AnimatedState<Paint> m_horizontalThumbColor{Color::rgba(0, 0, 0, 90)};
};

}
