#include <tavoos/widget/controls/popupwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

PopupWidget::PopupWidget(Object* parent) : PopupBase{parent} {
    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
            .color(m_backgroundColor.state())
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColor.state());
    });

    scrim([this](RectangleWidget& dim) {
        dim.color(m_scrimColor.state());
    });

    m_style.onChange([this](const PopupStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().popup);
}

void PopupWidget::applyStyle(const PopupStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    scrimColor(value.scrimColor);
    borderWidth(value.borderWidth);
    radius(value.radius);
    padding(value.padding);
    enter(value.enter);
    exit(value.exit);
}

}
