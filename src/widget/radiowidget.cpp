#include <tavoos/widget/radiowidget.h>

#include <tavoos/application.h>

#include <algorithm>

namespace Tavoos {

namespace {

constexpr int kContentMargin = 3;

}

RadioWidget::RadioWidget(Object* parent) : ButtonBase{parent} {
    m_backgroundColorOut.set(m_unselectedColor.get());
    m_contentColorOut.set(m_selectedColor.get());
    m_borderColorOut.set(m_unselectedBorderColor.get());

    m_selected.onChange([this](const bool& selected) {
        if (m_selectedState.get() != selected)
            m_selectedState.set(selected);
        updateColor(true);
    });
    enabledState().onChange([this](const bool&) { updateColor(true); });

    m_unselectedColor.onChange([this](const Paint&) { updateColor(false); });
    m_selectedColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_unselectedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_selectedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });

    m_radius.onChange([this](const int&) { updateGeometry(); });
    widthProperty().onChange([this](const int&) { updateGeometry(); });
    heightProperty().onChange([this](const int&) { updateGeometry(); });

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

void RadioWidget::handleClick(MouseEvent& event) {
    if (enabled() && !m_selected.get()) {
        if (m_group)
            m_group->select(this);
        else
            m_selected.set(true);
    }
    ButtonBase::handleClick(event);
}

void RadioWidget::updateColor(bool animate) {
    const bool isSelected = m_selected.get();
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get() : (isSelected ? m_selectedBorderColor.get() : m_unselectedBorderColor.get());

    m_backgroundColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_unselectedColor.get(), duration);
    m_contentColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_selectedColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
    m_contentOpacity.animateTo(isSelected ? 1.0f : 0.0f, duration);
}

void RadioWidget::updateGeometry() {
    const int size = std::min(width(), height());
    if (!m_radiusOverridden) {
        const int autoRadius = size / 2;
        if (m_radius.get() != autoRadius)
            m_radius.set(autoRadius);
    }
    m_contentRadius.set(std::max(0, m_radius.get() - kContentMargin));
}

void RadioWidget::applyStyle(const RadioStyle& value) {
    unselectedColor(value.unselectedColor);
    selectedColor(value.selectedColor);
    disabledColor(value.disabledColor);
    unselectedBorderColor(value.unselectedBorderColor);
    selectedBorderColor(value.selectedBorderColor);
    disabledBorderColor(value.disabledBorderColor);
    if (value.radius >= 0)
        radius(value.radius);
    transition(value.transition);
}

}
