#pragma once

#include <tavoos/gfx/visibility.h>
#include <tavoos/text/fontenums.h>
#include <tavoos/text/utf8.h>
#include <tavoos/types.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace Tavoos {

class FontFace;

struct TextLine {
    std::size_t byteStart, byteEnd;
    float width;
    std::string overrideText;
};

struct VisibleSpan {
    std::size_t line;
    std::size_t start;
    std::size_t end;
    float x;
};

struct ElidedLine {
    std::string text;
    float width;
};

std::vector<TextLine> layoutTextLines(const std::string& text, FontFace& face, float scale,
                                      WrapMode wrapMode, float wrapWidth);

ElidedLine elideLine(const std::string& line, FontFace& face, float scale,
                     ElideMode mode, float maxWidth);

int effectiveMaxLines(int explicitMaxLines, float resolvedHeight, float lineHeight);

std::vector<TextLine> clampLines(std::vector<TextLine> lines, int effectiveLimit,
                                 const std::string& text, FontFace& face, float scale,
                                 ElideMode elideMode, float maxWidth);

template <typename LineStartX, typename Advance>
std::vector<VisibleSpan> visibleSpans(const std::string& text, const std::vector<TextLine>& lines,
                                      float firstLineTop, float lineHeight, const std::optional<Bounds>& visible,
                                      LineStartX&& lineStartX, Advance&& advance) {
    std::vector<VisibleSpan> spans;
    for (std::size_t li = 0; li < lines.size(); ++li) {
        const TextLine& line = lines[li];
        const bool original = line.overrideText.empty();
        const std::string& source = original ? text : line.overrideText;
        const std::size_t end = original ? line.byteEnd : source.size();
        std::size_t start = original ? line.byteStart : 0;
        float x = lineStartX(li);
        if (!visible) {
            spans.push_back({li, start, end, x});
            continue;
        }

        const float top = firstLineTop + lineHeight * static_cast<float>(li);
        if (top + lineHeight < visible->top - lineHeight || top > visible->bottom + lineHeight)
            continue;

        const float left = visible->left - lineHeight;
        const float right = visible->right + lineHeight;
        while (start < end) {
            const Utf8Decoded d = decodeUtf8At(source, start);
            const float width = advance(d.codepoint);
            if (x + width >= left)
                break;
            x += width;
            start += d.length;
        }
        std::size_t stop = start;
        for (float pen = x; stop < end && pen <= right; ) {
            const Utf8Decoded d = decodeUtf8At(source, stop);
            pen += advance(d.codepoint);
            stop += d.length;
        }
        if (start < stop)
            spans.push_back({li, start, stop, x});
    }
    return spans;
}

inline std::size_t lineAt(const std::vector<TextLine>& lines, std::size_t position) {
    const auto after = std::upper_bound(lines.begin(), lines.end(), position,
                                        [](std::size_t p, const TextLine& line) { return p < line.byteStart; });
    return after == lines.begin() ? 0 : static_cast<std::size_t>(after - lines.begin()) - 1;
}

template <typename LineStartX, typename Advance>
Rect positionToRect(const std::string& text, const std::vector<TextLine>& lines, std::size_t position,
                    float firstLineTop, float lineHeight, LineStartX&& lineStartX, Advance&& advance) {
    if (lines.empty())
        return {0.0f, firstLineTop, 0.0f, lineHeight};
    const std::size_t li = lineAt(lines, position);
    const TextLine& line = lines[li];
    const std::size_t end = std::min(position, line.byteEnd);
    float x = lineStartX(li);
    for (std::size_t i = line.byteStart; i < end; ) {
        const Utf8Decoded d = decodeUtf8At(text, i);
        x += advance(d.codepoint);
        i += d.length;
    }
    return {x, firstLineTop + lineHeight * static_cast<float>(li), 0.0f, lineHeight};
}

template <typename LineStartX, typename Advance>
std::size_t positionAt(const std::string& text, const std::vector<TextLine>& lines, float x, float y,
                       float firstLineTop, float lineHeight, LineStartX&& lineStartX, Advance&& advance) {
    if (lines.empty())
        return 0;
    std::size_t li = 0;
    if (lineHeight > 0.0f && y > firstLineTop)
        li = std::min(static_cast<std::size_t>((y - firstLineTop) / lineHeight), lines.size() - 1);
    const TextLine& line = lines[li];
    float pen = lineStartX(li);
    std::size_t i = line.byteStart;
    while (i < line.byteEnd) {
        const Utf8Decoded d = decodeUtf8At(text, i);
        const float width = advance(d.codepoint);
        if (x < pen + width * 0.5f)
            return i;
        pen += width;
        i += d.length;
    }
    return line.byteEnd;
}

}
