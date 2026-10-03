#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/sliderbase.h>
#include <tavoos/widget/controls/style/sliderstyle.h>

namespace Tavoos {

class TAVOOS_EXPORT SliderWidget : public SliderBase {
public:
    SliderWidget(Object* parent);

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

private:
    void updateColor(bool animate);
    void applyStyle(const SliderStyle& style);

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
    bool m_settled{false};
};

}
