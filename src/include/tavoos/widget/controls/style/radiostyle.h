#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct RadioStyle {
    Paint uncheckedColor{Color::Transparent};
    Paint checkedColor{Color::rgba(85, 112, 241)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint uncheckedBorderColor{Color::rgba(205, 208, 218)};
    Paint checkedBorderColor{Color::rgba(109, 125, 205)};
    Paint disabledBorderColor{Color::rgba(220, 222, 230)};
    int radius{-1};
    float transition{0.12f};
};

}
