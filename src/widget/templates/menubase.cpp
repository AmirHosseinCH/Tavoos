#include <tavoos/widget/templates/menubase.h>

#include <tavoos/widget/templates/menuitembase.h>

#include <algorithm>

namespace Tavoos {

namespace {

void collectItems(const Object& root, bool navigableOnly, std::vector<MenuItemBase*>& out) {
    for (const auto& child : root.children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || widget->isOverlay() || (navigableOnly && !widget->visible()))
            continue;
        if (auto* const item = dynamic_cast<MenuItemBase*>(widget)) {
            if (!navigableOnly || item->enabled())
                out.push_back(item);
            continue;
        }
        collectItems(*widget, navigableOnly, out);
    }
}

}

MenuBase::MenuBase(Object* parent) : PopupBase{parent} {}

MenuBase::~MenuBase() {
    if (m_ticking)
        AnimationManager::instance().unregisterAnimation(this);
}

void MenuBase::applyOpened(bool opened) {
    if (opened && !openedState().get())
        highlight(nullptr);
    if (!opened) {
        stopSubMenuTimer();
        closeSubMenus(nullptr);
    }
    PopupBase::applyOpened(opened);
}

bool MenuBase::hasHandlerFor(EventType type) {
    if (type == EventType::KeyPress || type == EventType::ShortcutOverride)
        return true;
    return PopupBase::hasHandlerFor(type);
}

bool MenuBase::handlesKey(const KeyEvent& event) const {
    switch (event.keyCode()) {
    case static_cast<int>(Key::Up):
    case static_cast<int>(Key::Down):
    case static_cast<int>(Key::Home):
    case static_cast<int>(Key::End):
        return true;
    case static_cast<int>(Key::Tab):
        return event.modifiers() == KeyModifier::None || event.modifiers() == KeyModifier::Shift;
    case static_cast<int>(Key::Right): {
        const MenuItemBase* const item = highlightedItem();
        return item && item->subMenuItem();
    }
    case static_cast<int>(Key::Left):
        return ancestor<MenuBase>() != nullptr;
    default:
        return false;
    }
}

void MenuBase::triggerKeyPress(KeyEvent& event) {
    if (!handlesKey(event)) {
        event.ignore();
        PopupBase::triggerKeyPress(event);
        return;
    }

    switch (event.keyCode()) {
    case static_cast<int>(Key::Up):
        step(-1);
        break;
    case static_cast<int>(Key::Down):
        step(1);
        break;
    case static_cast<int>(Key::Tab):
        step(event.modifiers() == KeyModifier::Shift ? -1 : 1);
        break;
    case static_cast<int>(Key::Home):
    case static_cast<int>(Key::End): {
        const auto navigable = items(true);
        if (!navigable.empty())
            highlight(event.keyCode() == static_cast<int>(Key::Home) ? navigable.front() : navigable.back());
        break;
    }
    case static_cast<int>(Key::Right):
        highlightedItem()->openSubMenu();
        break;
    case static_cast<int>(Key::Left):
        close();
        break;
    default:
        break;
    }
    PopupBase::triggerKeyPress(event);
}

void MenuBase::triggerShortcutOverride(KeyEvent& event) {
    if (handlesKey(event))
        event.accept();
    PopupBase::triggerShortcutOverride(event);
}

Widget::Size MenuBase::computeIntrinsicSize() {
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    for (Widget* const child : stackedChildren()) {
        const Size size = child->intrinsicSize();
        contentWidth = std::max(contentWidth, child->marginLeft() + size.width + child->marginRight());
        contentHeight += child->marginTop() + size.height + child->marginBottom();
    }
    return { (width() > 0) ? static_cast<float>(width()) : contentWidth + paddingLeft() + paddingRight(),
             (height() > 0) ? static_cast<float>(height()) : contentHeight + paddingTop() + paddingBottom() };
}

Widget::ContentArea MenuBase::contentAreaFor(const Widget& child) const {
    float y = paddingTop();
    for (Widget* const stacked : stackedChildren()) {
        const float height = stacked->marginTop() + stacked->intrinsicSize().height + stacked->marginBottom();
        if (stacked == &child)
            return { paddingLeft(), y, displayedWidth() - paddingLeft() - paddingRight(), height };
        y += height;
    }
    return PopupBase::contentAreaFor(child);
}

bool MenuBase::tick(float dt) {
    m_pendingElapsed += dt;
    if (m_pendingElapsed >= m_subMenuDelay.get()) {
        MenuItemBase* const item = m_pendingItem;
        m_pendingItem = nullptr;
        m_ticking = false;
        if (std::ranges::contains(items(false), item) && item->highlighted())
            item->openSubMenu();
    }
    return m_ticking;
}

std::vector<Widget*> MenuBase::stackedChildren() const {
    std::vector<Widget*> result;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (widget && widget != backgroundSlot() && widget != scrimSlot() && widget->participatesInLayout())
            result.push_back(widget);
    }
    return result;
}

std::vector<MenuItemBase*> MenuBase::items(bool navigableOnly) const {
    std::vector<MenuItemBase*> result;
    collectItems(*this, navigableOnly, result);
    return result;
}

MenuItemBase* MenuBase::highlightedItem() const {
    for (MenuItemBase* const item : items(false))
        if (item->highlighted())
            return item;
    return nullptr;
}

void MenuBase::highlight(MenuItemBase* item, bool byHover) {
    const bool changed = item != highlightedItem();
    if (changed) {
        stopSubMenuTimer();
        for (MenuItemBase* const each : items(false))
            each->m_highlighted.setIfChanged(each == item);
    }
    if (item && !item->focused())
        item->focus();
    if (changed)
        closeSubMenus(item);
    if (byHover && item && item->subMenuItem() && !item->subMenuItem()->openedState().get())
        startSubMenuTimer(item);
}

void MenuBase::focusHighlighted() {
    MenuItemBase* item = highlightedItem();
    if (!item) {
        const auto navigable = items(true);
        if (navigable.empty())
            return;
        item = navigable.front();
    }
    highlight(item);
}

void MenuBase::step(int delta) {
    const auto navigable = items(true);
    if (navigable.empty())
        return;
    const int count = static_cast<int>(navigable.size());
    const auto current = std::ranges::find(navigable, highlightedItem());
    const int next = current == navigable.end()
                         ? (delta > 0 ? 0 : count - 1)
                         : (static_cast<int>(current - navigable.begin()) + delta + count) % count;
    highlight(navigable[next]);
}

void MenuBase::closeSubMenus(const MenuItemBase* except) {
    for (MenuItemBase* const each : items(false))
        if (each != except && each->subMenuItem())
            each->subMenuItem()->close();
}

void MenuBase::startSubMenuTimer(MenuItemBase* item) {
    if (m_subMenuDelay.get() <= 0.0f) {
        stopSubMenuTimer();
        item->openSubMenu();
        return;
    }
    m_pendingItem = item;
    m_pendingElapsed = 0.0f;
    if (m_ticking)
        return;
    m_ticking = true;
    AnimationManager::instance().registerAnimation(this);
}

void MenuBase::stopSubMenuTimer() {
    m_pendingItem = nullptr;
    if (!m_ticking)
        return;
    m_ticking = false;
    AnimationManager::instance().unregisterAnimation(this);
}

}
