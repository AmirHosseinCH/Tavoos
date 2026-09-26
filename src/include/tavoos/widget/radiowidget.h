#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>
#include <tavoos/widget/radiogroup.h>
#include <tavoos/widget/radiostyle.h>

namespace Tavoos {

class TAVOOS_EXPORT RadioWidget : public ButtonBase {
public:
    RadioWidget(Object* parent);

    decltype(auto) selected(this auto&& self, PropertyArg<bool> selected) {
        selected.applyTo(self.m_selected);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) group(this auto&& self, RadioGroup& group) {
        self.m_group = &group;
        group.add(&self);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) unselectedColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_unselectedColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) selectedColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_selectedColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) unselectedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_unselectedBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) selectedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_selectedBorderColor);
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

    bool selected() const { return m_selected; }
    State<bool>& selectedState() { return m_selectedState; }
    Paint unselectedColor() const { return m_unselectedColor; }
    Paint selectedColor() const { return m_selectedColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint unselectedBorderColor() const { return m_unselectedBorderColor; }
    Paint selectedBorderColor() const { return m_selectedBorderColor; }
    Paint disabledBorderColor() const { return m_disabledBorderColor; }
    int radius() const { return m_radius; }
    float transition() const { return m_transition; }
    RadioStyle style() const {
        return { m_unselectedColor.get(), m_selectedColor.get(), m_disabledColor.get(),
                 m_unselectedBorderColor.get(), m_selectedBorderColor.get(), m_disabledBorderColor.get(),
                 m_radius.get(), m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;
    void handleClick(MouseEvent& event) override;

private:
    void updateColor(bool animate);
    void updateGeometry();
    void applyStyle(const RadioStyle& style);

    Property<bool> m_selected{false};
    State<bool> m_selectedState{false};
    Property<Paint> m_unselectedColor{Color::Transparent};
    Property<Paint> m_selectedColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_unselectedBorderColor{Color::rgba(205, 208, 218)};
    Property<Paint> m_selectedBorderColor{Color::rgba(109, 125, 205)};
    Property<Paint> m_disabledBorderColor{Color::rgba(220, 222, 230)};
    BindableState<int> m_radius{10};
    bool m_radiusOverridden{false};
    Property<float> m_transition{0.12f};
    BindableState<RadioStyle> m_style;
    RadioGroup* m_group{nullptr};

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_contentColorOut;
    AnimatedState<Paint> m_borderColorOut;
    AnimatedState<float> m_contentOpacity;
    State<int> m_contentRadius{7};
    bool m_settled{false};
};

}
