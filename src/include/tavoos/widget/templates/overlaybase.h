#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/animation/transitionrunner.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>

#include <cstdint>
#include <functional>
#include <type_traits>

namespace Tavoos {

enum class ClosePolicy : std::uint32_t {
    None = 0,
    ClickOutside = 1 << 0,
    Escape = 1 << 1,
};

template<>
struct EnableBitmaskOperators<ClosePolicy> : std::true_type {};

class TAVOOS_EXPORT OverlayBase : public Control {
public:
    OverlayBase(Object* parent);

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

    template<typename W = RectangleWidget>
    decltype(auto) scrim(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installScrim<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) enter(this auto&& self, TransitionFactory factory) {
        self.m_enter = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) exit(this auto&& self, TransitionFactory factory) {
        self.m_exit = std::move(factory);
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) enter(this auto&& self, Args&&... args) {
        self.m_enter = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
        return std::forward<decltype(self)>(self);
    }

    template<typename T, typename... Args>
    decltype(auto) exit(this auto&& self, Args&&... args) {
        self.m_exit = [... captured = std::forward<Args>(args)] { return std::make_unique<T>(captured...); };
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
    bool modalActive() const noexcept { return m_modalActive; }

    State<bool>& openedState() { return m_openedState; }

    void open();
    void close();

    virtual void place() {}
    bool isContentHit(const Widget* hit) const;

    bool isOverlay() const override { return true; }

protected:
    bool hasHandlerFor(EventType type) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;

private:
    using Control::content;

    template<typename W>
    void installScrim(std::function<void(W&)> body) {
        if (m_scrim) {
            removeChild(m_scrim);
            m_scrim = nullptr;
        }
        m_scrim = addChild<W>([&body](W& slot) {
            slot.z(-2).fill(Fill::Both);
            if (body)
                body(slot);
        });
        m_scrim->visible(m_modalActive);
    }

    void applyOpened(bool opened);
    void runTransition(TransitionFactory factory, std::function<void()> onFinished);
    void finishHiding();

    Property<bool> m_opened{false};
    State<bool> m_openedState{false};
    Property<ClosePolicy> m_closePolicy{ClosePolicy::ClickOutside | ClosePolicy::Escape};
    Property<bool> m_modal{false};
    Widget* m_scrim{nullptr};
    bool m_modalActive{false};

    TransitionFactory m_enter;
    TransitionFactory m_exit;
    TransitionRunner m_runner;

    std::function<void()> m_onOpen;
    std::function<void()> m_onClose;
};

}
