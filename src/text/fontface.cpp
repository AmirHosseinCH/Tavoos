#include <tavoos/resources/resourceregistry.h>
#include <tavoos/text/fontface.h>

#include <cstddef>

#include <spdlog/spdlog.h>

namespace Tavoos {

namespace {

struct ParsedSource { bool isResource; std::string path; };

ParsedSource parseSource(const std::string& uri) {
    if (uri.rfind("resource:/", 0) == 0)
        return { true, uri.substr(10) };
    if (uri.rfind("file:", 0) == 0)
        return { false, uri.substr(5) };
    return { false, uri };
}

}

FontFace::FontFace(FT_Library ftLibrary, const std::string& sourcePath, FontWeight weight, FontStyle style)
    : m_weight{weight}, m_style{style} {

    const ParsedSource parsed = parseSource(sourcePath);
    FT_Error err;

    if (parsed.isResource) {
        const ResourceRegistry::Entry* res = ResourceRegistry::find(parsed.path);
        if (!res) {
            spdlog::error("font resource not found: '{}'", sourcePath);
            return;
        }
        err = FT_New_Memory_Face(ftLibrary, res->data, static_cast<FT_Long>(res->size), 0, &m_ftFace);
    } else {
        err = FT_New_Face(ftLibrary, parsed.path.c_str(), 0, &m_ftFace);
    }

    if (err != 0) {
        spdlog::error("failed to load font: '{}' (FreeType error {})", sourcePath, err);
        m_ftFace = nullptr;
        return;
    }

    FT_Set_Pixel_Sizes(m_ftFace, 0, static_cast<FT_UInt>(ReferencePixelSize));

    m_lineHeight = static_cast<float>(m_ftFace->size->metrics.height) / 64.0f;
    m_ascender   = static_cast<float>(m_ftFace->size->metrics.ascender) / 64.0f;
    m_descender  = static_cast<float>(m_ftFace->size->metrics.descender) / 64.0f;
}

FontFace::~FontFace() {
    if (m_ftFace)
        FT_Done_Face(m_ftFace);
}

const GlyphMetrics& FontFace::glyph(char32_t codepoint) {
    auto it = m_glyphCache.find(codepoint);
    if (it != m_glyphCache.end())
        return it->second;

    GlyphMetrics metrics{};
    rasterizeGlyph(codepoint, metrics);
    auto result = m_glyphCache.emplace(codepoint, metrics);
    return result.first->second;
}

void FontFace::rasterizeGlyph(char32_t codepoint, GlyphMetrics& out) {
    if (!m_ftFace) {
        out.valid = false;
        return;
    }

    const FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, codepoint);
    if (glyphIndex == 0) {
        spdlog::warn("glyph not found for codepoint U+{:04X}", static_cast<unsigned int>(codepoint));
        out.valid = false;
        return;
    }

    FT_Error err = FT_Load_Glyph(m_ftFace, glyphIndex, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
    if (err != 0) {
        spdlog::warn("failed to load glyph U+{:04X} (FreeType error {})", static_cast<unsigned int>(codepoint), err);
        out.valid = false;
        return;
    }

    const FT_GlyphSlot slot = m_ftFace->glyph;

    out.valid = true;
    out.advance  = static_cast<float>(slot->advance.x) / 64.0f;
    out.bearingX = static_cast<float>(slot->metrics.horiBearingX) / 64.0f;
    out.bearingY = static_cast<float>(slot->metrics.horiBearingY) / 64.0f;
    out.width    = static_cast<float>(slot->metrics.width) / 64.0f;
    out.height   = static_cast<float>(slot->metrics.height) / 64.0f;

    if (out.width <= 0.0f || out.height <= 0.0f) {
        out.u0 = out.v0 = out.u1 = out.v1 = 0.0f;
        out.atlasPage = -1;
        return;
    }

    err = FT_Render_Glyph(
        slot,
        FT_RENDER_MODE_NORMAL
        );

    if (err != 0) {
        spdlog::warn(
            "normal render failed for U+{:04X} (FreeType error {})",
            static_cast<unsigned int>(codepoint),
            err
            );
        out.valid = false;
        return;
    }

    err = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF);
    if (err != 0) {
        spdlog::warn("SDF render failed for U+{:04X} (FreeType error {}) — falling back to normal render",
                     static_cast<unsigned int>(codepoint), err);
        err = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (err != 0) {
            out.valid = false;
            return;
        }
    }

    const FT_Bitmap& bmp = slot->bitmap;
    if (bmp.width == 0 || bmp.rows == 0) {
        out.u0 = out.v0 = out.u1 = out.v1 = 0.0f;
        out.atlasPage = -1;
        return;
    }

    out.width  = static_cast<float>(bmp.width);
    out.height = static_cast<float>(bmp.rows);
    out.bearingY = static_cast<float>(slot->bitmap_top);
    out.bearingX = static_cast<float>(slot->bitmap_left);

    AtlasRect rect;
    int pageIndex = -1;

    for (std::size_t i = 0; i < m_atlasPages.size(); ++i) {
        if (m_atlasPages[i]->pack(static_cast<int>(bmp.width), static_cast<int>(bmp.rows), rect)) {
            pageIndex = static_cast<int>(i);
            break;
        }
    }

    if (pageIndex == -1) {
        auto newPage = std::make_unique<GlyphAtlas>(m_atlasPageSize);
        if (!newPage->pack(static_cast<int>(bmp.width), static_cast<int>(bmp.rows), rect)) {
            spdlog::error("glyph U+{:04X} ({}x{}) too large for atlas page size {}",
                          static_cast<unsigned int>(codepoint), bmp.width, bmp.rows, m_atlasPageSize);
            out.valid = false;
            return;
        }
        m_atlasPages.push_back(std::move(newPage));
        pageIndex = static_cast<int>(m_atlasPages.size()) - 1;
    }

    GlyphAtlas* const atlas = m_atlasPages[pageIndex].get();

    if (bmp.pitch == static_cast<int>(bmp.width)) {
        atlas->upload(rect, bmp.buffer);
    } else {
        std::vector<unsigned char> tight(bmp.width * bmp.rows);
        for (unsigned int row = 0; row < bmp.rows; ++row)
            std::memcpy(tight.data() + row * bmp.width, bmp.buffer + static_cast<std::ptrdiff_t>(row) * bmp.pitch, bmp.width);
        atlas->upload(rect, tight.data());
    }

    const float atlasSize = static_cast<float>(atlas->size());
    out.atlasPage = pageIndex;
    out.u0 = static_cast<float>(rect.x) / atlasSize;
    out.v0 = static_cast<float>(rect.y) / atlasSize;
    out.u1 = static_cast<float>(rect.x + rect.width) / atlasSize;
    out.v1 = static_cast<float>(rect.y + rect.height) / atlasSize;
}

}