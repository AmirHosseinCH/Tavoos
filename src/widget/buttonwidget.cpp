#include <tavoos/widget/buttonwidget.h>

namespace Tavoos {

ButtonWidget::ButtonWidget(Object* parent) : ButtonBase{parent} {
    m_color.set(m_idleColor.get());

    m_idleColor.onChange([this](const Paint&) { updateColor(false); });
    m_hoverColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    hoveredState().onChange([this](const bool&) { updateColor(true); });
    enabledState().onChange([this](const bool&) { updateColor(true); });

    background([this](RectangleWidget& rect) { rect.radius(8).color(m_color); });
}

void ButtonWidget::updateColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledColor.get() : hovered() ? m_hoverColor.get() : m_idleColor.get();
    m_color.animateTo(target, animate ? m_transition.get() : 0.0f);
}

}
