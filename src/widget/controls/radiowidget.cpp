#include <tavoos/widget/controls/radiowidget.h>

#include <tavoos/application.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

constexpr int kContentMargin = 3;

}

RadioWidget::RadioWidget(Object* parent) : ButtonBase{parent} {
    m_backgroundColorOut.set(m_uncheckedColor.get());
    m_contentColorOut.set(m_checkedColor.get());
    m_borderColorOut.set(m_uncheckedBorderColor.get());

    checkable(true);
    exclusive(true);
    checkedState().onChange([this](const bool&) { updateColor(true); });
    enabledState().onChange([this](const bool&) { updateColor(true); });

    m_uncheckedColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_uncheckedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });

    m_radius.onChange([this](const int&) { updateGeometry(); });

    width(20).height(20);

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundColorOut).borderWidth(1.5f).borderColor(m_borderColorOut);
    });

    content<RectangleWidget>([this](RectangleWidget& fill) {
        fill.fill(Fill::Both)
            .marginLeft(kContentMargin).marginRight(kContentMargin).marginTop(kContentMargin).marginBottom(kContentMargin)
            .radius(m_contentRadius).color(m_contentColorOut).opacity(m_contentOpacity);
    });

    m_style.onChange([this](const RadioStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().radio);
}

void RadioWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

void RadioWidget::updateColor(bool animate) {
    const bool isChecked = checked();
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get() : (isChecked ? m_checkedBorderColor.get() : m_uncheckedBorderColor.get());

    m_backgroundColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_uncheckedColor.get(), duration);
    m_contentColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_checkedColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
    m_contentOpacity.animateTo(isChecked ? 1.0f : 0.0f, duration);
}

void RadioWidget::onResolvedSizeChanged() {
    updateGeometry();
}

void RadioWidget::updateGeometry() {
    if (resolvedWidth() <= 0.0f || resolvedHeight() <= 0.0f)
        return;
    const int size = static_cast<int>(std::lround(std::min(resolvedWidth(), resolvedHeight())));
    if (!m_radiusOverridden) {
        const int autoRadius = size / 2;
        if (m_radius.get() != autoRadius)
            m_radius.set(autoRadius);
    }
    m_contentRadius.set(std::max(0, m_radius.get() - kContentMargin));
}

void RadioWidget::applyStyle(const RadioStyle& value) {
    uncheckedColor(value.uncheckedColor);
    checkedColor(value.checkedColor);
    disabledColor(value.disabledColor);
    uncheckedBorderColor(value.uncheckedBorderColor);
    checkedBorderColor(value.checkedBorderColor);
    disabledBorderColor(value.disabledBorderColor);
    if (value.radius >= 0)
        radius(value.radius);
    transition(value.transition);
}

}
