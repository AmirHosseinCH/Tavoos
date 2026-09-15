#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/fontfamily.h>

#include <memory>
#include <string>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Tavoos {

class TAVOOS_EXPORT FontManager {
    friend class Application;

public:
    static FontManager& instance();

    ~FontManager();
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    void registerFont(const std::string& familyName, const std::string& sourcePath,
                      FontWeight weight = FontWeight::Regular, FontStyle style = FontStyle::Normal);

    FontFamily* family(const std::string& name);

private:
    FontManager();
    void releaseAll();
    FontFamily* systemDefaultFamily();

    FT_Library m_ftLibrary{nullptr};
    std::unordered_map<std::string, std::unique_ptr<FontFamily>> m_families;
    std::unique_ptr<FontFamily> m_systemDefaultFamily;
};

}