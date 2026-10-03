#include <tavoos/widget/progressbarwidget.h>

#include <tavoos/application.h>

#include <algorithm>

namespace Tavoos {

ProgressBarWidget::ProgressBarWidget(Object* parent) : Control{parent} {
    m_value.onChange([this](const int& value) {
        if (m_valueState.get() != value)
            m_valueState.set(value);
        updatePosition();
    });
    m_minValue.onChange([this](const int&) { updatePosition(); });
    m_maxValue.onChange([this](const int&) { updatePosition(); });

    width(200).height(8);
    updatePosition();

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_trackColor.state());
    });

    content<RectangleWidget>([this](RectangleWidget& fill) {
        fill.fill(Fill::Height)
            .alignment(Alignment::Left | Alignment::CenterVertical)
            .widthFraction(m_position)
            .radius(m_radius.state())
            .color(m_fillColor.state())
            .fillAnimation(true, m_transition.state());
    });

    m_style.onChange([this](const ProgressBarStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().progressBar);
}

void ProgressBarWidget::updatePosition() {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int val = std::clamp(m_value.get(), minV, maxV);
    const float fraction = static_cast<float>(val - minV) / static_cast<float>(maxV - minV);

    if (m_position.get() != fraction)
        m_position.set(fraction);
}

void ProgressBarWidget::applyStyle(const ProgressBarStyle& value) {
    trackColor(value.trackColor);
    fillColor(value.fillColor);
    radius(value.radius);
    transition(value.transition);
}

}
