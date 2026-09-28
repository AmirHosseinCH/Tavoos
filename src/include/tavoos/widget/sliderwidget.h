#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>
#include <tavoos/widget/style/sliderstyle.h>

namespace Tavoos {

class TAVOOS_EXPORT SliderWidget : public ButtonBase {
public:
    SliderWidget(Object* parent);

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
        color.applyTo(self.m_trackColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fillColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_fillColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) thumbColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_thumbColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledThumbColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledThumbColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const SliderStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<SliderStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    int value() const { return m_value; }
    State<int>& valueState() { return m_valueState; }
    int minValue() const { return m_minValue; }
    int maxValue() const { return m_maxValue; }
    Paint trackColor() const { return m_trackColor; }
    Paint fillColor() const { return m_fillColor; }
    Paint thumbColor() const { return m_thumbColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint disabledThumbColor() const { return m_disabledThumbColor; }
    float transition() const { return m_transition; }
    SliderStyle style() const {
        return { m_trackColor.get(), m_fillColor.get(), m_thumbColor.get(),
                 m_disabledColor.get(), m_disabledThumbColor.get(), m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;
    void handleClick(MouseEvent& event) override;

private:
    void updateColor(bool animate);
    void updateGeometry();
    void applyStyle(const SliderStyle& style);
    int valueFromPosition(float x) const;

    Property<int> m_value{0};
    State<int> m_valueState{0};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    Property<Paint> m_trackColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_fillColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_thumbColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_disabledThumbColor{Color::rgba(160, 163, 175)};
    Property<float> m_transition{0.12f};
    BindableState<SliderStyle> m_style;

    AnimatedState<Paint> m_trackColorOut;
    AnimatedState<Paint> m_fillColorOut;
    AnimatedState<Paint> m_thumbColorOut;
    State<int> m_fillWidth{0};
    State<int> m_thumbX{0};

    int m_dragStartValue{0};
    bool m_settled{false};
};

}
