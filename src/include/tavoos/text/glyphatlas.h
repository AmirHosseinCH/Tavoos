#pragma once

#include <tavoos/export.hpp>
#include <tavoos/third_party/glad/glad.h>

namespace Tavoos {

struct AtlasRect { int x, y, width, height; };

class TAVOOS_EXPORT GlyphAtlas {
public:
    explicit GlyphAtlas(int size = 1024);
    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    bool pack(int width, int height, AtlasRect& outRect);

    void upload(const AtlasRect& rect, const unsigned char* pixels);

    GLuint texture() const { return m_texture; }
    int size() const { return m_size; }

private:
    void ensureTexture();

    int m_size;
    GLuint m_texture{0};

    int m_cursorX{0};
    int m_cursorY{0};
    int m_shelfHeight{0};

    static constexpr int Padding = 4;
};

}