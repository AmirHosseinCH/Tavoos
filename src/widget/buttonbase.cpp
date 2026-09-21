#include <tavoos/widget/buttonbase.h>

namespace Tavoos {

namespace {

void assign(State<bool>& state, bool value) {
    if (state.get() != value)
        state.set(value);
}

}

ButtonBase::ButtonBase(Object* parent) : Widget{parent} {
    m_enabled.onChange([this](const bool& enabled) { assign(m_enabledState, enabled); });
}

void ButtonBase::render(Renderer& renderer) {
    renderChildren(renderer);
}

bool ButtonBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::MouseClick:
    case EventType::MouseEnter:
    case EventType::MouseLeave:
        return true;
    default:
        return Widget::hasHandlerFor(type);
    }
}

void ButtonBase::triggerClick(MouseEvent&) {
}

void ButtonBase::triggerMouseEnter(MouseEvent& event) {
    assign(m_hovered, true);
    Widget::triggerMouseEnter(event);
}

void ButtonBase::triggerMouseLeave(MouseEvent& event) {
    assign(m_hovered, false);
    Widget::triggerMouseLeave(event);
}

void ButtonBase::triggerPress(MouseEvent& event) {
    if (!m_enabled)
        return;

    assign(m_hovered, true);
    Widget::triggerPress(event);
}

void ButtonBase::triggerRelease(MouseEvent& event) {
    if (!m_enabled)
        return;

    Widget::triggerRelease(event);

    if (m_hovered && event.button() == MouseButton::Left) {
        MouseEvent click{EventType::MouseClick, event.x(), event.y(), event.button(), event.modifiers()};
        Widget::triggerClick(click);
    }
}

}
