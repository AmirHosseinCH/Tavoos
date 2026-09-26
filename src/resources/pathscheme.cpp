#include <tavoos/resources/pathscheme.h>

namespace Tavoos {

ParsedPath parseResourcePath(const std::string& uri) {
    if (uri.rfind("resource:/", 0) == 0)
        return { PathScheme::Resource, uri.substr(10) };
    if (uri.rfind("data:", 0) == 0)
        return { PathScheme::Data, uri.substr(5) };
    if (uri.rfind("file:", 0) == 0)
        return { PathScheme::File, uri.substr(5) };
    return { PathScheme::File, uri };
}

}