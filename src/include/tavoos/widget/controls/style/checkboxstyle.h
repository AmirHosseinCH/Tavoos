#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct CheckboxStyle {
    Paint uncheckedColor{Color::Transparent};
    Paint checkedColor{Color::rgba(85, 112, 241)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint uncheckedBorderColor{Color::rgba(205, 208, 218)};
    Paint checkedBorderColor{Color::rgba(109, 125, 205)};
    Paint disabledBorderColor{Color::rgba(220, 222, 230)};
    Paint checkColor{Color::rgba(176, 202, 217)};
    Paint disabledCheckColor{Color::rgba(160, 163, 175)};
    int radius{6};
    float transition{0.12f};
};

}
