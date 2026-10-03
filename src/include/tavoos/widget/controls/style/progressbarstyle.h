#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct ProgressBarStyle {
    Paint trackColor{Color::rgba(228, 229, 235)};
    Paint fillColor{Color::rgba(85, 112, 241)};
    int radius{4};
    float transition{0.15f};
};

}
