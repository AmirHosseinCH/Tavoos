#pragma once

#include <tavoos/export.hpp>
#include <tavoos/widget/controls/scrollareaskin.h>
#include <tavoos/widget/controls/style/listviewstyle.h>
#include <tavoos/widget/templates/listviewbase.h>

namespace Tavoos {

class TAVOOS_EXPORT ListViewWidget : public ScrollAreaSkin<ListViewBase> {
public:
    ListViewWidget(Object* parent);
};

}
