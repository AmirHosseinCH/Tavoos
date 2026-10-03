#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/buttongroup.h>
#include <tavoos/widget/templates/control.h>

namespace Tavoos {

class TAVOOS_EXPORT ButtonBase : public Control {
public:
    ButtonBase(Object* parent);

    decltype(auto) checkable(this auto&& self, PropertyArg<bool> checkable) {
        checkable.applyTo(self.m_checkable);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) exclusive(this auto&& self, PropertyArg<bool> exclusive) {
        exclusive.applyTo(self.m_exclusive);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) group(this auto&& self, ButtonGroup& group) {
        self.m_group = &group;
        group.add(&self);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checked(this auto&& self, PropertyArg<bool> checked) {
        checked.applyTo(self.m_checked);
        return std::forward<decltype(self)>(self);
    }

    bool pressed() const { return m_pressed; }
    bool checkable() const { return m_checkable; }
    bool exclusive() const { return m_exclusive; }
    bool checked() const { return m_checked; }

    State<bool>& pressedState() { return m_pressed; }
    State<bool>& checkedState() { return m_checkedState; }

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
    Property<bool> m_checkable{false};
    Property<bool> m_exclusive{false};
    ButtonGroup* m_group{nullptr};
    Property<bool> m_checked{false};
    State<bool> m_checkedState{false};
    bool m_spaceDown{false};
};

}
