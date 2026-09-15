#pragma once

#include <tavoos/export.hpp>

#include <cstddef>
#include <string>

namespace Tavoos {

class TAVOOS_EXPORT ResourceRegistry {
public:
    struct Entry {
        const unsigned char* data;
        std::size_t size;
    };

    static void registerResource(const std::string& path, const unsigned char* data, std::size_t size);
    static const Entry* find(const std::string& path);
};

}