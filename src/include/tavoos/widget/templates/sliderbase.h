#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/control.h>

namespace Tavoos {

class TAVOOS_EXPORT SliderBase : public Control {
public:
    SliderBase(Object* parent);

    template<typename W = RectangleWidget>
    decltype(auto) handle(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installHandle<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) value(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) minValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_minValue);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) maxValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_maxValue);
        return std::forward<decltype(self)>(self);
    }

    int value() const { return m_value; }
    int minValue() const { return m_minValue; }
    int maxValue() const { return m_maxValue; }
    float position() const { return m_position.get(); }
    bool pressed() const { return m_pressed.get(); }

    State<int>& valueState() { return m_valueState; }
    State<float>& positionState() { return m_position; }
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

    void updatePosition();
    float travel() const;
    int valueFromPosition(float x) const;
    bool pointOnHandle(float x) const;

    Property<int> m_value{0};
    State<int> m_valueState{0};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    State<float> m_position{0.0f};
    State<bool> m_pressed{false};

    Widget* m_handle{nullptr};
    int m_dragStartValue{0};
};

}
