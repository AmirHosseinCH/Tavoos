#include <tavoos/widget/controls/menubarwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/controls/menubaritemwidget.h>

namespace Tavoos {

MenuBarWidget::MenuBarWidget(Object* parent) : MenuBarBase{parent} {
    background([this](RectangleWidget& box) { box.color(m_backgroundColor.state()); });

    m_style.onChange([this](const MenuBarStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().menuBar);
}

MenuBarItemBase* MenuBarWidget::createItem() {
    return addChild<MenuBarItemWidget>();
}

void MenuBarWidget::applyStyle(const MenuBarStyle& value) {
    backgroundColor(value.backgroundColor);
    padding(value.padding);
}

}
