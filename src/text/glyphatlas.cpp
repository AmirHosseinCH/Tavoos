#include <tavoos/text/glyphatlas.h>

#include <vector>

namespace Tavoos {

GlyphAtlas::GlyphAtlas(int size) : m_size{size} {}

GlyphAtlas::~GlyphAtlas() {
    if (m_texture != 0)
        glDeleteTextures(1, &m_texture);
}

void GlyphAtlas::ensureTexture() {
    if (m_texture != 0)
        return;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::vector<unsigned char> blank(m_size * m_size, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_size, m_size, 0, GL_RED, GL_UNSIGNED_BYTE, blank.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool GlyphAtlas::pack(int width, int height, AtlasRect& outRect) {
    ensureTexture();

    int paddedWidth = width + Padding * 2;
    int paddedHeight = height + Padding * 2;

    if (m_cursorX + paddedWidth > m_size) {
        m_cursorX = 0;
        m_cursorY += m_shelfHeight;
        m_shelfHeight = 0;
    }
    if (m_cursorY + paddedHeight > m_size) {
        return false;
    }

    outRect = { m_cursorX + Padding, m_cursorY + Padding, width, height };

    m_cursorX += paddedWidth;
    m_shelfHeight = std::max(m_shelfHeight, paddedHeight);
    return true;
}

void GlyphAtlas::upload(const AtlasRect& rect, const unsigned char* pixels) {
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width, rect.height, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}