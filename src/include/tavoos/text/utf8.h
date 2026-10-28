#pragma once

#include <string>

namespace Tavoos {

struct Utf8Decoded {
    char32_t codepoint;
    int length;
};

constexpr Utf8Decoded decodeUtf8At(const std::string& content, std::size_t i) {
    unsigned char b = static_cast<unsigned char>(content[i]);
    char32_t codepoint = 0;
    int len = 1;
    if ((b & 0x80) == 0)         { codepoint = b;        len = 1; }
    else if ((b & 0xE0) == 0xC0) { codepoint = b & 0x1F;  len = 2; }
    else if ((b & 0xF0) == 0xE0) { codepoint = b & 0x0F;  len = 3; }
    else if ((b & 0xF8) == 0xF0) { codepoint = b & 0x07;  len = 4; }
    else {
        return { 0xFFFD, 1 };
    }
    int available = static_cast<int>(content.size() - i);
    int usable = (len < available) ? len : available;
    for (int k = 1; k < usable; ++k) {
        unsigned char cont = static_cast<unsigned char>(content[i + k]);
        if ((cont & 0xC0) != 0x80) {
            return { 0xFFFD, k };
        }
        codepoint = (codepoint << 6) | (cont & 0x3F);
    }
    if (usable < len)
        return { 0xFFFD, usable };

    static constexpr char32_t kMinForLength[] = { 0, 0, 0x80, 0x800, 0x10000 };
    bool overlong = codepoint < kMinForLength[len];
    bool surrogate = codepoint >= 0xD800 && codepoint <= 0xDFFF;
    bool tooLarge = codepoint > 0x10FFFF;
    if (overlong || surrogate || tooLarge)
        return { 0xFFFD, len };

    return { codepoint, len };
}

inline std::size_t previousCodepointStart(const std::string& content, std::size_t i) {
    if (i == 0)
        return 0;
    std::size_t j = i - 1;
    while (j > 0 && (static_cast<unsigned char>(content[j]) & 0xC0) == 0x80)
        --j;
    return j;
}

inline std::size_t codepointStartAt(const std::string& content, std::size_t i) {
    if (i >= content.size())
        return content.size();
    while (i > 0 && (static_cast<unsigned char>(content[i]) & 0xC0) == 0x80)
        --i;
    return i;
}

inline std::size_t byteOffsetOfCharacter(const std::string& content, std::size_t character) {
    std::size_t i = 0;
    for (; character > 0 && i < content.size(); --character)
        i += decodeUtf8At(content, i).length;
    return i;
}

inline std::size_t characterIndexAt(const std::string& content, std::size_t byte) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < byte && i < content.size(); i += decodeUtf8At(content, i).length)
        ++count;
    return count;
}

inline std::size_t characterCount(const std::string& content) {
    return characterIndexAt(content, content.size());
}

template <typename Fn>
inline void forEachUtf8Codepoint(const std::string& content, Fn&& fn) {
    for (std::size_t i = 0; i < content.size(); ) {
        Utf8Decoded d = decodeUtf8At(content, i);
        fn(d.codepoint);
        i += d.length;
    }
}

inline std::string encodeUtf8(char32_t codepoint) {
    std::string out;
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (codepoint >> 18));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    return out;
}

}