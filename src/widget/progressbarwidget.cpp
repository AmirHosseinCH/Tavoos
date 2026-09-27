#include <tavoos/widget/progressbarwidget.h>

#include <tavoos/application.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

ProgressBarWidget::ProgressBarWidget(Object* parent) : SlotWidget{parent} {
    m_value.onChange([this](const int& value) {
        if (m_valueState.get() != value)
            m_valueState.set(value);
        updateGeometry();
    });
    m_minValue.onChange([this](const int&) { updateGeometry(); });
    m_maxValue.onChange([this](const int&) { updateGeometry(); });
    widthProperty().onChange([this](const int&) { updateGeometry(); });

    width(200).height(8);
    updateGeometry();

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_trackColor.state());
    });

    content<RectangleWidget>([this](RectangleWidget& fill) {
        fill.fill(Fill::Height)
            .alignment(Alignment::Left | Alignment::CenterVertical)
            .width(m_fillWidth)
            .radius(m_radius.state())
            .color(m_fillColor.state())
            .fillAnimation(true, m_transition.state());
    });

    m_style.onChange([this](const ProgressBarStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().progressBar);
}

void ProgressBarWidget::updateGeometry() {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int val = std::clamp(m_value.get(), minV, maxV);
    const float fraction = static_cast<float>(val - minV) / static_cast<float>(maxV - minV);

    const int fillW = static_cast<int>(std::round(fraction * static_cast<float>(width())));
    if (m_fillWidth.get() != fillW)
        m_fillWidth.set(fillW);
}

void ProgressBarWidget::applyStyle(const ProgressBarStyle& value) {
    trackColor(value.trackColor);
    fillColor(value.fillColor);
    radius(value.radius);
    transition(value.transition);
}

}
