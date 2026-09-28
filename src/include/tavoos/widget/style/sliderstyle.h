#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct SliderStyle {
    Paint trackColor{Color::rgba(228, 229, 235)};
    Paint fillColor{Color::rgba(85, 112, 241)};
    Paint thumbColor{Color::rgba(85, 112, 241)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint disabledThumbColor{Color::rgba(160, 163, 175)};
    float transition{0.12f};
};

}
