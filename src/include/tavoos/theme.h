#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/style/buttonstyle.h>
#include <tavoos/widget/style/checkboxstyle.h>
#include <tavoos/widget/style/radiostyle.h>
#include <tavoos/widget/style/progressbarstyle.h>
#include <tavoos/widget/style/sliderstyle.h>
#include <tavoos/widget/style/spinboxstyle.h>
#include <tavoos/widget/style/switchstyle.h>
#include <tavoos/widget/style/textfieldstyle.h>

namespace Tavoos {

class Theme {
public:
    State<ButtonStyle> button{ButtonStyle{}};
    State<CheckboxStyle> checkbox{CheckboxStyle{}};
    State<RadioStyle> radio{RadioStyle{}};
    State<SwitchStyle> switchControl{SwitchStyle{}};
    State<ProgressBarStyle> progressBar{ProgressBarStyle{}};
    State<SliderStyle> slider{SliderStyle{}};
    State<TextFieldStyle> textField{TextFieldStyle{}};
    State<SpinBoxStyle> spinBox{SpinBoxStyle{}};
};

}
