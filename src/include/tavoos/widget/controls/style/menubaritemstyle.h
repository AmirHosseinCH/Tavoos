#pragma once

#include <tavoos/text/font.h>
#include <tavoos/types.h>

namespace Tavoos {

struct MenuBarItemStyle {
    Paint highlightColor{Color::rgba(85, 112, 241, 40)};
    Paint hoverColor{Color::rgba(30, 32, 40, 14)};
    Paint textColor{Color::rgba(30, 32, 40)};
    Paint disabledTextColor{Color::rgba(160, 163, 175)};
    Font font;
    int radius{6};
};

}
