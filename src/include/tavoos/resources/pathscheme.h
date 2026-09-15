#pragma once

#include <string>

namespace Tavoos {

enum class PathScheme { File, Resource };

struct ParsedPath {
    PathScheme scheme;
    std::string path;
};

ParsedPath parseResourcePath(const std::string& uri);

}