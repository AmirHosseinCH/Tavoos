#include <tavoos/widget/controls/switchwidget.h>

#include <tavoos/application.h>

#include <algorithm>

namespace Tavoos {

namespace {

constexpr int kThumbMargin = 3;

}

SwitchWidget::SwitchWidget(Object* parent) : ButtonBase{parent} {
    m_trackColorOut.set(m_uncheckedColor.get());
    m_thumbColorOut.set(m_thumbColor.get());

    checkable(true);
    checkedState().onChange([this](const bool&) {
        updateColor(true);
        updateThumbPosition();
    });
    enabledState().onChange([this](const bool&) { updateColor(true); });
    m_uncheckedColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_thumbColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedThumbColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledThumbColor.onChange([this](const Paint&) { updateColor(false); });

    heightProperty().onChange([this](const int&) { updateGeometry(); });

    width(36).height(20);
    updateGeometry();
    updateThumbPosition();

    background([this](RectangleWidget& box) {
        box.radius(m_trackRadius).color(m_trackColorOut);
    });

    content<RectangleWidget>([this](RectangleWidget& thumb) {
        thumb.width(m_thumbSize)
            .height(m_thumbSize)
            .radius(m_thumbRadius)
            .marginLeft(kThumbMargin)
            .marginRight(kThumbMargin)
            .alignment(m_thumbAlignment)
            .alignmentAnimation(true, m_transition.state())
            .color(m_thumbColorOut);
    });

    m_style.onChange([this](const SwitchStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().switchControl);
}

void SwitchWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

void SwitchWidget::updateColor(bool animate) {
    const bool isChecked = checked();
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& track = !isEnabled ? m_disabledColor.get() : (isChecked ? m_checkedColor.get() : m_uncheckedColor.get());
    const Paint& thumb = !isEnabled ? m_disabledThumbColor.get() : (isChecked ? m_checkedThumbColor.get() : m_thumbColor.get());

    m_trackColorOut.animateTo(track, duration);
    m_thumbColorOut.animateTo(thumb, duration);
}

void SwitchWidget::updateThumbPosition() {
    m_thumbAlignment.set(Alignment::CenterVertical | (checked() ? Alignment::Right : Alignment::Left));
}

void SwitchWidget::updateGeometry() {
    const int h = height();
    const int trackRadius = h / 2;
    if (m_trackRadius.get() != trackRadius)
        m_trackRadius.set(trackRadius);

    const int thumbSize = std::max(4, h - 2 * kThumbMargin);
    if (m_thumbSize.get() != thumbSize)
        m_thumbSize.set(thumbSize);

    const int thumbRadius = thumbSize / 2;
    if (m_thumbRadius.get() != thumbRadius)
        m_thumbRadius.set(thumbRadius);
}

void SwitchWidget::applyStyle(const SwitchStyle& value) {
    uncheckedColor(value.uncheckedColor);
    checkedColor(value.checkedColor);
    disabledColor(value.disabledColor);
    thumbColor(value.thumbColor);
    checkedThumbColor(value.checkedThumbColor);
    disabledThumbColor(value.disabledThumbColor);
    transition(value.transition);
}

}
