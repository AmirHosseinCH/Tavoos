#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitionrunner.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace Tavoos {

struct TransitionSet {
    TransitionFactory enter;
    TransitionFactory exit;

    static TransitionSet immediate() { return {}; }
};

class TAVOOS_EXPORT StackViewBase : public Control {
public:
    StackViewBase(Object* parent);

    template<typename T>
    T* push(std::function<void(T&)> body = {}, std::optional<TransitionSet> transitions = std::nullopt) {
        static_assert(std::is_base_of_v<Widget, T>, "push<T>() requires T to derive from Widget");
        finishTransition();
        Widget* const previous = currentItem();
        T* const created = addChild<T>([&body](T& item) {
            item.fill(Fill::Both);
            if (body)
                body(item);
        });
        itemPushed(previous, created);
        runTransition(created, previous, chooseEnter(transitions, m_pushEnter), chooseExit(transitions, m_pushExit),
                      [this, previous] { hideItem(previous); });
        return created;
    }

    template<typename T>
    T* replace(std::function<void(T&)> body = {}, std::optional<TransitionSet> transitions = std::nullopt) {
        static_assert(std::is_base_of_v<Widget, T>, "replace<T>() requires T to derive from Widget");
        finishTransition();
        Widget* const replaced = currentItem();
        if (replaced)
            m_retiring.push_back(replaced);
        T* const created = addChild<T>([&body](T& item) {
            item.fill(Fill::Both);
            if (body)
                body(item);
        });
        itemPushed(replaced, created);
        if (replaced) {
            handOverFocus(replaced, created);
            runTransition(created, replaced, chooseEnter(transitions, m_replaceEnter), chooseExit(transitions, m_replaceExit),
                          [this, replaced] { removeRetiring(replaced); });
        }
        return created;
    }

    template<typename T>
    T* initialItem(std::function<void(T&)> body = {}) {
        return push<T>(std::move(body));
    }

    decltype(auto) onItemChange(this auto&& self, std::function<void(Widget*)> callback) {
        self.m_onItemChange = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) pushEnter(this auto&& self, TransitionFactory factory) {
        self.m_pushEnter = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) pushExit(this auto&& self, TransitionFactory factory) {
        self.m_pushExit = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) popEnter(this auto&& self, TransitionFactory factory) {
        self.m_popEnter = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) popExit(this auto&& self, TransitionFactory factory) {
        self.m_popExit = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) replaceEnter(this auto&& self, TransitionFactory factory) {
        self.m_replaceEnter = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) replaceExit(this auto&& self, TransitionFactory factory) {
        self.m_replaceExit = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) pushEnter(this auto&& self, Args&&... args) {
        self.m_pushEnter = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) pushExit(this auto&& self, Args&&... args) {
        self.m_pushExit = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) popEnter(this auto&& self, Args&&... args) {
        self.m_popEnter = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) popExit(this auto&& self, Args&&... args) {
        self.m_popExit = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) replaceEnter(this auto&& self, Args&&... args) {
        self.m_replaceEnter = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) replaceExit(this auto&& self, Args&&... args) {
        self.m_replaceExit = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) clearTransitions(this auto&& self) {
        self.m_pushEnter = self.m_pushExit = self.m_popEnter = self.m_popExit = nullptr;
        self.m_replaceEnter = self.m_replaceExit = nullptr;
        return std::forward<decltype(self)>(self);
    }

    bool pop(std::optional<TransitionSet> transitions = std::nullopt);
    bool popTo(Widget* item, std::optional<TransitionSet> transitions = std::nullopt);
    void popToRoot(std::optional<TransitionSet> transitions = std::nullopt);
    void clear();

    int depth() const { return m_depth.get(); }
    State<int>& depthState() { return m_depth; }
    State<bool>& transitionRunningState() { return m_running; }
    Widget* currentItem() const;
    Widget* item(int index) const;
    int indexOf(const Widget* item) const;

    static StackViewBase* of(Widget& widget) { return widget.ancestor<StackViewBase>(); }

protected:
    void render(Renderer& renderer) override;

private:
    static TransitionFactory chooseEnter(const std::optional<TransitionSet>& over, const TransitionFactory& fallback) {
        return over ? over->enter : fallback;
    }

    static TransitionFactory chooseExit(const std::optional<TransitionSet>& over, const TransitionFactory& fallback) {
        return over ? over->exit : fallback;
    }

    std::vector<Widget*> items() const;
    bool isChildItem(const Widget* item) const;
    void itemPushed(Widget* previous, Widget* created);
    void handOverFocus(Widget* replaced, Widget* created);
    void hideItem(Widget* item);
    void removeRetiring(Widget* item);
    void finishTransition();
    void runTransition(Widget* entering, Widget* exiting, TransitionFactory enter, TransitionFactory exit,
                       std::function<void()> onFinished);
    void syncDepth();
    void notify();

    State<int> m_depth{0};
    State<bool> m_running{false};
    std::function<void(Widget*)> m_onItemChange;

    TransitionFactory m_pushEnter;
    TransitionFactory m_pushExit;
    TransitionFactory m_popEnter;
    TransitionFactory m_popExit;
    TransitionFactory m_replaceEnter;
    TransitionFactory m_replaceExit;

    std::vector<Widget*> m_retiring;
    TransitionRunner m_runner;
};

}
