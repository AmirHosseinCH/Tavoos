#pragma once

#include <tavoos/types.h>

namespace Tavoos {

struct PopupStyle {
    Paint backgroundColor{Color::rgba(255, 255, 255)};
    Paint borderColor{Color::rgba(220, 222, 230)};
    Paint scrimColor{Color::rgba(0, 0, 0, 102)};
    float borderWidth{1.0f};
    float padding{8.0f};
    int radius{8};
};

}
