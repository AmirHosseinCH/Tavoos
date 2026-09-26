#include <tavoos/gfx/svgdocumentcache.h>
#include <tavoos/resources/resources.h>

#include <lunasvg.h>
#include <spdlog/spdlog.h>

namespace Tavoos {

std::unordered_map<std::string, std::unique_ptr<lunasvg::Document>> SVGDocumentCache::s_cache;

lunasvg::Document* SVGDocumentCache::get(const std::string& uri) {
    auto it = s_cache.find(uri);
    if (it != s_cache.end())
        return it->second.get();

    const ParsedPath parsed = parseResourcePath(uri);
    std::unique_ptr<lunasvg::Document> document;

    if (parsed.scheme == PathScheme::Resource) {
        const ResourceRegistry::Entry* res = ResourceRegistry::find(parsed.path);
        if (!res) {
            spdlog::warn("SVG resource not found: '{}'", uri);
            return nullptr;
        }
        document = lunasvg::Document::loadFromData(
            reinterpret_cast<const char*>(res->data), res->size);
    } else if (parsed.scheme == PathScheme::Data) {
        document = lunasvg::Document::loadFromData(parsed.path.c_str(), parsed.path.size());
    } else {
        document = lunasvg::Document::loadFromFile(parsed.path);
    }

    if (!document) {
        spdlog::warn("failed to load SVG: '{}'", uri);
        return nullptr;
    }

    auto* const raw = document.get();
    s_cache.emplace(uri, std::move(document));
    return raw;
}

}