#pragma once

#include <tavoos/export.hpp>

namespace Tavoos {

enum class EventType {
    None,
    MousePress,
    MouseRelease,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseClick,
    MouseDoubleClick,
    Wheel,
    KeyPress,
    KeyRelease,
    TextInput,
    FocusIn,
    FocusOut,
};

class TAVOOS_EXPORT Event {
public:
    explicit Event(EventType type) : m_type{type} {}
    virtual ~Event() = default;

    EventType type() const { return m_type; }

    bool isAccepted() const { return m_accepted; }
    void accept() { m_accepted = true; }
    void ignore() { m_accepted = false; }

private:
    EventType m_type;
    bool m_accepted{false};
};

}