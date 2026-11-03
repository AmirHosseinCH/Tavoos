#include <tavoos/widget/templates/menuitembase.h>

namespace Tavoos {

MenuItemBase::MenuItemBase(Object* parent) : ButtonBase{parent} {
    fill(Fill::Width);
}

void MenuItemBase::handleClick(MouseEvent& event) {
    if (MenuBase* const owner = menu())
        owner->close();
    ButtonBase::handleClick(event);
}

void MenuItemBase::triggerMouseEnter(MouseEvent& event) {
    ButtonBase::triggerMouseEnter(event);
    if (!enabled())
        return;
    if (MenuBase* const owner = menu())
        owner->highlight(this);
}

void MenuItemBase::triggerFocusIn(Event& event) {
    ButtonBase::triggerFocusIn(event);
    if (MenuBase* const owner = menu())
        owner->highlight(this);
}

}
