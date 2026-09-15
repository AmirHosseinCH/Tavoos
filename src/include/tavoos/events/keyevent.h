#pragma once

#include <tavoos/events/event.h>
#include <tavoos/events/key.h>
#include <tavoos/events/mouseevent.h>
#include <tavoos/export.hpp>

namespace Tavoos {

class TAVOOS_EXPORT KeyEvent : public Event {
public:
    KeyEvent(EventType type, int keyCode, KeyModifier mods)
        : Event{type}, m_keyCode{keyCode}, m_mods{mods} {}

    int keyCode() const { return m_keyCode; }
    KeyModifier modifiers() const { return m_mods; }

private:
    int m_keyCode;
    KeyModifier m_mods;
};

}
