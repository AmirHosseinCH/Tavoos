#pragma once

#include <tavoos/text/font.h>
#include <tavoos/types.h>

namespace Tavoos {

struct MenuItemStyle {
    Paint highlightColor{Color::rgba(85, 112, 241, 40)};
    Paint textColor{Color::rgba(30, 32, 40)};
    Paint disabledTextColor{Color::rgba(160, 163, 175)};
    Paint shortcutColor{Color::rgba(130, 133, 145)};
    Paint checkColor{Color::rgba(85, 112, 241)};
    Font font;
    int radius{6};
};

}
