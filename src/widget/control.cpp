#include <tavoos/widget/control.h>

namespace Tavoos {

namespace {

void assign(State<bool>& state, bool value) {
    if (state.get() != value)
        state.set(value);
}

}

Control::Control(Object* parent) : Widget{parent} {
    m_enabled.onChange([this](const bool& enabled) { assign(m_enabledState, enabled); });
}

void Control::render(Renderer& renderer) {
    renderChildren(renderer);
}

Widget::Size Control::computeIntrinsicSize() {
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    if (m_content) {
        const Size size = m_content->intrinsicSize();
        contentWidth = size.width + m_content->marginLeft() + m_content->marginRight();
        contentHeight = size.height + m_content->marginTop() + m_content->marginBottom();
    }
    return { (width() > 0) ? static_cast<float>(width()) : contentWidth,
             (height() > 0) ? static_cast<float>(height()) : contentHeight };
}

bool Control::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MouseEnter:
    case EventType::MouseLeave:
        return true;
    default:
        return Widget::hasHandlerFor(type);
    }
}

void Control::triggerMouseEnter(MouseEvent& event) {
    assign(m_hovered, true);
    Widget::triggerMouseEnter(event);
}

void Control::triggerMouseLeave(MouseEvent& event) {
    assign(m_hovered, false);
    Widget::triggerMouseLeave(event);
}

void Control::onSlotReplaced() {
    assign(m_hovered, false);
}

void Control::replaceSlot(Widget*& slot) {
    removeChild(slot);
    slot = nullptr;
    onSlotReplaced();
}

}
