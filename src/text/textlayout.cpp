#include <tavoos/text/fontface.h>
#include <tavoos/text/textlayout.h>
#include <tavoos/text/utf8.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

float measureWidth(const std::string& s, FontFace& face, float scale) {
    float w = 0.0f;
    std::size_t i = 0;
    while (i < s.size()) {
        const Utf8Decoded d = decodeUtf8At(s, i);
        i += d.length;
        const GlyphMetrics& m = face.glyph(d.codepoint);
        if (m.valid)
            w += m.advance * scale;
    }
    return w;
}

std::vector<std::pair<std::size_t,std::size_t>> codepointRanges(const std::string& s) {
    std::vector<std::pair<std::size_t,std::size_t>> out;
    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t start = i;
        const Utf8Decoded d = decodeUtf8At(s, i);
        i += d.length;
        out.push_back({start, i - start});
    }
    return out;
}

}

std::vector<TextLine> layoutTextLines(const std::string& text, FontFace& face, float scale,
                                      WrapMode wrapMode, float wrapWidth) {
    std::vector<TextLine> lines;

    std::size_t lineStart = 0;
    std::size_t lastBreakOpportunity = std::string::npos;
    float widthAtLastBreak = 0.0f;
    float lineWidth = 0.0f;

    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t charStart = i;
        const Utf8Decoded d = decodeUtf8At(text, i);
        const char32_t codepoint = d.codepoint;
        i += d.length;

        if (codepoint == '\n') {
            lines.push_back({lineStart, charStart, lineWidth, ""});
            lineStart = i;
            lineWidth = 0.0f;
            lastBreakOpportunity = std::string::npos;
            continue;
        }

        const GlyphMetrics& m = face.glyph(codepoint);
        const float advance = m.valid ? m.advance * scale : 0.0f;
        const bool isSpace = (codepoint == ' ' || codepoint == '\t');

        const bool needsBreak = (wrapMode != WrapMode::NoWrap) && wrapWidth > 0.0f &&
                          lineWidth + advance > wrapWidth && lineWidth > 0.0f;

        if (needsBreak) {
            if (wrapMode == WrapMode::WordWrap && lastBreakOpportunity != std::string::npos
                && lastBreakOpportunity > lineStart) {
                lines.push_back({lineStart, lastBreakOpportunity, widthAtLastBreak, ""});
                lineStart = lastBreakOpportunity + 1;
                lineWidth -= widthAtLastBreak;
            } else {
                lines.push_back({lineStart, charStart, lineWidth, ""});
                lineStart = charStart;
                lineWidth = 0.0f;
            }
            lastBreakOpportunity = std::string::npos;
        }

        if (wrapMode == WrapMode::WordWrap && isSpace) {
            lastBreakOpportunity = charStart;
            widthAtLastBreak = lineWidth;
        }

        lineWidth += advance;
    }

    lines.push_back({lineStart, text.size(), lineWidth, ""});
    return lines;
}

ElidedLine elideLine(const std::string& line, FontFace& face, float scale, ElideMode mode, float maxWidth) {
    const std::string ellipsis = "\xE2\x80\xA6";
    const float fullWidth = measureWidth(line, face, scale);
    const float ellipsisWidth = measureWidth(ellipsis, face, scale);

    if (mode == ElideMode::None || fullWidth + ellipsisWidth <= maxWidth)
        return { line, fullWidth };

    const float available = maxWidth - ellipsisWidth;
    if (available <= 0.0f)
        return { ellipsis, ellipsisWidth };

    const auto cps = codepointRanges(line);
    if (cps.empty())
        return { line, fullWidth };

    auto fitsPrefix = [&](std::size_t count) {
        std::size_t byteLen = (count == 0) ? 0 : cps[count-1].first + cps[count-1].second;
        return measureWidth(line.substr(0, byteLen), face, scale) <= available;
    };
    auto fitsSuffix = [&](std::size_t count) {
        std::size_t startByte = (count == 0) ? line.size() : cps[cps.size()-count].first;
        return measureWidth(line.substr(startByte), face, scale) <= available;
    };

    if (mode == ElideMode::Right) {
        std::size_t lo = 0, hi = cps.size();
        while (lo < hi) {
            const std::size_t mid = (lo + hi + 1) / 2;
            if (fitsPrefix(mid)) lo = mid; else hi = mid - 1;
        }
        const std::size_t byteLen = (lo == 0) ? 0 : cps[lo-1].first + cps[lo-1].second;
        const std::string result = line.substr(0, byteLen) + ellipsis;
        return { result, measureWidth(result, face, scale) };
    }

    if (mode == ElideMode::Left) {
        std::size_t lo = 0, hi = cps.size();
        while (lo < hi) {
            const std::size_t mid = (lo + hi + 1) / 2;
            if (fitsSuffix(mid)) lo = mid; else hi = mid - 1;
        }
        const std::size_t startByte = (lo == 0) ? line.size() : cps[cps.size()-lo].first;
        const std::string result = ellipsis + line.substr(startByte);
        return { result, measureWidth(result, face, scale) };
    }

    float half = available * 0.5f;
    std::size_t prefixCount = 0;
    while (prefixCount < cps.size()) {
        std::size_t byteLen = cps[prefixCount].first + cps[prefixCount].second;
        if (measureWidth(line.substr(0, byteLen), face, scale) > half)
            break;
        prefixCount++;
    }
    std::size_t suffixCount = 0;
    while (suffixCount < cps.size() - prefixCount) {
        std::size_t startByte = cps[cps.size()-suffixCount-1].first;
        std::string prefixPart = line.substr(0, (prefixCount==0)?0:cps[prefixCount-1].first+cps[prefixCount-1].second);
        std::string candidate = prefixPart + ellipsis + line.substr(startByte);
        if (measureWidth(candidate, face, scale) > maxWidth)
            break;
        suffixCount++;
    }
    std::string prefixPart = line.substr(0, (prefixCount==0)?0:cps[prefixCount-1].first+cps[prefixCount-1].second);
    std::string suffixPart = (suffixCount==0) ? "" : line.substr(cps[cps.size()-suffixCount].first);
    std::string result = prefixPart + ellipsis + suffixPart;
    return { result, measureWidth(result, face, scale) };
}

