#pragma once

#include <tavoos/text/fontenums.h>

#include <string>

namespace Tavoos {

struct Font {
    std::string family;
    float size{14.0f};
    FontWeight weight{FontWeight::Regular};
    FontStyle style{FontStyle::Normal};

    bool operator==(const Font&) const = default;
};

}
