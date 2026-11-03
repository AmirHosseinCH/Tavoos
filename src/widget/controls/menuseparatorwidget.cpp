#include <tavoos/widget/controls/menuseparatorwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

MenuSeparatorWidget::MenuSeparatorWidget(Object* parent) : Control{parent} {
    fill(Fill::Width);

    m_thickness.onChange([this](const int&) { requestRelayout(); });
    m_spacing.onChange([this](const float&) { requestRelayout(); });

    content([this](RectangleWidget& line) {
        line.fill(Fill::Width).height(m_thickness.state()).color(m_color.state())
            .marginTop(m_spacing.state()).marginBottom(m_spacing.state());
    });

    m_style.onChange([this](const MenuSeparatorStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().menuSeparator);
}

void MenuSeparatorWidget::applyStyle(const MenuSeparatorStyle& value) {
    color(value.color);
    thickness(value.thickness);
    spacing(value.spacing);
}

}