int effectiveMaxLines(int explicitMaxLines, float resolvedHeight, float lineHeight) {
    int fromHeight = 0;
    if (resolvedHeight > 0.0f && lineHeight > 0.0f)
        fromHeight = static_cast<int>(std::floor(resolvedHeight / lineHeight));

    if (explicitMaxLines > 0 && fromHeight > 0)
        return std::min(explicitMaxLines, fromHeight);
    if (explicitMaxLines > 0)
        return explicitMaxLines;
    if (fromHeight > 0)
        return fromHeight;
    return 0;
}

namespace {

void spliceEllipsis(TextLine& line, const std::string& text, FontFace& face, float scale,
                    float maxWidth, bool prepend) {
    const std::string lineText = line.overrideText.empty()
    ? text.substr(line.byteStart, line.byteEnd - line.byteStart)
    : line.overrideText;

    const std::string ellipsis = "\xE2\x80\xA6";
    const std::string withEllipsis = prepend ? (ellipsis + lineText) : (lineText + ellipsis);
    const float widthWithEllipsis = measureWidth(withEllipsis, face, scale);

    if (widthWithEllipsis <= maxWidth) {
        line.overrideText = withEllipsis;
        line.width = widthWithEllipsis;
        return;
    }

    ElidedLine e = elideLine(lineText, face, scale, prepend ? ElideMode::Left : ElideMode::Right, maxWidth);
    line.overrideText = e.text;
    line.width = e.width;
}

}

std::vector<TextLine> clampLines(std::vector<TextLine> lines, int effectiveLimit,
                                 const std::string& text, FontFace& face, float scale,
                                 ElideMode elideMode, float maxWidth) {
    const int totalLines = static_cast<int>(lines.size());
    const bool wasLineClamped = effectiveLimit > 0 && totalLines > effectiveLimit;

    if (!wasLineClamped) {
        if (elideMode == ElideMode::None || lines.empty() || maxWidth <= 0.0f)
            return lines;
        TextLine& last = lines.back();
        if (last.width <= maxWidth)
            return lines;
        spliceEllipsis(last, text, face, scale, maxWidth, elideMode == ElideMode::Left);
        return lines;
    }

    if (elideMode == ElideMode::None || maxWidth <= 0.0f) {
        lines.resize(effectiveLimit);
        return lines;
    }

    if (elideMode == ElideMode::Right) {
        std::vector<TextLine> kept(lines.begin(), lines.begin() + effectiveLimit);
        spliceEllipsis(kept.back(), text, face, scale, maxWidth, false);
        return kept;
    }

    if (elideMode == ElideMode::Left) {
        std::vector<TextLine> kept(lines.end() - effectiveLimit, lines.end());
        spliceEllipsis(kept.front(), text, face, scale, maxWidth, true);
        return kept;
    }

    const int prefixCount = (effectiveLimit + 1) / 2;
    const int suffixCount = effectiveLimit - prefixCount;

    std::vector<TextLine> kept(lines.begin(), lines.begin() + prefixCount);
    for (int k = totalLines - suffixCount; k < totalLines; ++k)
        kept.push_back(lines[k]);

    spliceEllipsis(kept[prefixCount - 1], text, face, scale, maxWidth, false);
    return kept;
}

}