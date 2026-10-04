#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <cstdint>
#include <functional>

namespace Tavoos {

enum class ClosePolicy : std::uint32_t {
    None = 0,
    ClickOutside = 1 << 0,
    Escape = 1 << 1,
};

template<>
struct EnableBitmaskOperators<ClosePolicy> : std::true_type {};

class TAVOOS_EXPORT PopupBase : public Control {
public:
    PopupBase(Object* parent);

    decltype(auto) opened(this auto&& self, PropertyArg<bool> opened) {
        opened.applyTo(self.m_opened);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) closePolicy(this auto&& self, PropertyArg<ClosePolicy> policy) {
        policy.applyTo(self.m_closePolicy);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) modal(this auto&& self, PropertyArg<bool> modal) {
        modal.applyTo(self.m_modal);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onOpen(this auto&& self, std::function<void()> callback) {
        self.m_onOpen = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onClose(this auto&& self, std::function<void()> callback) {
        self.m_onClose = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    bool opened() const { return m_opened; }
    ClosePolicy closePolicy() const { return m_closePolicy; }
    bool modal() const { return m_modal; }

    State<bool>& openedState() { return m_openedState; }

    void open();
    void close();

    bool isOverlay() const override { return true; }

protected:
    bool hasHandlerFor(EventType type) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;

private:
    using Control::content;

    void applyOpened(bool opened);

    Property<bool> m_opened{false};
    State<bool> m_openedState{false};
    Property<ClosePolicy> m_closePolicy{ClosePolicy::ClickOutside | ClosePolicy::Escape};
    Property<bool> m_modal{false};

    std::function<void()> m_onOpen;
    std::function<void()> m_onClose;
};

}
