#include <tavoos/text/fontmanager.h>
#include <tavoos/text/systemfont.h>

#include <stdexcept>

#include <spdlog/spdlog.h>
#include FT_MODULE_H

namespace Tavoos {

FontManager& FontManager::instance() {
    static FontManager mgr;
    return mgr;
}

FontManager::FontManager() {
    FT_Error err = FT_Init_FreeType(&m_ftLibrary);
    if (err != 0)
        throw std::runtime_error{"failed to initialize FreeType"};
}

void FontManager::releaseAll() {
    m_families.clear();
    m_systemDefaultFamily.reset();
}

FontManager::~FontManager() {
    m_families.clear();
    m_systemDefaultFamily.reset();
    if (m_ftLibrary)
        FT_Done_FreeType(m_ftLibrary);
}

void FontManager::registerFont(const std::string& familyName, const std::string& sourcePath,
                               FontWeight weight, FontStyle style) {
    auto it = m_families.find(familyName);
    if (it == m_families.end()) {
        auto result = m_families.emplace(familyName, std::make_unique<FontFamily>(familyName));
        it = result.first;
    }
    it->second->addFace(m_ftLibrary, sourcePath, weight, style);
}

FontFamily* FontManager::family(const std::string& name) {
    if (!name.empty()) {
        auto it = m_families.find(name);
        if (it != m_families.end())
            return it->second.get();
        spdlog::warn("font family not registered: '{}' - falling back to the system default font", name);
    }
    return systemDefaultFamily();
}

FontFamily* FontManager::systemDefaultFamily() {
    if (m_systemDefaultFamily)
        return m_systemDefaultFamily.get();

    const std::string path = findSystemDefaultFontPath();
    if (path.empty()) {
        spdlog::error("could not locate a system default font");
        return nullptr;
    }

    auto fallback = std::make_unique<FontFamily>("__tavoos_system_default");
    fallback->addFace(m_ftLibrary, path, FontWeight::Regular, FontStyle::Normal);

    m_systemDefaultFamily = std::move(fallback);
    return m_systemDefaultFamily.get();
}

}