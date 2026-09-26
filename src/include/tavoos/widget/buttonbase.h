#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/rectangle.h>
#include <tavoos/widget/text.h>
#include <tavoos/widget/widget.h>

#include <functional>
#include <type_traits>

namespace Tavoos {

class TAVOOS_EXPORT ButtonBase : public Widget {
public:
    ButtonBase(Object* parent);

    decltype(auto) enabled(this auto&& self, PropertyArg<bool> enabled) {
        enabled.applyTo(self.m_enabled);
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) background(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installBackground<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = TextWidget>
    decltype(auto) content(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installContent<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    unsigned contentRevision() const noexcept { return m_contentRevision; }
    bool enabled() const { return m_enabled; }
    bool hovered() const { return m_hovered; }
    bool pressed() const { return m_pressed; }

    State<bool>& enabledState() { return m_enabledState; }
    State<bool>& hoveredState() { return m_hovered; }
    State<bool>& pressedState() { return m_pressed; }

protected:
    void render(Renderer& renderer) override;
    Size computeIntrinsicSize() override;

    bool hasHandlerFor(EventType type) override;
    void triggerClick(MouseEvent& event) override;
    virtual void handleClick(MouseEvent& event);
    void triggerPress(MouseEvent& event) override;
    void triggerRelease(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;
    void triggerMouseLeave(MouseEvent& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerKeyRelease(KeyEvent& event) override;
    void triggerFocusOut(Event& event) override;

private:
    template<typename W>
    void installBackground(std::function<void(W&)> body) {
        if (m_background)
            replaceSlot(m_background);
        m_background = addChild<W>([&body](W& slot) {
            slot.z(-1).fill(Fill::Both);
            if (body)
                body(slot);
        });
    }

    template<typename W>
    void installContent(std::function<void(W&)> body) {
        if (m_content)
            replaceSlot(m_content);
        m_content = addChild<W>([&body](W& slot) {
            slot.alignment(Alignment::Center);
            if (body)
                body(slot);
        });
        ++m_contentRevision;
        requestRelayout();
    }

    void replaceSlot(Widget*& slot);
    void sendClick(float x, float y, KeyModifier modifiers);
    void sendClickFromKeyboard(KeyModifier modifiers);

    Property<bool> m_enabled{true};
    State<bool> m_enabledState{true};
    State<bool> m_hovered{false};
    State<bool> m_pressed{false};
    bool m_spaceDown{false};

    Widget* m_background{nullptr};
    Widget* m_content{nullptr};
    unsigned m_contentRevision{0};
};

}
