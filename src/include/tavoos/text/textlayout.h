#pragma once

#include <tavoos/text/fontenums.h>

#include <string>
#include <vector>

namespace Tavoos {

class FontFace;

struct TextLine {
    std::size_t byteStart, byteEnd;
    float width;
    std::string overrideText;
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

}