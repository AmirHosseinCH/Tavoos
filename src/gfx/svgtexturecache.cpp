#include <tavoos/gfx/svgdocumentcache.h>
#include <tavoos/gfx/svgtexturecache.h>
#include <tavoos/third_party/glad/glad.h>

#include <lunasvg.h>
#include <spdlog/spdlog.h>

namespace Tavoos {

std::unordered_map<std::string, SVGTextureCache::Entry> SVGTextureCache::s_cache;

std::string SVGTextureCache::makeKey(const std::string& path, int width, int height) {
    return path + ":" + std::to_string(width) + "x" + std::to_string(height);
}

SVGTextureCache::Entry* SVGTextureCache::acquire(const std::string& path, int width, int height) {
    constexpr int kMaxDimension = 8192;
    if (width <= 0 || height <= 0 || width > kMaxDimension || height > kMaxDimension) {
        spdlog::warn("refusing to rasterize '{}' at {}x{} (must be in [1,{}])", path, width, height, kMaxDimension);
        return nullptr;
    }

    const std::string key = makeKey(path, width, height);
    auto it = s_cache.find(key);
    if (it != s_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    lunasvg::Document* const document = SVGDocumentCache::get(path);
    if (!document)
        return nullptr;

    const lunasvg::Bitmap bitmap = document->renderToBitmap(width, height);
    if (bitmap.isNull()) {
        spdlog::warn("failed to rasterize SVG '{}' at {}x{}", path, width, height);
        return nullptr;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap.width(), bitmap.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    auto result = s_cache.emplace(key, Entry{texture, 1});
    return &result.first->second;
}

void SVGTextureCache::release(const std::string& path, int width, int height) {
    const std::string key = makeKey(path, width, height);
    auto it = s_cache.find(key);
    if (it == s_cache.end())
        return;
    it->second.refCount--;
    if (it->second.refCount <= 0) {
        glDeleteTextures(1, &it->second.texture);
        s_cache.erase(it);
    }
}

void SVGTextureCache::releaseAll() {
    for (auto& [path, entry] : s_cache) {
        if (entry.texture != 0) {
            glDeleteTextures(1, &entry.texture);
        }
    }
    s_cache.clear();
}

}