#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/control.h>
#include <tavoos/widget/style/progressbarstyle.h>

namespace Tavoos {

class TAVOOS_EXPORT ProgressBarWidget : public Control {
public:
    ProgressBarWidget(Object* parent);

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

    decltype(auto) trackColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_trackColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fillColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_fillColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        self.m_transition.set(seconds);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const ProgressBarStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<ProgressBarStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    int value() const { return m_value; }
    State<int>& valueState() { return m_valueState; }
    int minValue() const { return m_minValue; }
    int maxValue() const { return m_maxValue; }
    Paint trackColor() const { return m_trackColor; }
    Paint fillColor() const { return m_fillColor; }
    int radius() const { return m_radius; }
    float transition() const { return m_transition; }
    ProgressBarStyle style() const {
        return { m_trackColor.get(), m_fillColor.get(), m_radius.get(), m_transition.get() };
    }

private:
    void updatePosition();
    void applyStyle(const ProgressBarStyle& style);

    Property<int> m_value{0};
    State<int> m_valueState{0};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    BindableState<Paint> m_trackColor{Color::rgba(228, 229, 235)};
    BindableState<Paint> m_fillColor{Color::rgba(85, 112, 241)};
    BindableState<int> m_radius{4};
    BindableState<float> m_transition{0.15f};
    BindableState<ProgressBarStyle> m_style;

    State<float> m_position{0.0f};
};

}
