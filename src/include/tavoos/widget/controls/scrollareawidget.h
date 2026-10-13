#pragma once

#include <tavoos/export.hpp>
#include <tavoos/widget/controls/scrollareaskin.h>
#include <tavoos/widget/templates/scrollareabase.h>

namespace Tavoos {

class TAVOOS_EXPORT ScrollAreaWidget : public ScrollAreaSkin<ScrollAreaBase> {
public:
    ScrollAreaWidget(Object* parent);
};

}
