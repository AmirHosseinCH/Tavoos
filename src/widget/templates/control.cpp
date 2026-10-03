#include <tavoos/widget/templates/control.h>

namespace Tavoos {

Control::Control(Object* parent) : Widget{parent} {
    m_enabled.onChange([this](const bool& enabled) { m_enabledState.setIfChanged(enabled); });
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
    m_hovered.setIfChanged(true);
    Widget::triggerMouseEnter(event);
}

void Control::triggerMouseLeave(MouseEvent& event) {
    m_hovered.setIfChanged(false);
    Widget::triggerMouseLeave(event);
}

void Control::onSlotReplaced() {
    m_hovered.setIfChanged(false);
}

void Control::replaceSlot(Widget*& slot) {
    removeChild(slot);
    slot = nullptr;
    onSlotReplaced();
}

}
