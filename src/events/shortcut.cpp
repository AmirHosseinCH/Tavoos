#include <tavoos/events/shortcut.h>

#include <tavoos/widget/widget.h>
#include <tavoos/window.h>

namespace Tavoos {

Shortcut::Shortcut(Object* parent) : Object{parent} {
    for (Object* object = parent; object && !m_window; object = object->parent()) {
        if (auto* const widget = dynamic_cast<Widget*>(object))
            m_window = widget->ownerWindow();
        else
            m_window = dynamic_cast<Window*>(object);
    }
    if (m_window)
        m_window->addShortcut(this);
}

Shortcut::~Shortcut() {
    if (m_window)
        m_window->removeShortcut(this);
}

bool Shortcut::matches(const KeySequence& sequence, bool repeat) const {
    if (!m_enabled.get() || m_sequence.get().isEmpty() || m_sequence.get() != sequence || (repeat && !m_autoRepeat.get()))
        return false;
    for (const Object* object = parent(); object; object = object->parent()) {
        if (const auto* const widget = dynamic_cast<const Widget*>(object); widget && !widget->visible())
            return false;
    }
    return true;
}

}
