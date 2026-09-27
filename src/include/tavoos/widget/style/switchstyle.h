#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct SwitchStyle {
    Paint uncheckedColor{Color::rgba(234, 237, 253)};
    Paint checkedColor{Color::rgba(187, 198, 249)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint thumbColor{Color::rgba(187, 197, 203)};
    Paint checkedThumbColor{Color::rgba(85, 112, 241)};
    Paint disabledThumbColor{Color::rgba(160, 163, 175)};
    float transition{0.12f};
};

}
