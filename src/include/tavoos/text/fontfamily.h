#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/fontface.h>

#include <memory>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT FontFamily {
public:
    explicit FontFamily(std::string name) : m_name{std::move(name)} {}

    void addFace(FT_Library ftLibrary, const std::string& sourcePath, FontWeight weight, FontStyle style = FontStyle::Normal);

    FontFace* resolveFace(FontWeight weight, FontStyle style);

    const std::string& name() const { return m_name; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<FontFace>> m_faces;
};

}