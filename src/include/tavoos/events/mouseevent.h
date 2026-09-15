#pragma once

#include <tavoos/events/event.h>
#include <tavoos/types.h>

#include <cstdint>

namespace Tavoos {

enum class MouseButton {
    Left,
    Right,
    Middle,
    Unknown,
};

enum class KeyModifier : std::uint32_t {
    None    = 0,
    Shift   = 1 << 0,
    Control = 1 << 1,
    Alt     = 1 << 2,
    Super   = 1 << 3,
};

template<>
struct EnableBitmaskOperators<KeyModifier> : std::true_type {};

class TAVOOS_EXPORT MouseEvent : public Event {
public:
    MouseEvent(EventType type, float x, float y, MouseButton button, KeyModifier mods)
        : Event{type}, m_x{x}, m_y{y}, m_button{button}, m_mods{mods} {}

    float x() const { return m_x; }
    float y() const { return m_y; }
    MouseButton button() const { return m_button; }
    KeyModifier modifiers() const { return m_mods; }

private:
    float m_x, m_y;
    MouseButton m_button;
    KeyModifier m_mods;
};

}