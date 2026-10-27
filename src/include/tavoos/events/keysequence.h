#pragma once

#include <tavoos/events/key.h>
#include <tavoos/events/mouseevent.h>
#include <tavoos/export.hpp>

#include <string>
#include <string_view>

namespace Tavoos {

struct TAVOOS_EXPORT KeySequence {
    KeyModifier modifiers{KeyModifier::None};
    Key key{Key::Unknown};

    constexpr KeySequence() = default;
    constexpr KeySequence(Key key) : key{key} {}
    constexpr KeySequence(KeyModifier modifiers, Key key) : modifiers{modifiers}, key{key} {}

    static KeySequence fromString(std::string_view text);
    std::string toString() const;
    bool isEmpty() const { return key == Key::Unknown; }

    bool operator==(const KeySequence&) const = default;
};

}
