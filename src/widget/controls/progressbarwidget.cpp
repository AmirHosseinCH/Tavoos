#include <tavoos/widget/controls/progressbarwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

ProgressBarWidget::ProgressBarWidget(Object* parent) : ProgressBarBase{parent} {
    width(200).height(8);

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_trackColor.state());
    });

    content<RectangleWidget>([this](RectangleWidget& fill) {
        fill.fill(Fill::Height)
            .alignment(Alignment::Left | Alignment::CenterVertical)
            .widthFraction(positionState())
            .radius(m_radius.state())
            .color(m_fillColor.state())
            .fillAnimation(true, m_transition.state());
    });

    m_style.onChange([this](const ProgressBarStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().progressBar);
}

void ProgressBarWidget::applyStyle(const ProgressBarStyle& value) {
    trackColor(value.trackColor);
    fillColor(value.fillColor);
    radius(value.radius);
    transition(value.transition);
}

}
