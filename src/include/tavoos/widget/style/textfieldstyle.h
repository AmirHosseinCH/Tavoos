#pragma once

#include <tavoos/text/font.h>
#include <tavoos/types.h>

namespace Tavoos {

struct TextFieldStyle {
    Paint backgroundColor{Color::White};
    Paint borderColor{Color::rgba(205, 208, 218)};
    Paint focusedBorderColor{Color::rgba(85, 112, 241)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint disabledBorderColor{Color::rgba(220, 222, 230)};
    Paint textColor{Color::Black};
    Paint placeholderColor{Color::rgba(160, 163, 175)};
    Paint caretColor{Color::Black};
    int radius{6};
    float borderWidth{1.5f};
    float innerPaddingLeft{10.0f};
    float innerPaddingTop{0.0f};
    float innerPaddingRight{10.0f};
    float innerPaddingBottom{0.0f};
    Font font;
    float transition{0.12f};
};

}
