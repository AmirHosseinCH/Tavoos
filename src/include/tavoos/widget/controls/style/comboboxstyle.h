#pragma once

#include <tavoos/text/font.h>
#include <tavoos/types.h>

namespace Tavoos {

struct ComboBoxStyle {
    Paint backgroundColor{Color::White};
    Paint hoverColor{Color::rgba(246, 247, 250)};
    Paint pressedColor{Color::rgba(238, 240, 245)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint borderColor{Color::rgba(205, 208, 218)};
    float borderWidth{1.5f};
    int radius{6};
    Paint textColor{Color::rgba(30, 32, 40)};
    Paint placeholderColor{Color::rgba(160, 163, 175)};
    Paint disabledTextColor{Color::rgba(160, 163, 175)};
    Font font;
    Paint indicatorColor{Color::rgba(110, 113, 125)};
    Paint highlightColor{Color::rgba(85, 112, 241, 40)};
    float transition{0.12f};
};

}
