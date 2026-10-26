#include <tavoos/widget/controls/tooltipwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/text.h>

namespace Tavoos {

ToolTipWidget::ToolTipWidget(Object* parent) : ToolTipBase{parent} {
    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
            .color(m_backgroundColor.state())
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColor.state());
    });

    addChild<TextWidget>([this](TextWidget& label) {
        label.text(textState()).color(m_textColor.state()).font(m_font.state());
    });

    m_style.onChange([this](const ToolTipStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().toolTip);
}

void ToolTipWidget::applyStyle(const ToolTipStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    borderWidth(value.borderWidth);
    radius(value.radius);
    padding(value.padding);
    textColor(value.textColor);
    font(value.font);
    enter(value.enter);
    exit(value.exit);
}

}
