#include <tavoos/widget/templates/flickareabase.h>

#include <tavoos/events/key.h>
#include <tavoos/gfx/renderer.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

FlickAreaBase::FlickAreaBase(Object* parent) : Control{parent} {
    clip(true);
    background([](RectangleWidget& box) { box.color(Color::Transparent); });

    m_direction.onChange([this](const FlickDirection&) { syncOffsets(); });
    m_contentX.onChange([this](const float&) { syncOffsets(); });
    m_contentY.onChange([this](const float&) { syncOffsets(); });
    m_contentWidth.onChange([this](const float&) { syncOffsets(); });
    m_contentHeight.onChange([this](const float&) { syncOffsets(); });
}

float FlickAreaBase::viewportWidth() const {
    return std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight());
}

float FlickAreaBase::viewportHeight() const {
    return std::max(0.0f, resolvedHeight() - paddingTop() - paddingBottom());
}

Widget::Size FlickAreaBase::computeContentSize() {
    float extentWidth = 0.0f;
    float extentHeight = 0.0f;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || widget == backgroundSlot() || !widget->participatesInLayout())
            continue;

        const Size size = widget->intrinsicSize();
        const float childWidth = hasFlag(widget->fill(), Fill::Width) ? 0.0f : size.width;
        const float childHeight = hasFlag(widget->fill(), Fill::Height) ? 0.0f : size.height;
        extentWidth = std::max(extentWidth, widget->marginLeft() + static_cast<float>(widget->x()) + childWidth + widget->marginRight());
        extentHeight = std::max(extentHeight, widget->marginTop() + static_cast<float>(widget->y()) + childHeight + widget->marginBottom());
    }
    return { extentWidth, extentHeight };
}

float FlickAreaBase::contentWidth() {
    const float explicitWidth = m_contentWidth.get();
    return std::max(viewportWidth(), explicitWidth > 0.0f ? explicitWidth : computeContentSize().width);
}

float FlickAreaBase::contentHeight() {
    const float explicitHeight = m_contentHeight.get();
    return std::max(viewportHeight(), explicitHeight > 0.0f ? explicitHeight : computeContentSize().height);
}

float FlickAreaBase::maxContentX() {
    return hasFlag(m_direction.get(), FlickDirection::Horizontal) ? std::max(0.0f, contentWidth() - viewportWidth()) : 0.0f;
}

float FlickAreaBase::maxContentY() {
    return hasFlag(m_direction.get(), FlickDirection::Vertical) ? std::max(0.0f, contentHeight() - viewportHeight()) : 0.0f;
}

bool FlickAreaBase::scrollable() {
    return maxContentX() > 0.0f || maxContentY() > 0.0f;
}

void FlickAreaBase::syncOffsets() {
    const float x = std::clamp(std::round(m_contentX.get()), 0.0f, maxContentX());
    const float y = std::clamp(std::round(m_contentY.get()), 0.0f, maxContentY());
    const bool changed = x != m_contentXState.get() || y != m_contentYState.get();
    m_contentXState.setIfChanged(x);
    m_contentYState.setIfChanged(y);
    markLayoutDirty();
    if (changed && m_onScroll)
        m_onScroll(x, y);
}

void FlickAreaBase::onResolvedSizeChanged() {
    syncOffsets();
}

void FlickAreaBase::scrollTo(float x, float y) {
    m_contentX.set(x);
    m_contentY.set(y);
}

void FlickAreaBase::scrollBy(float dx, float dy) {
    scrollTo(m_contentXState.get() + dx, m_contentYState.get() + dy);
}

void FlickAreaBase::scrollToTop() {
    scrollTo(m_contentXState.get(), 0.0f);
}

void FlickAreaBase::scrollToBottom() {
    scrollTo(m_contentXState.get(), maxContentY());
}

void FlickAreaBase::scrollToLeft() {
    scrollTo(0.0f, m_contentYState.get());
}

void FlickAreaBase::scrollToRight() {
    scrollTo(maxContentX(), m_contentYState.get());
}

void FlickAreaBase::render(Renderer& renderer) {
    if (renderer.isCapturingMask()) {
        if (Widget* const shape = backgroundSlot())
            renderer.renderWidget(*shape);
        return;
    }
    renderChildren(renderer);
}

bool FlickAreaBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::Wheel:
        return scrollable();
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
        return m_dragScroll.get() && scrollable();
    case EventType::KeyPress:
        return m_keyNavigation.get() && scrollable();
    default:
        return Control::hasHandlerFor(type);
    }
}

void FlickAreaBase::triggerWheel(WheelEvent& event) {
    const float beforeX = m_contentXState.get();
    const float beforeY = m_contentYState.get();
    const float step = m_wheelStep.get();
    scrollBy(-event.deltaX() * step, -event.deltaY() * step);
    if (m_contentXState.get() == beforeX && m_contentYState.get() == beforeY)
        event.ignore();
    Widget::triggerWheel(event);
}

void FlickAreaBase::triggerDragMove(DragEvent& event) {
    scrollBy(-event.dx(), -event.dy());
    Widget::triggerDragMove(event);
}

void FlickAreaBase::triggerKeyPress(KeyEvent& event) {
    const float beforeX = m_contentXState.get();
    const float beforeY = m_contentYState.get();
    const float step = m_wheelStep.get();

    bool recognized = true;
    switch (event.keyCode()) {
    case static_cast<int>(Key::Up):       scrollBy(0.0f, -step); break;
    case static_cast<int>(Key::Down):     scrollBy(0.0f, step); break;
    case static_cast<int>(Key::Left):     scrollBy(-step, 0.0f); break;
    case static_cast<int>(Key::Right):    scrollBy(step, 0.0f); break;
    case static_cast<int>(Key::PageUp):   scrollBy(0.0f, -viewportHeight()); break;
    case static_cast<int>(Key::PageDown): scrollBy(0.0f, viewportHeight()); break;
    case static_cast<int>(Key::Home):     scrollToTop(); break;
    case static_cast<int>(Key::End):      scrollToBottom(); break;
    default:                              recognized = false; break;
    }

    if (!recognized || (m_contentXState.get() == beforeX && m_contentYState.get() == beforeY))
        event.ignore();
    Widget::triggerKeyPress(event);
}

Widget::ContentArea FlickAreaBase::contentAreaFor(const Widget& child) const {
    if (&child == backgroundSlot())
        return { 0.0f, 0.0f, displayedWidth(), displayedHeight() };

    const float areaWidth = m_contentWidth.get() > 0.0f ? std::max(viewportWidth(), m_contentWidth.get()) : viewportWidth();
    const float areaHeight = m_contentHeight.get() > 0.0f ? std::max(viewportHeight(), m_contentHeight.get()) : viewportHeight();
    return { paddingLeft() - m_contentXState.get(), paddingTop() - m_contentYState.get(), areaWidth, areaHeight };
}

}
