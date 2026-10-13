#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/scrollareastyle.h>
#include <tavoos/widget/templates/scrollareabase.h>

namespace Tavoos {

template<typename Base>
class ScrollAreaSkin : public Base {
public:
    ScrollAreaSkin(Object* parent) : Base{parent} {
        this->verticalThumbHoveredState().onChange([this](const bool&) { updateThumbColors(true); });
        this->verticalThumbPressedState().onChange([this](const bool&) { updateThumbColors(true); });
        this->horizontalThumbHoveredState().onChange([this](const bool&) { updateThumbColors(true); });
        this->horizontalThumbPressedState().onChange([this](const bool&) { updateThumbColors(true); });
        m_thumbColor.onChange([this](const Paint&) { updateThumbColors(false); });
        m_thumbHoverColor.onChange([this](const Paint&) { updateThumbColors(false); });
        m_thumbPressedColor.onChange([this](const Paint&) { updateThumbColors(false); });

        m_style.onChange([this](const ScrollAreaStyle& style) { applyStyle(style); });
    }

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
                 m_barMargin.get(), this->minThumbSize(), this->barHideDelay(), this->barFadeDuration(),
                 m_transition.get() };
    }

protected:
    void installDefaultSlots() {
        this->background([this](RectangleWidget& box) {
            box.radius(m_radius.state()).color(m_backgroundColor.state());
        });

        this->verticalTrack([this](RectangleWidget& track) {
            track.alignment(Alignment::Right).fill(Fill::Height).width(m_barThickness.state())
                .marginRight(m_barMargin.state()).marginTop(m_barMargin.state()).marginBottom(m_barMargin.state())
                .radius(m_thumbRadius.state()).color(m_trackColor.state());
        });

        this->verticalThumb([this](RectangleWidget& thumb) {
            thumb.alignment(Alignment::Right).width(m_barThickness.state())
                .marginRight(m_barMargin.state()).marginTop(m_barMargin.state()).marginBottom(m_barMargin.state())
                .heightFraction(this->verticalThumbSizeState()).yFraction(this->verticalThumbPositionState())
                .radius(m_thumbRadius.state()).color(m_verticalThumbColor);
        });

        this->horizontalTrack([this](RectangleWidget& track) {
            track.alignment(Alignment::Bottom).fill(Fill::Width).height(m_barThickness.state())
                .marginBottom(m_barMargin.state()).marginLeft(m_barMargin.state()).marginRight(m_barMargin.state())
                .radius(m_thumbRadius.state()).color(m_trackColor.state());
        });

        this->horizontalThumb([this](RectangleWidget& thumb) {
            thumb.alignment(Alignment::Bottom).height(m_barThickness.state())
                .marginBottom(m_barMargin.state()).marginLeft(m_barMargin.state()).marginRight(m_barMargin.state())
                .widthFraction(this->horizontalThumbSizeState()).xFraction(this->horizontalThumbPositionState())
                .radius(m_thumbRadius.state()).color(m_horizontalThumbColor);
        });
    }

private:
    void applyStyle(const ScrollAreaStyle& value) {
        backgroundColor(value.backgroundColor);
        trackColor(value.trackColor);
        thumbColor(value.thumbColor);
        thumbHoverColor(value.thumbHoverColor);
        thumbPressedColor(value.thumbPressedColor);
        radius(value.radius);
        thumbRadius(value.thumbRadius);
        barThickness(value.barThickness);
        barMargin(value.barMargin);
        this->minThumbSize(value.minThumbSize);
        this->barHideDelay(value.barHideDelay);
        this->barFadeDuration(value.barFadeDuration);
        transition(value.transition);
    }

    void updateThumbColors(bool animate) {
        const float duration = animate ? m_transition.get() : 0.0f;
        auto pick = [this](bool pressed, bool hovered) -> const Paint& {
            return pressed ? m_thumbPressedColor.get() : hovered ? m_thumbHoverColor.get() : m_thumbColor.get();
        };
        m_verticalThumbColor.animateTo(
            pick(this->verticalThumbPressedState().get(), this->verticalThumbHoveredState().get()), duration);
        m_horizontalThumbColor.animateTo(
            pick(this->horizontalThumbPressedState().get(), this->horizontalThumbHoveredState().get()), duration);
    }

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
