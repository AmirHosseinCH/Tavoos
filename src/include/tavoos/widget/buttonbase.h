#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/control.h>

namespace Tavoos {

class TAVOOS_EXPORT ButtonBase : public Control {
public:
    ButtonBase(Object* parent);

    bool pressed() const { return m_pressed; }

    State<bool>& pressedState() { return m_pressed; }

protected:
    void onSlotReplaced() override;

    bool hasHandlerFor(EventType type) override;
    void triggerClick(MouseEvent& event) override;
    virtual void handleClick(MouseEvent& event);
    void triggerPress(MouseEvent& event) override;
    void triggerRelease(MouseEvent& event) override;
    void triggerMouseLeave(MouseEvent& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerKeyRelease(KeyEvent& event) override;
    void triggerFocusOut(Event& event) override;

private:
    void sendClick(float x, float y, KeyModifier modifiers);
    void sendClickFromKeyboard(KeyModifier modifiers);

    State<bool> m_pressed{false};
    bool m_spaceDown{false};
};

}
