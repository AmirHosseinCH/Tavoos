#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/control.h>

namespace Tavoos {

class TAVOOS_EXPORT ProgressBarBase : public Control {
public:
    ProgressBarBase(Object* parent);

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

    State<int>& valueState() { return m_valueState; }
    State<float>& positionState() { return m_position; }

private:
    void updatePosition();

    Property<int> m_value{0};
    State<int> m_valueState{0};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    State<float> m_position{0.0f};
};

}
