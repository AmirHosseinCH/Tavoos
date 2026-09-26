#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/buttonstyle.h>
#include <tavoos/widget/checkboxstyle.h>
#include <tavoos/widget/radiostyle.h>

namespace Tavoos {

class Theme {
public:
    State<ButtonStyle> button{ButtonStyle{}};
    State<CheckboxStyle> checkbox{CheckboxStyle{}};
    State<RadioStyle> radio{RadioStyle{}};
};

}
