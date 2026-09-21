#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>

namespace Tavoos {

class TAVOOS_EXPORT ButtonWidget : public ButtonBase {
public:
    ButtonWidget(Object* parent);

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

    float transition() const { return m_transition; }

    State<Paint>& colorState() { return m_color; }

private:
    void updateColor(bool animate);

    Property<Paint> m_idleColor{Paint{Color::rgba(60, 130, 255)}};
    Property<Paint> m_hoverColor{Paint{Color::rgba(84, 148, 255)}};
    Property<Paint> m_disabledColor{Paint{Color::rgba(228, 229, 235)}};
    Property<float> m_transition{0.12f};
    AnimatedState<Paint> m_color;
};

}
