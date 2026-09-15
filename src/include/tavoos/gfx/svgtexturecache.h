#pragma once

#include <string>
#include <unordered_map>

namespace Tavoos {

class SVGTextureCache {
public:
    struct Entry {
        unsigned int texture{0};
        int refCount{0};
    };

    static Entry* acquire(const std::string& path, int width, int height);
    static void release(const std::string& path, int width, int height);
    static void releaseAll();

private:
    static std::string makeKey(const std::string& path, int width, int height);
    static std::unordered_map<std::string, Entry> s_cache;
};

}