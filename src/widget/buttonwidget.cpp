#include <tavoos/widget/buttonwidget.h>

namespace Tavoos {

ButtonWidget::ButtonWidget(Object* parent) : ButtonBase{parent} {
    m_color.set(m_idleColor.get());
    m_textColorOut.set(m_textColor.get());

    m_idleColor.onChange([this](const Paint&) { updateColor(false); });
    m_hoverColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    hoveredState().onChange([this](const bool&) { updateColor(true); });
    enabledState().onChange([this](const bool&) {
        updateColor(true);
        updateTextColor(true);
    });

    m_textColor.onChange([this](const Paint&) { updateTextColor(false); });
    m_disabledTextColor.onChange([this](const Paint&) { updateTextColor(false); });
    m_text.onChange([this](const std::string&) { requestRelayout(); });
    m_font.onChange([this](const Font&) { requestRelayout(); });

    background([this](RectangleWidget& rect) { rect.radius(m_radius.state()).color(m_color); });
    content([this](TextWidget& label) {
        label.text(m_text.state()).font(m_font.state()).color(m_textColorOut)
            .marginLeft(16).marginRight(16).marginTop(8).marginBottom(8);
    });
}

void ButtonWidget::updateColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledColor.get() : hovered() ? m_hoverColor.get() : m_idleColor.get();
    m_color.animateTo(target, animate ? m_transition.get() : 0.0f);
}

void ButtonWidget::updateTextColor(bool animate) {
    const Paint& target = enabled() ? m_textColor.get() : m_disabledTextColor.get();
    m_textColorOut.animateTo(target, animate ? m_transition.get() : 0.0f);
}

}
