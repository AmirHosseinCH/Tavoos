#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace lunasvg { class Document; }

namespace Tavoos {

class SVGDocumentCache {
public:
    static lunasvg::Document* get(const std::string& uri);

private:
    static std::unordered_map<std::string, std::unique_ptr<lunasvg::Document>> s_cache;
};

}