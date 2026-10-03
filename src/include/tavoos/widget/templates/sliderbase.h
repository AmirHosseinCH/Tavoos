#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/rangebase.h>

namespace Tavoos {

class TAVOOS_EXPORT SliderBase : public RangeBase {
public:
    SliderBase(Object* parent);

    template<typename W = RectangleWidget>
    decltype(auto) handle(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installHandle<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    bool pressed() const { return m_pressed.get(); }

    State<bool>& pressedState() { return m_pressed; }

protected:
    bool hasHandlerFor(EventType type) override;
    void triggerPress(MouseEvent& event) override;
    void triggerRelease(MouseEvent& event) override;
    void triggerDragStart(DragEvent& event) override;
    void triggerDragMove(DragEvent& event) override;
    void triggerDragEnd(DragEvent& event) override;

private:
    template<typename W>
    void installHandle(std::function<void(W&)> body) {
        if (m_handle) {
            removeChild(m_handle);
            m_handle = nullptr;
        }
        m_handle = addChild<W>([&body](W& slot) {
            if (body)
                body(slot);
        });
    }

    float travel() const;
    int valueFromPosition(float x) const;
    bool pointOnHandle(float x) const;

    State<bool> m_pressed{false};

    Widget* m_handle{nullptr};
    int m_dragStartValue{0};
};

}
