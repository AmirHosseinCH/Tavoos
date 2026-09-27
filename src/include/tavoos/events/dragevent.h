#pragma once

#include <tavoos/events/event.h>
#include <tavoos/export.hpp>

namespace Tavoos {

class TAVOOS_EXPORT DragEvent : public Event {
public:
    DragEvent(EventType type, float dx, float dy, float totalDx, float totalDy)
        : Event{type}, m_dx{dx}, m_dy{dy}, m_totalDx{totalDx}, m_totalDy{totalDy} {}

    float dx() const { return m_dx; }
    float dy() const { return m_dy; }
    float totalDx() const { return m_totalDx; }
    float totalDy() const { return m_totalDy; }

private:
    float m_dx, m_dy;
    float m_totalDx, m_totalDy;
};

}
