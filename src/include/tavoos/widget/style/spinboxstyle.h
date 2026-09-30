#pragma once

#include <tavoos/types.h>
#include <tavoos/widget/style/buttonstyle.h>
#include <tavoos/widget/style/textfieldstyle.h>

namespace Tavoos {

struct SpinBoxStyle {
    Paint backgroundColor{Color::White};
    Paint borderColor{Color::rgba(205, 208, 218)};
    Paint focusedBorderColor{Color::rgba(85, 112, 241)};
    Paint disabledColor{Color::rgba(228, 229, 235)};
    Paint disabledBorderColor{Color::rgba(220, 222, 230)};
    int radius{6};
    float borderWidth{1.5f};
    float transition{0.12f};
    TextFieldStyle field{};
    ButtonStyle stepperButton{[] { ButtonStyle style; style.radius = 4; return style; }()};
};

}
