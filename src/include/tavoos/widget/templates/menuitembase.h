#pragma once

#include <tavoos/events/events.h>
#include <tavoos/events/keysequence.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/menubase.h>

#include <concepts>
#include <functional>
#include <string_view>
#include <type_traits>

namespace Tavoos {

class Shortcut;

class TAVOOS_EXPORT MenuItemBase : public ButtonBase {
    friend class MenuBase;

public:
    MenuItemBase(Object* parent);

    decltype(auto) shortcut(this auto&& self, const KeySequence& sequence) {
        self.setShortcut(sequence);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) shortcut(this auto&& self, std::string_view text) {
        self.setShortcut(KeySequence::fromString(text));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = MenuBase>
        requires std::derived_from<W, MenuBase>
    decltype(auto) subMenu(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installSubMenu<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    bool highlighted() const { return m_highlighted.get(); }
    KeySequence shortcut() const { return m_shortcut.get(); }
    State<bool>& highlightedState() { return m_highlighted; }
    State<KeySequence>& shortcutState() { return m_shortcut; }
    MenuBase* menu() const { return ancestor<MenuBase>(); }
    MenuBase* subMenuItem() const { return m_subMenu; }

protected:
    virtual void styleSubMenu(MenuBase&) {}
    void handleClick(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;
    void triggerMouseLeave(MouseEvent& event) override;
    void triggerFocusIn(Event& event) override;

private:
    template<typename W>
    void installSubMenu(std::function<void(W&)> body) {
        if (m_subMenu) {
            removeChild(m_subMenu);
            m_subMenu = nullptr;
        }
        m_subMenu = addChild<W>([this, &body](W& slot) {
            slot.placement(Placement::Right).closePolicy(ClosePolicy::ClickOutsideParent | ClosePolicy::Escape);
            styleSubMenu(slot);
            if (body)
                runInBuilderScope(slot, [&slot, &body] { body(slot); });
        });
    }

    static void runInBuilderScope(Widget& slot, const std::function<void()>& call);
    void setShortcut(const KeySequence& sequence);
    void openSubMenu();
    MenuBase* rootMenu() const;

    State<bool> m_highlighted{false};
    State<KeySequence> m_shortcut{KeySequence{}};
    Shortcut* m_shortcutTrigger{nullptr};
    MenuBase* m_subMenu{nullptr};
};

}
