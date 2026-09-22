#pragma once

#include <tavoos/text/font.h>
#include <tavoos/types.h>

namespace Tavoos {

struct ButtonStyle {
    Paint idleColor{Color::rgba(60, 130, 255)};
    Paint hoverColor{Color::rgba(84, 148, 255)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint textColor{Color::White};
    Paint disabledTextColor{Color::rgba(160, 163, 175)};
    Paint borderColor{Color::rgba(205, 208, 218)};
    Paint disabledBorderColor{Color::rgba(220, 222, 230)};
    float borderWidth{0.0f};
    int radius{8};
    Font font;
    float transition{0.12f};
};

}
