#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <functional>
#include <type_traits>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT StackViewWidget : public Control {
public:
    StackViewWidget(Object* parent);

    template<typename T>
    T* push(std::function<void(T&)> body = {}) {
        static_assert(std::is_base_of_v<Widget, T>, "push<T>() requires T to derive from Widget");
        Widget* const previous = currentItem();
        T* const created = addChild<T>([&body](T& item) {
            item.fill(Fill::Both);
            if (body)
                body(item);
        });
        itemPushed(previous, created);
        return created;
    }

    template<typename T>
    T* replace(std::function<void(T&)> body = {}) {
        Widget* const replaced = currentItem();
        T* const created = push<T>(std::move(body));
        if (replaced)
            retire(replaced);
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

    bool pop();
    bool popTo(Widget* item);
    void popToRoot();
    void clear();

    int depth() const { return m_depth.get(); }
    State<int>& depthState() { return m_depth; }
    Widget* currentItem() const;
    Widget* item(int index) const;

    static StackViewWidget* of(Widget& widget) { return widget.ancestor<StackViewWidget>(); }

private:
    std::vector<Widget*> items() const;
    void itemPushed(Widget* previous, Widget* created);
    void retire(Widget* item);
    void syncDepth();
    void notify();

    State<int> m_depth{0};
    std::function<void(Widget*)> m_onItemChange;
};

}
