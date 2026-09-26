#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/fontenums.h>
#include <tavoos/text/glyphatlas.h>
#include <tavoos/text/glyphmetrics.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Tavoos {

class TAVOOS_EXPORT FontFace {
public:
    static constexpr float ReferencePixelSize = 64.0f;

    FontFace(FT_Library ftLibrary, const std::string& sourcePath, FontWeight weight, FontStyle style);
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    FontWeight weight() const { return m_weight; }
    FontStyle style() const { return m_style; }
    bool isValid() const { return m_ftFace != nullptr; }

    float lineHeight() const { return m_lineHeight; }
    float ascender() const { return m_ascender; }
    float descender() const { return m_descender; }

    const GlyphMetrics& glyph(char32_t codepoint);
    GLuint atlasTexture(int page) const {
        return (page >= 0 && page < static_cast<int>(m_atlasPages.size())) ? m_atlasPages[page]->texture() : 0;
    }
    int atlasPageCount() const { return static_cast<int>(m_atlasPages.size()); }

private:
    void rasterizeGlyph(char32_t codepoint, GlyphMetrics& out);

    FT_Face m_ftFace{nullptr};
    FontWeight m_weight;
    FontStyle m_style;
    float m_lineHeight{0.0f};
    float m_ascender{0.0f};
    float m_descender{0.0f};

    std::unordered_map<char32_t, GlyphMetrics> m_glyphCache;
    std::vector<std::unique_ptr<GlyphAtlas>> m_atlasPages;
    int m_atlasPageSize{1024};

    std::string m_ownedFontData;
};

}