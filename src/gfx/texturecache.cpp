#include <tavoos/gfx/texturecache.h>
#include <tavoos/resources/resources.h>
#include <tavoos/third_party/glad/glad.h>

#include <spdlog/spdlog.h>
#include <stb_image/stb_image.h>

namespace Tavoos {

std::unordered_map<std::string, TextureCache::Entry> TextureCache::s_cache;

TextureCache::Entry* TextureCache::acquire(const std::string& uri) {
    auto it = s_cache.find(uri);
    if (it != s_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    const ParsedPath parsed = parseResourcePath(uri);
    int w, h, channels;
    unsigned char* data = nullptr;

    if (parsed.scheme == PathScheme::Resource) {
        const ResourceRegistry::Entry* res = ResourceRegistry::find(parsed.path);
        if (!res) {
            spdlog::warn("resource not found: '{}'", uri);
            return nullptr;
        }
        data = stbi_load_from_memory(res->data, static_cast<int>(res->size), &w, &h, &channels, 4);
    } else {
        data = stbi_load(parsed.path.c_str(), &w, &h, &channels, 4);
    }

    if (!data) {
        spdlog::warn("failed to load image: '{}'", uri);
        return nullptr;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    auto [inserted, _] = s_cache.emplace(uri, Entry{texture, w, h, 1});
    return &inserted->second;
}

void TextureCache::release(const std::string& path) {
    auto it = s_cache.find(path);
    if (it == s_cache.end())
        return;

    it->second.refCount--;
    if (it->second.refCount <= 0) {
        glDeleteTextures(1, &it->second.texture);
        s_cache.erase(it);
    }
}

void TextureCache::releaseAll() {
    for (auto& [path, entry] : s_cache) {
        if (entry.texture != 0)
            glDeleteTextures(1, &entry.texture);
    }
    s_cache.clear();
}

}