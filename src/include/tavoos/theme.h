#pragma once

#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/controls/style/buttonstyle.h>
#include <tavoos/widget/controls/style/checkboxstyle.h>
#include <tavoos/widget/controls/style/flickareastyle.h>
#include <tavoos/widget/controls/style/popupstyle.h>
#include <tavoos/widget/controls/style/radiostyle.h>
#include <tavoos/widget/controls/style/scrollareastyle.h>
#include <tavoos/widget/controls/style/progressbarstyle.h>
#include <tavoos/widget/controls/style/sliderstyle.h>
#include <tavoos/widget/controls/style/spinboxstyle.h>
#include <tavoos/widget/controls/style/switchstyle.h>
#include <tavoos/widget/controls/style/textfieldstyle.h>

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
    State<PopupStyle> popup{PopupStyle{}};
    State<FlickAreaStyle> flickArea{FlickAreaStyle{}};
    State<ScrollAreaStyle> scrollArea{ScrollAreaStyle{}};
    State<SpinBoxStyle> spinBox{SpinBoxStyle{}};
};

}
