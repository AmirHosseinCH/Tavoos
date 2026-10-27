#include <tavoos/events/keysequence.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <vector>

namespace Tavoos {

namespace {

struct NamedModifier {
    std::string_view name;
    KeyModifier modifier;
};

struct NamedKey {
    std::string_view name;
    Key key;
};

constexpr NamedModifier kModifiers[] = {
    {"Ctrl", KeyModifier::Control}, {"Alt", KeyModifier::Alt}, {"Shift", KeyModifier::Shift}, {"Meta", KeyModifier::Super},
};

constexpr NamedModifier kModifierAliases[] = {
    {"Control", KeyModifier::Control}, {"Super", KeyModifier::Super},
};

constexpr NamedKey kKeys[] = {
    {"Space", Key::Space}, {"Enter", Key::Enter}, {"Return", Key::Enter}, {"Esc", Key::Escape}, {"Escape", Key::Escape},
    {"Tab", Key::Tab}, {"Backspace", Key::Backspace}, {"Ins", Key::Insert}, {"Insert", Key::Insert},
    {"Del", Key::Delete}, {"Delete", Key::Delete}, {"Home", Key::Home}, {"End", Key::End},
    {"PgUp", Key::PageUp}, {"PageUp", Key::PageUp}, {"PgDown", Key::PageDown}, {"PageDown", Key::PageDown},
    {"Left", Key::Left}, {"Right", Key::Right}, {"Up", Key::Up}, {"Down", Key::Down},
    {"-", Key::Minus}, {"=", Key::Equal}, {"[", Key::LeftBracket}, {"]", Key::RightBracket}, {";", Key::Semicolon},
    {"'", Key::Apostrophe}, {",", Key::Comma}, {".", Key::Period}, {"/", Key::Slash}, {"\\", Key::Backslash},
    {"`", Key::GraveAccent},
};

bool sameText(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
    });
}

std::string_view trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(' ');
    if (first == std::string_view::npos)
        return {};
    return text.substr(first, text.find_last_not_of(' ') - first + 1);
}

std::optional<KeyModifier> modifierNamed(std::string_view name) {
    for (const auto& entry : kModifiers) {
        if (sameText(entry.name, name))
            return entry.modifier;
    }
    for (const auto& entry : kModifierAliases) {
        if (sameText(entry.name, name))
            return entry.modifier;
    }
    return std::nullopt;
}

Key keyNamed(std::string_view name) {
    if (name.size() == 1) {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            return static_cast<Key>(c);
    }
    if (name.size() >= 2 && name.size() <= 3 && (name[0] == 'F' || name[0] == 'f')
        && std::ranges::all_of(name.substr(1), [](char c) { return c >= '0' && c <= '9'; })) {
        const int number = std::stoi(std::string{name.substr(1)});
        if (number >= 1 && number <= 25)
            return static_cast<Key>(static_cast<int>(Key::F1) + number - 1);
    }
    for (const auto& entry : kKeys) {
        if (sameText(entry.name, name))
            return entry.key;
    }
    return Key::Unknown;
}

std::string keyName(Key key) {
    const int code = static_cast<int>(key);
    if ((key >= Key::A && key <= Key::Z) || (key >= Key::Num0 && key <= Key::Num9))
        return std::string(1, static_cast<char>(code));
    if (key >= Key::F1 && key <= Key::F25)
        return "F" + std::to_string(code - static_cast<int>(Key::F1) + 1);
    for (const auto& entry : kKeys) {
        if (entry.key == key)
            return std::string{entry.name};
    }
    return {};
}

}

KeySequence KeySequence::fromString(std::string_view text) {
    std::vector<std::string_view> parts;
    for (std::size_t start = 0;;) {
        const std::size_t plus = text.find('+', start);
        parts.push_back(trimmed(text.substr(start, plus == std::string_view::npos ? std::string_view::npos : plus - start)));
        if (plus == std::string_view::npos)
            break;
        start = plus + 1;
    }

    KeySequence sequence;
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        const auto modifier = modifierNamed(parts[i]);
        if (!modifier) {
            spdlog::error("KeySequence: unknown modifier \"{}\" in \"{}\"", parts[i], text);
            return {};
        }
        sequence.modifiers |= *modifier;
    }
    sequence.key = keyNamed(parts.back());
    if (sequence.key == Key::Unknown) {
        spdlog::error("KeySequence: unknown key \"{}\" in \"{}\"", parts.back(), text);
        return {};
    }
    return sequence;
}

std::string KeySequence::toString() const {
    if (key == Key::Unknown)
        return {};
    std::string text;
    for (const auto& entry : kModifiers) {
        if (hasFlag(modifiers, entry.modifier)) {
            text += entry.name;
            text += '+';
        }
    }
    return text + keyName(key);
}

}
