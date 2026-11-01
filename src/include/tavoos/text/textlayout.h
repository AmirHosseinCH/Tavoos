#pragma once

#include <tavoos/gfx/visibility.h>
#include <tavoos/text/fontenums.h>
#include <tavoos/text/utf8.h>

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

}
