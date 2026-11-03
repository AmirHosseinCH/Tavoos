#pragma once

#include <tavoos/events/events.h>
#include <tavoos/events/keysequence.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/menubase.h>

#include <string_view>

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

    bool highlighted() const { return m_highlighted.get(); }
    KeySequence shortcut() const { return m_shortcut.get(); }
    State<bool>& highlightedState() { return m_highlighted; }
    State<KeySequence>& shortcutState() { return m_shortcut; }
    MenuBase* menu() const { return ancestor<MenuBase>(); }

protected:
    void handleClick(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;
    void triggerFocusIn(Event& event) override;

private:
    void setShortcut(const KeySequence& sequence);

    State<bool> m_highlighted{false};
    State<KeySequence> m_shortcut{KeySequence{}};
    Shortcut* m_shortcutTrigger{nullptr};
};

}
