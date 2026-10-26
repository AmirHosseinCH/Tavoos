#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/buttongroup.h>
#include <tavoos/widget/templates/control.h>
#include <tavoos/widget/templates/tooltipbase.h>

#include <concepts>
#include <functional>
#include <type_traits>

namespace Tavoos {

class TAVOOS_EXPORT ButtonBase : public Control {
    friend class ButtonGroup;

public:
    ButtonBase(Object* parent);
    ~ButtonBase() override;

    decltype(auto) checkable(this auto&& self, PropertyArg<bool> checkable) {
        checkable.applyTo(self.m_checkable);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) exclusive(this auto&& self, PropertyArg<bool> exclusive) {
        exclusive.applyTo(self.m_exclusive);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) group(this auto&& self, ButtonGroup& group) {
        if (self.m_group)
            self.m_group->remove(&self);
        self.m_group = &group;
        group.add(&self);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) checked(this auto&& self, PropertyArg<bool> checked) {
        checked.applyTo(self.m_checked);
        return std::forward<decltype(self)>(self);
    }

    template<typename W>
        requires std::derived_from<W, ToolTipBase>
    decltype(auto) toolTip(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installToolTip<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    bool pressed() const { return m_pressed; }
    bool checkable() const { return m_checkable; }
    bool exclusive() const { return m_exclusive; }
    bool checked() const { return m_checked; }

    State<bool>& pressedState() { return m_pressed; }
    State<bool>& checkedState() { return m_checkedState; }

    ToolTipBase& toolTip();
    ToolTipBase* toolTipItem() const { return m_toolTip; }

protected:
    virtual void createToolTip();
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
    template<typename W>
    void installToolTip(std::function<void(W&)> body) {
        if (m_toolTip) {
            removeChild(m_toolTip);
            m_toolTip = nullptr;
        }
        m_toolTip = addChild<W>([this, &body](W& slot) {
            slot.delay(0.5f).opened(hoveredState());
            if (body)
                runInBuilderScope(slot, [&slot, &body] { body(slot); });
        });
    }

    static void runInBuilderScope(Widget& slot, const std::function<void()>& call);
    void sendClick(float x, float y, KeyModifier modifiers);
    void sendClickFromKeyboard(KeyModifier modifiers);

    State<bool> m_pressed{false};
    Property<bool> m_checkable{false};
    Property<bool> m_exclusive{false};
    ButtonGroup* m_group{nullptr};
    Property<bool> m_checked{false};
    State<bool> m_checkedState{false};
    bool m_spaceDown{false};
    ToolTipBase* m_toolTip{nullptr};
};

}
