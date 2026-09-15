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

template <typename Fn>
inline void forEachUtf8Codepoint(const std::string& content, Fn&& fn) {
    for (std::size_t i = 0; i < content.size(); ) {
        Utf8Decoded d = decodeUtf8At(content, i);
        fn(d.codepoint);
        i += d.length;
    }
}

}