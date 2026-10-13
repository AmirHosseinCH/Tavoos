#include <tavoos/widget/controls/listviewwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

ListViewWidget::ListViewWidget(Object* parent) : ScrollAreaSkin{parent} {
    installDefaultSlots();
    style(Application::instance()->theme().listView);
}

}
