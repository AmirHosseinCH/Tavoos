#include <tavoos/widget/templates/menuitembase.h>

#include <tavoos/builder.h>
#include <tavoos/events/shortcut.h>

namespace Tavoos {

namespace {

class MenuItemShortcut : public Shortcut {
public:
    using Shortcut::Shortcut;

protected:
    bool reachable() const override {
        for (const Object* object = parent(); object; object = object->parent()) {
            const auto* const widget = dynamic_cast<const Widget*>(object);
            if (widget && !widget->visible() && !dynamic_cast<const MenuBase*>(widget))
                return false;
        }
        return true;
    }
};

}

MenuItemBase::MenuItemBase(Object* parent) : ButtonBase{parent} {
    fill(Fill::Width);
}

void MenuItemBase::runInBuilderScope(Widget& slot, const std::function<void()>& call) {
    Builder::Scope scope{&slot};
    call();
}

void MenuItemBase::setShortcut(const KeySequence& sequence) {
    m_shortcut.setIfChanged(sequence);
    if (!m_shortcutTrigger) {
        m_shortcutTrigger = addChild<MenuItemShortcut>([this](MenuItemShortcut& trigger) {
            trigger.enabled(enabledState()).onActivated([this] { sendClickFromKeyboard(KeyModifier::None); });
        });
    }
    m_shortcutTrigger->sequence(sequence);
}

void MenuItemBase::openSubMenu() {
    if (!m_subMenu)
        return;
    if (MenuBase* const owner = menu(); owner && !owner->openedState().get())
        return;
    if (m_subMenu->openedState().get())
        m_subMenu->focusHighlighted();
    else
        m_subMenu->open();
}

MenuBase* MenuItemBase::rootMenu() const {
    MenuBase* root = menu();
    while (root) {
        MenuBase* const above = root->ancestor<MenuBase>();
        if (!above)
            break;
        root = above;
    }
    return root;
}

void MenuItemBase::handleClick(MouseEvent& event) {
    if (m_subMenu) {
        openSubMenu();
        return;
    }
    if (MenuBase* const root = rootMenu())
        root->close();
    ButtonBase::handleClick(event);
}

void MenuItemBase::triggerMouseEnter(MouseEvent& event) {
    ButtonBase::triggerMouseEnter(event);
    if (!enabled())
        return;
    if (MenuBase* const owner = menu())
        owner->highlight(this, true);
}

void MenuItemBase::triggerMouseLeave(MouseEvent& event) {
    ButtonBase::triggerMouseLeave(event);
    if (MenuBase* const owner = menu(); owner && owner->m_pendingItem == this)
        owner->stopSubMenuTimer();
}

void MenuItemBase::triggerFocusIn(Event& event) {
    ButtonBase::triggerFocusIn(event);
    if (MenuBase* const owner = menu())
        owner->highlight(this);
}

}
