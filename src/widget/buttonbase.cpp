#include <tavoos/widget/buttonbase.h>

namespace Tavoos {

namespace {

void assign(State<bool>& state, bool value) {
    if (state.get() != value)
        state.set(value);
}

}

ButtonBase::ButtonBase(Object* parent) : Widget{parent} {
    m_enabled.onChange([this](const bool& enabled) {
        assign(m_enabledState, enabled);
        focusable(enabled);
    });
    focusable(true);
}

void ButtonBase::render(Renderer& renderer) {
    renderChildren(renderer);
}

Widget::Size ButtonBase::computeIntrinsicSize() {
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

void ButtonBase::replaceSlot(Widget*& slot) {
    removeChild(slot);
    slot = nullptr;
    assign(m_hovered, false);
    assign(m_pressed, false);
}

bool ButtonBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::MouseClick:
    case EventType::MouseEnter:
    case EventType::MouseLeave:
    case EventType::KeyPress:
    case EventType::KeyRelease:
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
    assign(m_pressed, false);
    Widget::triggerMouseLeave(event);
}

void ButtonBase::triggerPress(MouseEvent& event) {
    if (!m_enabled)
        return;

    assign(m_hovered, true);
    if (event.button() == MouseButton::Left)
        assign(m_pressed, true);
    Widget::triggerPress(event);
}

void ButtonBase::triggerRelease(MouseEvent& event) {
    if (!m_enabled)
        return;

    Widget::triggerRelease(event);

    if (event.button() == MouseButton::Left)
        assign(m_pressed, false);

    if (m_hovered && event.button() == MouseButton::Left)
        sendClick(event.x(), event.y(), event.modifiers());
}

void ButtonBase::triggerKeyPress(KeyEvent& event) {
    if (!m_enabled)
        return;

    const int key = event.keyCode();
    if (key == static_cast<int>(Key::Space)) {
        m_spaceDown = true;
        assign(m_pressed, true);
    } else if (key == static_cast<int>(Key::Enter) || key == static_cast<int>(Key::KpEnter)) {
        sendClickFromKeyboard(event.modifiers());
    } else {
        event.ignore();
    }
    Widget::triggerKeyPress(event);
}

void ButtonBase::triggerKeyRelease(KeyEvent& event) {
    if (!m_enabled)
        return;

    if (event.keyCode() != static_cast<int>(Key::Space) || !m_spaceDown) {
        event.ignore();
        Widget::triggerKeyRelease(event);
        return;
    }

    m_spaceDown = false;
    assign(m_pressed, false);
    Widget::triggerKeyRelease(event);
    sendClickFromKeyboard(event.modifiers());
}

void ButtonBase::triggerFocusOut(Event& event) {
    m_spaceDown = false;
    assign(m_pressed, false);
    Widget::triggerFocusOut(event);
}

void ButtonBase::sendClick(float x, float y, KeyModifier modifiers) {
    MouseEvent click{EventType::MouseClick, x, y, MouseButton::Left, modifiers};
    Widget::triggerClick(click);
}

void ButtonBase::sendClickFromKeyboard(KeyModifier modifiers) {
    const glm::vec4 center = worldMatrix() * glm::vec4{displayedWidth() * 0.5f, displayedHeight() * 0.5f, 0.0f, 1.0f};
    sendClick(center.x, center.y, modifiers);
}

}
