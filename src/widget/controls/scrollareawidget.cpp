#include <tavoos/widget/controls/scrollareawidget.h>

#include <tavoos/application.h>

namespace Tavoos {

ScrollAreaWidget::ScrollAreaWidget(Object* parent) : ScrollAreaBase{parent} {
    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundColor.state());
    });

    verticalTrack([this](RectangleWidget& track) {
        track.alignment(Alignment::Right).fill(Fill::Height).width(m_barThickness.state())
            .marginRight(m_barMargin.state()).marginTop(m_barMargin.state()).marginBottom(m_barMargin.state())
            .radius(m_thumbRadius.state()).color(m_trackColor.state());
    });

    verticalThumb([this](RectangleWidget& thumb) {
        thumb.alignment(Alignment::Right).width(m_barThickness.state())
            .marginRight(m_barMargin.state()).marginTop(m_barMargin.state()).marginBottom(m_barMargin.state())
            .heightFraction(verticalThumbSizeState()).yFraction(verticalThumbPositionState())
            .radius(m_thumbRadius.state()).color(m_verticalThumbColor);
    });

    horizontalTrack([this](RectangleWidget& track) {
        track.alignment(Alignment::Bottom).fill(Fill::Width).height(m_barThickness.state())
            .marginBottom(m_barMargin.state()).marginLeft(m_barMargin.state()).marginRight(m_barMargin.state())
            .radius(m_thumbRadius.state()).color(m_trackColor.state());
    });

    horizontalThumb([this](RectangleWidget& thumb) {
        thumb.alignment(Alignment::Bottom).height(m_barThickness.state())
            .marginBottom(m_barMargin.state()).marginLeft(m_barMargin.state()).marginRight(m_barMargin.state())
            .widthFraction(horizontalThumbSizeState()).xFraction(horizontalThumbPositionState())
            .radius(m_thumbRadius.state()).color(m_horizontalThumbColor);
    });

    verticalThumbHoveredState().onChange([this](const bool&) { updateThumbColors(true); });
    verticalThumbPressedState().onChange([this](const bool&) { updateThumbColors(true); });
    horizontalThumbHoveredState().onChange([this](const bool&) { updateThumbColors(true); });
    horizontalThumbPressedState().onChange([this](const bool&) { updateThumbColors(true); });
    m_thumbColor.onChange([this](const Paint&) { updateThumbColors(false); });
    m_thumbHoverColor.onChange([this](const Paint&) { updateThumbColors(false); });
    m_thumbPressedColor.onChange([this](const Paint&) { updateThumbColors(false); });

    m_style.onChange([this](const ScrollAreaStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().scrollArea);
}

void ScrollAreaWidget::applyStyle(const ScrollAreaStyle& value) {
    backgroundColor(value.backgroundColor);
    trackColor(value.trackColor);
    thumbColor(value.thumbColor);
    thumbHoverColor(value.thumbHoverColor);
    thumbPressedColor(value.thumbPressedColor);
    radius(value.radius);
    thumbRadius(value.thumbRadius);
    barThickness(value.barThickness);
    barMargin(value.barMargin);
    minThumbSize(value.minThumbSize);
    barHideDelay(value.barHideDelay);
    barFadeDuration(value.barFadeDuration);
    transition(value.transition);
}

void ScrollAreaWidget::updateThumbColors(bool animate) {
    const float duration = animate ? m_transition.get() : 0.0f;
    auto pick = [this](bool pressed, bool hovered) -> const Paint& {
        return pressed ? m_thumbPressedColor.get() : hovered ? m_thumbHoverColor.get() : m_thumbColor.get();
    };
    m_verticalThumbColor.animateTo(pick(verticalThumbPressedState().get(), verticalThumbHoveredState().get()), duration);
    m_horizontalThumbColor.animateTo(pick(horizontalThumbPressedState().get(), horizontalThumbHoveredState().get()), duration);
}

}
