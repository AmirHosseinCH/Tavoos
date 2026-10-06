#include <tavoos/widget/controls/flickareawidget.h>

#include <tavoos/application.h>

namespace Tavoos {

FlickAreaWidget::FlickAreaWidget(Object* parent) : FlickAreaBase{parent} {
    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundColor.state());
    });

    m_style.onChange([this](const FlickAreaStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().flickArea);
}

void FlickAreaWidget::applyStyle(const FlickAreaStyle& value) {
    backgroundColor(value.backgroundColor);
    radius(value.radius);
}

}
