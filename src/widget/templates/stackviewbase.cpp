#include <tavoos/widget/templates/stackviewbase.h>

#include <tavoos/gfx/renderer.h>
#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

StackViewBase::StackViewBase(Object* parent) : Control{parent} {
    background([](RectangleWidget& box) { box.color(Color::Transparent); });
}

std::vector<Widget*> StackViewBase::items() const {
    std::vector<Widget*> result;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || widget == backgroundSlot() || widget->isOverlay())
            continue;
        if (std::ranges::find(m_retiring, widget) != m_retiring.end())
            continue;
        result.push_back(widget);
    }
    return result;
}

bool StackViewBase::isChildItem(const Widget* target) const {
    for (const auto& child : children()) {
        if (child.get() == target)
            return true;
    }
    return false;
}

Widget* StackViewBase::currentItem() const {
    const std::vector<Widget*> all = items();
    return all.empty() ? nullptr : all.back();
}

Widget* StackViewBase::item(int index) const {
    const std::vector<Widget*> all = items();
    if (index < 0 || index >= static_cast<int>(all.size()))
        return nullptr;
    return all[static_cast<std::size_t>(index)];
}

int StackViewBase::indexOf(const Widget* target) const {
    const std::vector<Widget*> all = items();
    const auto found = std::ranges::find(all, target);
    return found == all.end() ? -1 : static_cast<int>(found - all.begin());
}

void StackViewBase::syncDepth() {
    m_depth.setIfChanged(static_cast<int>(items().size()));
}

void StackViewBase::notify() {
    if (m_onItemChange)
        m_onItemChange(currentItem());
}

void StackViewBase::render(Renderer& renderer) {
    if (renderer.isCapturingMask()) {
        if (Widget* const shape = backgroundSlot())
            renderer.renderWidget(*shape);
        return;
    }
    renderChildren(renderer);
}

void StackViewBase::itemPushed(Widget* previous, Widget* created) {
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

void StackViewBase::handOverFocus(Widget* replaced, Widget* created) {
    if (Window* const window = ownerWindow())
        window->transferFocusMemory(replaced, created);
}

void StackViewBase::hideItem(Widget* target) {
    if (target && isChildItem(target))
        target->visible(false);
}

void StackViewBase::removeRetiring(Widget* target) {
    std::erase(m_retiring, target);
    if (target && isChildItem(target))
        removeChild(target);
    syncDepth();
}

void StackViewBase::finishTransition() {
    m_runner.finishNow();
}

void StackViewBase::runTransition(Widget* entering, Widget* exiting, TransitionFactory enter, TransitionFactory exit,
                                  std::function<void()> onFinished) {
    std::vector<TransitionRunner::Entry> entries;
    if (enter && entering)
        entries.push_back({entering, enter()});
    if (exit && exiting)
        entries.push_back({exiting, exit()});

    if (entries.empty()) {
        if (onFinished)
            onFinished();
        return;
    }

    clip(true);
    m_running.setIfChanged(true);
    m_runner.start(std::move(entries), displayedWidth(), displayedHeight(),
                   [this](Widget* w) { return isChildItem(w); },
                   [this, onFinished = std::move(onFinished)] {
                       clip(false);
                       m_running.setIfChanged(false);
                       if (onFinished)
                           onFinished();
                   });
}

bool StackViewBase::pop(std::optional<TransitionSet> transitions) {
    finishTransition();
    const std::vector<Widget*> all = items();
    if (all.size() <= 1)
        return false;

    Widget* const top = all.back();
    Widget* const below = all[all.size() - 2];
    below->visible(true);
    if (Window* const window = ownerWindow())
        window->restoreFocus(top);
    m_retiring.push_back(top);
    syncDepth();
    notify();
    runTransition(below, top, chooseEnter(transitions, m_popEnter), chooseExit(transitions, m_popExit),
                  [this, top] { removeRetiring(top); });
    return true;
}

bool StackViewBase::popTo(Widget* target, std::optional<TransitionSet> transitions) {
    finishTransition();
    const std::vector<Widget*> all = items();
    const auto found = std::ranges::find(all, target);
    if (found == all.end())
        return false;
    if (all.back() == target)
        return true;

    Widget* const top = all.back();
    Window* const window = ownerWindow();
    target->visible(true);
    if (window) {
        for (std::size_t i = all.size(); i-- > 0 && all[i] != target;)
            window->restoreFocus(all[i]);
    }
    for (std::size_t i = static_cast<std::size_t>(found - all.begin()) + 1; i + 1 < all.size(); ++i)
        removeChild(all[i]);

    m_retiring.push_back(top);
    syncDepth();
    notify();
    runTransition(target, top, chooseEnter(transitions, m_popEnter), chooseExit(transitions, m_popExit),
                  [this, top] { removeRetiring(top); });
    return true;
}

void StackViewBase::popToRoot(std::optional<TransitionSet> transitions) {
    if (Widget* const root = item(0))
        popTo(root, std::move(transitions));
}

void StackViewBase::clear() {
    finishTransition();
    const std::vector<Widget*> all = items();
    for (Widget* const target : all)
        removeChild(target);
    syncDepth();
    notify();
}

}
