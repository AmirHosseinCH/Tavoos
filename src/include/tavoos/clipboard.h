#pragma once

#include <tavoos/export.hpp>

#include <string>

namespace Tavoos {

class TAVOOS_EXPORT Clipboard {
public:
    static std::string text();
    static void setText(const std::string& text);
};

}
