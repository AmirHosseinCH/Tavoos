#include <tavoos/widget/controls/stackviewwidget.h>

#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

StackViewWidget::StackViewWidget(Object* parent) : Control{parent} {
}

std::vector<Widget*> StackViewWidget::items() const {
    std::vector<Widget*> result;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (widget && widget != backgroundSlot() && !widget->isOverlay())
            result.push_back(widget);
    }
    return result;
}

Widget* StackViewWidget::currentItem() const {
    const std::vector<Widget*> all = items();
    return all.empty() ? nullptr : all.back();
}

Widget* StackViewWidget::item(int index) const {
    const std::vector<Widget*> all = items();
    if (index < 0 || index >= static_cast<int>(all.size()))
        return nullptr;
    return all[static_cast<std::size_t>(index)];
}

void StackViewWidget::syncDepth() {
    m_depth.setIfChanged(static_cast<int>(items().size()));
}

void StackViewWidget::notify() {
    if (m_onItemChange)
        m_onItemChange(currentItem());
}

void StackViewWidget::itemPushed(Widget* previous, Widget* created) {
    if (previous)
        previous->visible(false);

    if (Window* const window = ownerWindow()) {
        window->rememberFocus(created);
        window->focusFirstIn(created);
        if (previous) {
            for (Widget* w = window->focusedWidget(); w; w = dynamic_cast<Widget*>(w->parent())) {
                if (w == previous) {
                    window->setFocusedWidget(nullptr);
                    break;
                }
            }
        }
    }

    syncDepth();
    notify();
}

void StackViewWidget::handOverFocus(Widget* replaced, Widget* created) {
    if (Window* const window = ownerWindow())
        window->transferFocusMemory(replaced, created);
}

void StackViewWidget::retire(Widget* item) {
    removeChild(item);
    syncDepth();
}

bool StackViewWidget::pop() {
    const std::vector<Widget*> all = items();
    if (all.size() <= 1)
        return false;

    Widget* const top = all.back();
    Widget* const below = all[all.size() - 2];
    below->visible(true);
    if (Window* const window = ownerWindow())
        window->restoreFocus(top);
    removeChild(top);
    syncDepth();
    notify();
    return true;
}

bool StackViewWidget::popTo(Widget* target) {
    const std::vector<Widget*> all = items();
    if (std::ranges::find(all, target) == all.end())
        return false;
    if (all.back() == target)
        return true;

    Window* const window = ownerWindow();
    target->visible(true);
    for (std::size_t i = all.size(); i-- > 0 && all[i] != target;) {
        if (window)
            window->restoreFocus(all[i]);
        removeChild(all[i]);
    }
    syncDepth();
    notify();
    return true;
}

void StackViewWidget::popToRoot() {
    if (Widget* const root = item(0))
        popTo(root);
}

void StackViewWidget::clear() {
    const std::vector<Widget*> all = items();
    for (Widget* const item : all)
        removeChild(item);
    syncDepth();
    notify();
}

}
