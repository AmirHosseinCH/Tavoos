#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct ScrollAreaStyle {
    Paint backgroundColor{Color::Transparent};
    Paint trackColor{Color::Transparent};
    Paint thumbColor{Color::rgba(0, 0, 0, 90)};
    Paint thumbHoverColor{Color::rgba(0, 0, 0, 140)};
    Paint thumbPressedColor{Color::rgba(0, 0, 0, 190)};
    int radius{0};
    int thumbRadius{4};
    int barThickness{8};
    float barMargin{2.0f};
    float minThumbSize{24.0f};
    float barHideDelay{1.0f};
    float barFadeDuration{0.2f};
    float transition{0.12f};
};

}
