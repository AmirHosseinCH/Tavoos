#include <tavoos/resources/resourceregistry.h>

#include <mutex>
#include <unordered_map>

namespace Tavoos {

static std::mutex& registryMutex() {
    static std::mutex instance;
    return instance;
}

static std::unordered_map<std::string, ResourceRegistry::Entry>& registryMap() {
    static std::unordered_map<std::string, ResourceRegistry::Entry> instance;
    return instance;
}

void ResourceRegistry::registerResource(const std::string& path, const unsigned char* data, std::size_t size) {
    std::lock_guard lock{registryMutex()};
    registryMap()[path] = Entry{data, size};
}

const ResourceRegistry::Entry* ResourceRegistry::find(const std::string& path) {
    std::lock_guard lock{registryMutex()};
    auto it = registryMap().find(path);
    return it != registryMap().end() ? &it->second : nullptr;
}

}