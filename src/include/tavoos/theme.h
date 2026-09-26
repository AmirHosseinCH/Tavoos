#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/style/buttonstyle.h>
#include <tavoos/widget/style/checkboxstyle.h>
#include <tavoos/widget/style/radiostyle.h>

namespace Tavoos {

class Theme {
public:
    State<ButtonStyle> button{ButtonStyle{}};
    State<CheckboxStyle> checkbox{CheckboxStyle{}};
    State<RadioStyle> radio{RadioStyle{}};
};

}
