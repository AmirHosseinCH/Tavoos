#pragma once

#include <tavoos/events/dropevent.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

#include <functional>
#include <string>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT DropAreaWidget : public Widget {
    friend class Window;

public:
    DropAreaWidget(Object* parent);

    decltype(auto) keys(this auto&& self, std::vector<std::string> keys) {
        self.m_keys = std::move(keys);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onEntered(this auto&& self, std::function<void(DropEvent&)> callback) {
        self.m_onEntered = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onPositionChanged(this auto&& self, std::function<void(DropEvent&)> callback) {
        self.m_onPositionChanged = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onExited(this auto&& self, std::function<void()> callback) {
        self.m_onExited = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onDropped(this auto&& self, std::function<void(DropEvent&)> callback) {
        self.m_onDropped = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    const std::vector<std::string>& keys() const { return m_keys; }
    bool containsDrag() const { return m_containsDrag.get(); }
    State<bool>& containsDragState() { return m_containsDrag; }
    Widget* dragSource() const { return m_dragSource; }

    bool acceptsKeys(const std::vector<std::string>& keys) const;

protected:
    void render(Renderer& renderer) override { renderChildren(renderer); }

private:
    std::vector<std::string> m_keys;
    std::function<void(DropEvent&)> m_onEntered;
    std::function<void(DropEvent&)> m_onPositionChanged;
    std::function<void()> m_onExited;
    std::function<void(DropEvent&)> m_onDropped;
    State<bool> m_containsDrag{false};
    Widget* m_dragSource{nullptr};
};

}
