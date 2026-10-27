#pragma once

#include <tavoos/events/keysequence.h>
#include <tavoos/export.hpp>
#include <tavoos/object.h>
#include <tavoos/reactive/reactive.h>

#include <functional>
#include <string_view>

namespace Tavoos {

class Window;

enum class ShortcutContext {
    Window,
    Application,
};

class TAVOOS_EXPORT Shortcut : public Object {
    friend class Window;

public:
    Shortcut(Object* parent);
    ~Shortcut() override;

    decltype(auto) sequence(this auto&& self, const KeySequence& sequence) {
        self.m_sequence.set(sequence);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) sequence(this auto&& self, std::string_view text) {
        self.m_sequence.set(KeySequence::fromString(text));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) context(this auto&& self, PropertyArg<ShortcutContext> context) {
        context.applyTo(self.m_context);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) enabled(this auto&& self, PropertyArg<bool> enabled) {
        enabled.applyTo(self.m_enabled);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) autoRepeat(this auto&& self, PropertyArg<bool> autoRepeat) {
        autoRepeat.applyTo(self.m_autoRepeat);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onActivated(this auto&& self, std::function<void()> callback) {
        self.m_onActivated = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onActivatedAmbiguously(this auto&& self, std::function<void()> callback) {
        self.m_onActivatedAmbiguously = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    KeySequence sequence() const { return m_sequence; }
    ShortcutContext context() const { return m_context; }
    bool enabled() const { return m_enabled; }
    bool autoRepeat() const { return m_autoRepeat; }

private:
    bool matches(const KeySequence& sequence, bool repeat) const;

    Window* m_window{nullptr};
    Property<KeySequence> m_sequence{KeySequence{}};
    Property<ShortcutContext> m_context{ShortcutContext::Window};
    Property<bool> m_enabled{true};
    Property<bool> m_autoRepeat{true};
    std::function<void()> m_onActivated;
    std::function<void()> m_onActivatedAmbiguously;
};

}
