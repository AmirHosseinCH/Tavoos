#pragma once

#include <tavoos/events/event.h>
#include <tavoos/export.hpp>

namespace Tavoos {

class TAVOOS_EXPORT WheelEvent : public Event {
public:
    WheelEvent(float deltaX, float deltaY)
        : Event{EventType::Wheel}, m_deltaX{deltaX}, m_deltaY{deltaY} {}

    float deltaX() const { return m_deltaX; }
    float deltaY() const { return m_deltaY; }

private:
    float m_deltaX, m_deltaY;
};

}
