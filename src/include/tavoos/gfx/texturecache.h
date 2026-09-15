#pragma once

#include <string>
#include <unordered_map>

namespace Tavoos {

class TextureCache {
public:
    struct Entry {
        unsigned int texture{0};
        int width{0};
        int height{0};
        int refCount{0};
    };

    static Entry* acquire(const std::string& path);

    static void release(const std::string& path);

    static void releaseAll();

private:
    static std::unordered_map<std::string, Entry> s_cache;
};

}