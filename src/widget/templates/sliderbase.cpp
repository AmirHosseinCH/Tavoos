#include <tavoos/widget/templates/sliderbase.h>

#include <tavoos/widget/templates/rangemath.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

SliderBase::SliderBase(Object* parent) : RangeBase{parent} {
    enabledState().onChange([this](const bool& enabled) { focusable(enabled); });
    focusable(true);
}

bool SliderBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
        return true;
    default:
        return Control::hasHandlerFor(type);
    }
}

void SliderBase::triggerPress(MouseEvent& event) {
    if (!enabled())
        return;

    if (event.button() == MouseButton::Left) {
        m_pressed.setIfChanged(true);
        if (!pointOnHandle(event.x()))
            commitValue(valueFromPosition(event.x()));
    }
    Widget::triggerPress(event);
}

void SliderBase::triggerRelease(MouseEvent& event) {
    if (!enabled())
        return;

    if (event.button() == MouseButton::Left)
        m_pressed.setIfChanged(false);
    Widget::triggerRelease(event);
}

void SliderBase::triggerDragStart(DragEvent& event) {
    if (enabled())
        m_dragStartValue = value();
    Widget::triggerDragStart(event);
}

void SliderBase::triggerDragMove(DragEvent& event) {
    if (!enabled())
        return;

    const long long span = detail::spanOf(minValue(), maxValue());
    const double valuePerPixel = static_cast<double>(span) / static_cast<double>(travel());
    const long long newValue = static_cast<long long>(m_dragStartValue) + std::llround(event.totalDx() * valuePerPixel);
    commitValue(newValue);
    Widget::triggerDragMove(event);
}

void SliderBase::triggerDragEnd(DragEvent& event) {
    m_pressed.setIfChanged(false);
    Widget::triggerDragEnd(event);
}

float SliderBase::travel() const {
    const float handleWidth = m_handle ? m_handle->displayedWidth() : 0.0f;
    return std::max(1.0f, displayedWidth() - handleWidth);
}

int SliderBase::valueFromPosition(float x) const {
    const long long minV = minValue();
    const long long span = detail::spanOf(minV, maxValue());
    const float handleWidth = m_handle ? m_handle->displayedWidth() : 0.0f;
    const double fraction = std::clamp((x - handleWidth * 0.5f) / travel(), 0.0f, 1.0f);
    return static_cast<int>(detail::clampToRange(minV + std::llround(fraction * static_cast<double>(span)), minV, maxValue()));
}

bool SliderBase::pointOnHandle(float x) const {
    return m_handle && x >= m_handle->resolvedX() && x <= m_handle->resolvedX() + m_handle->displayedWidth();
}

}
