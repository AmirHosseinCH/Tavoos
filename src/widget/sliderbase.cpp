#include <tavoos/widget/sliderbase.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

void assign(State<bool>& state, bool value) {
    if (state.get() != value)
        state.set(value);
}

}

SliderBase::SliderBase(Object* parent) : Control{parent} {
    m_value.onChange([this](const int& value) {
        if (m_valueState.get() != value)
            m_valueState.set(value);
        updatePosition();
    });
    m_minValue.onChange([this](const int&) { updatePosition(); });
    m_maxValue.onChange([this](const int&) { updatePosition(); });
    enabledState().onChange([this](const bool& enabled) { focusable(enabled); });
    focusable(true);
    updatePosition();
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
        assign(m_pressed, true);
        if (!pointOnHandle(event.x()))
            m_value.set(valueFromPosition(event.x()));
    }
    Widget::triggerPress(event);
}

void SliderBase::triggerRelease(MouseEvent& event) {
    if (!enabled())
        return;

    if (event.button() == MouseButton::Left)
        assign(m_pressed, false);
    Widget::triggerRelease(event);
}

void SliderBase::triggerDragStart(DragEvent& event) {
    if (enabled())
        m_dragStartValue = m_value.get();
    Widget::triggerDragStart(event);
}

void SliderBase::triggerDragMove(DragEvent& event) {
    if (!enabled())
        return;

    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const float valuePerPixel = static_cast<float>(maxV - minV) / travel();
    const int newValue = m_dragStartValue + static_cast<int>(std::lround(event.totalDx() * valuePerPixel));
    m_value.set(std::clamp(newValue, minV, maxV));
    Widget::triggerDragMove(event);
}

void SliderBase::triggerDragEnd(DragEvent& event) {
    assign(m_pressed, false);
    Widget::triggerDragEnd(event);
}

float SliderBase::travel() const {
    const float handleWidth = m_handle ? m_handle->displayedWidth() : 0.0f;
    return std::max(1.0f, displayedWidth() - handleWidth);
}

int SliderBase::valueFromPosition(float x) const {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const float handleWidth = m_handle ? m_handle->displayedWidth() : 0.0f;
    const float fraction = std::clamp((x - handleWidth * 0.5f) / travel(), 0.0f, 1.0f);
    return minV + static_cast<int>(std::lround(fraction * static_cast<float>(maxV - minV)));
}

bool SliderBase::pointOnHandle(float x) const {
    return m_handle && x >= m_handle->resolvedX() && x <= m_handle->resolvedX() + m_handle->displayedWidth();
}

void SliderBase::updatePosition() {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int val = std::clamp(m_value.get(), minV, maxV);
    const float fraction = static_cast<float>(val - minV) / static_cast<float>(maxV - minV);

    if (m_position.get() != fraction)
        m_position.set(fraction);
}

}
