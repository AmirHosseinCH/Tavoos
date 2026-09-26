#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/buttonstyle.h>
#include <tavoos/widget/checkboxstyle.h>

namespace Tavoos {

class Theme {
public:
    State<ButtonStyle> button{ButtonStyle{}};
    State<CheckboxStyle> checkbox{CheckboxStyle{}};
};

}
