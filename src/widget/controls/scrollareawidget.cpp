#include <tavoos/widget/controls/scrollareawidget.h>

#include <tavoos/application.h>

namespace Tavoos {

ScrollAreaWidget::ScrollAreaWidget(Object* parent) : ScrollAreaSkin{parent} {
    installDefaultSlots();
    style(Application::instance()->theme().scrollArea);
}

}
