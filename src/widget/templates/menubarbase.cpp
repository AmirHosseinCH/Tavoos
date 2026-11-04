#include <tavoos/widget/templates/menubarbase.h>

#include <tavoos/widget/templates/menubaritembase.h>
#include <tavoos/widget/templates/menubase.h>

#include <algorithm>

namespace Tavoos {

MenuBarBase::MenuBarBase(Object* parent) : Control{parent} {}

MenuBarItemBase* MenuBarBase::createItem() {
    return addChild<MenuBarItemBase>();
}

void MenuBarBase::onBuilt() {
    std::vector<MenuBase*> menus;
    for (const auto& child : children())
        if (auto* const menu = dynamic_cast<MenuBase*>(child.get()))
            menus.push_back(menu);

    for (MenuBase* const menu : menus) {
        MenuBarItemBase* const item = createItem();
        item->m_menu = menu;
        menu->anchor(*item).placement(Placement::Bottom).closePolicy(ClosePolicy::ClickOutsideParent | ClosePolicy::Escape);
        item->bindMenu(*menu);
    }
    Control::onBuilt();
}

bool MenuBarBase::hasHandlerFor(EventType type) {
    if (type == EventType::KeyPress || type == EventType::ShortcutOverride)
        return true;
    return Control::hasHandlerFor(type);
}

bool MenuBarBase::handlesKey(const KeyEvent& event) const {
    switch (event.keyCode()) {
    case static_cast<int>(Key::Left):
    case static_cast<int>(Key::Right):
        return openItem() || focusedItem();
    case static_cast<int>(Key::Down): {
        const MenuBarItemBase* const item = focusedItem();
        return item && item->menu() && !openItem();
    }
    default:
        return false;
    }
}

void MenuBarBase::triggerKeyPress(KeyEvent& event) {
    if (!handlesKey(event)) {
        event.ignore();
        Control::triggerKeyPress(event);
        return;
    }

    if (event.keyCode() == static_cast<int>(Key::Down)) {
        openMenu(focusedItem());
    } else {
        const int delta = event.keyCode() == static_cast<int>(Key::Right) ? 1 : -1;
        if (MenuBarItemBase* const open = openItem()) {
            if (MenuBarItemBase* const next = neighbor(open, delta); next && next != open)
                openMenu(next);
        } else if (MenuBarItemBase* const next = neighbor(focusedItem(), delta)) {
            next->focus();
        }
    }
    Control::triggerKeyPress(event);
}

void MenuBarBase::triggerShortcutOverride(KeyEvent& event) {
    if (handlesKey(event))
        event.accept();
    Control::triggerShortcutOverride(event);
}

Widget::Size MenuBarBase::computeIntrinsicSize() {
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    for (Widget* const child : stackedChildren()) {
        const Size size = child->intrinsicSize();
        contentWidth += child->marginLeft() + size.width + child->marginRight();
        contentHeight = std::max(contentHeight, child->marginTop() + size.height + child->marginBottom());
    }
    return { (width() > 0) ? static_cast<float>(width()) : contentWidth + paddingLeft() + paddingRight(),
             (height() > 0) ? static_cast<float>(height()) : contentHeight + paddingTop() + paddingBottom() };
}

Widget::ContentArea MenuBarBase::contentAreaFor(const Widget& child) const {
    float x = paddingLeft();
    for (Widget* const stacked : stackedChildren()) {
        const float width = stacked->marginLeft() + stacked->intrinsicSize().width + stacked->marginRight();
        if (stacked == &child)
            return { x, paddingTop(), width, displayedHeight() - paddingTop() - paddingBottom() };
        x += width;
    }
    return Control::contentAreaFor(child);
}

std::vector<Widget*> MenuBarBase::stackedChildren() const {
    std::vector<Widget*> result;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (widget && widget != backgroundSlot() && widget->participatesInLayout())
            result.push_back(widget);
    }
    return result;
}

std::vector<MenuBarItemBase*> MenuBarBase::items() const {
    std::vector<MenuBarItemBase*> result;
    for (const auto& child : children())
        if (auto* const item = dynamic_cast<MenuBarItemBase*>(child.get()))
            result.push_back(item);
    return result;
}

MenuBarItemBase* MenuBarBase::openItem() const {
    for (MenuBarItemBase* const item : items())
        if (item->menu() && item->menu()->openedState().get())
            return item;
    return nullptr;
}

MenuBarItemBase* MenuBarBase::focusedItem() const {
    for (MenuBarItemBase* const item : items())
        if (item->focused())
            return item;
    return nullptr;
}

MenuBarItemBase* MenuBarBase::neighbor(MenuBarItemBase* item, int delta) const {
    std::vector<MenuBarItemBase*> navigable;
    for (MenuBarItemBase* const each : items())
        if (each->enabled() && each->visible())
            navigable.push_back(each);
    if (navigable.empty())
        return nullptr;
    const int count = static_cast<int>(navigable.size());
    const auto current = std::ranges::find(navigable, item);
    if (current == navigable.end())
        return delta > 0 ? navigable.front() : navigable.back();
    return navigable[(static_cast<int>(current - navigable.begin()) + delta + count) % count];
}

void MenuBarBase::openMenu(MenuBarItemBase* item) {
    if (!item || !item->menu())
        return;
    for (MenuBarItemBase* const other : items())
        if (other != item && other->menu())
            other->menu()->close();
    item->focus();
    item->menu()->open();
}

void MenuBarBase::hoverItem(MenuBarItemBase* item) {
    MenuBarItemBase* const open = openItem();
    if (open && open != item)
        openMenu(item);
}

}
