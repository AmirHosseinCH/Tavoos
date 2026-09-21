#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

namespace Tavoos {

class TAVOOS_EXPORT ButtonBase : public Widget {
public:
    ButtonBase(Object* parent);

    decltype(auto) enabled(this auto&& self, PropertyArg<bool> enabled) {
        enabled.applyTo(self.m_enabled);
        return std::forward<decltype(self)>(self);
    }

    bool enabled() const { return m_enabled; }
    bool hovered() const { return m_hovered; }

    State<bool>& enabledState() { return m_enabledState; }
    State<bool>& hoveredState() { return m_hovered; }

protected:
    void render(Renderer& renderer) override;

    bool hasHandlerFor(EventType type) override;
    void triggerClick(MouseEvent& event) override;
    void triggerPress(MouseEvent& event) override;
    void triggerRelease(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;
    void triggerMouseLeave(MouseEvent& event) override;

private:
    Property<bool> m_enabled{true};
    State<bool> m_enabledState{true};
    State<bool> m_hovered{false};
};

}
