#include <tavoos/widget/controls/menuwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

MenuWidget::MenuWidget(Object* parent) : MenuBase{parent} {
    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
            .color(m_backgroundColor.state())
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColor.state());
    });

    m_style.onChange([this](const MenuStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().menu);
}

void MenuWidget::applyStyle(const MenuStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    borderWidth(value.borderWidth);
    radius(value.radius);
    padding(value.padding);
    enter(value.enter);
    exit(value.exit);
}

}
