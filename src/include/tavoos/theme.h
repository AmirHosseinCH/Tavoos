#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/buttonstyle.h>

namespace Tavoos {

class Theme {
public:
    State<ButtonStyle> button{ButtonStyle{}};
};

}
