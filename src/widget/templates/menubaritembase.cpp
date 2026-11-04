#include <tavoos/widget/templates/menubaritembase.h>

namespace Tavoos {

MenuBarItemBase::MenuBarItemBase(Object* parent) : ButtonBase{parent} {
    fill(Fill::Height);
}

void MenuBarItemBase::handleClick(MouseEvent&) {
    if (m_menu && m_menu->openedState().get())
        m_menu->close();
    else if (MenuBarBase* const owner = bar())
        owner->openMenu(this);
}

void MenuBarItemBase::triggerMouseEnter(MouseEvent& event) {
    ButtonBase::triggerMouseEnter(event);
    if (!enabled())
        return;
    if (MenuBarBase* const owner = bar())
        owner->hoverItem(this);
}

}
