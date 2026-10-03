#include <tavoos/widget/templates/buttonbase.h>

namespace Tavoos {

namespace {

void assign(State<bool>& state, bool value) {
    if (state.get() != value)
        state.set(value);
}

}

ButtonBase::ButtonBase(Object* parent) : Control{parent} {
    enabledState().onChange([this](const bool& enabled) { focusable(enabled); });
    m_checked.onChange([this](const bool& checked) { assign(m_checkedState, checked); });
    focusable(true);
}

void ButtonBase::onSlotReplaced() {
    Control::onSlotReplaced();
    assign(m_pressed, false);
}

bool ButtonBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::MouseClick:
    case EventType::KeyPress:
    case EventType::KeyRelease:
        return true;
    default:
        return Control::hasHandlerFor(type);
    }
}

void ButtonBase::triggerClick(MouseEvent&) {
}

void ButtonBase::handleClick(MouseEvent& event) {
    if (m_checkable && enabled()) {
        if (m_group || m_exclusive) {
            if (!m_checked.get()) {
                if (m_group)
                    m_group->select(this);
                else
                    m_checked.set(true);
            }
        } else {
            m_checked.set(!m_checked.get());
        }
    }
    Widget::triggerClick(event);
}

void ButtonBase::triggerMouseLeave(MouseEvent& event) {
    assign(m_pressed, false);
    Control::triggerMouseLeave(event);
}

void ButtonBase::triggerPress(MouseEvent& event) {
    if (!enabled())
        return;

    assign(hoveredState(), true);
    if (event.button() == MouseButton::Left)
        assign(m_pressed, true);
    Widget::triggerPress(event);
}

void ButtonBase::triggerRelease(MouseEvent& event) {
    if (!enabled())
        return;

    Widget::triggerRelease(event);

    if (event.button() == MouseButton::Left)
        assign(m_pressed, false);

    if (hovered() && event.button() == MouseButton::Left)
        sendClick(event.x(), event.y(), event.modifiers());
}

void ButtonBase::triggerKeyPress(KeyEvent& event) {
    if (!enabled())
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
    if (!enabled())
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
    handleClick(click);
}

void ButtonBase::sendClickFromKeyboard(KeyModifier modifiers) {
    sendClick(displayedWidth() * 0.5f, displayedHeight() * 0.5f, modifiers);
}

}
