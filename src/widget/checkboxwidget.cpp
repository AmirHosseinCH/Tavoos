#include <tavoos/widget/checkboxwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/svg.h>

#include <algorithm>
#include <string>

namespace Tavoos {

namespace {

constexpr const char* kCheckIconSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.5 L9.5 17 L19 6.5" /></svg>)svg";
constexpr int kContentMargin = 3;

}

CheckboxWidget::CheckboxWidget(Object* parent) : ButtonBase{parent} {
    m_backgroundColorOut.set(m_uncheckedColor.get());
    m_contentColorOut.set(m_checkedColor.get());
    m_borderColorOut.set(m_uncheckedBorderColor.get());
    m_checkColorOut.set(m_checkColor.get());

    m_checked.onChange([this](const bool& checked) {
        if (m_checkedState.get() != checked)
            m_checkedState.set(checked);
        updateColor(true);
    });
    enabledState().onChange([this](const bool&) { updateColor(true); });

    m_uncheckedColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_uncheckedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_checkColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledCheckColor.onChange([this](const Paint&) { updateColor(false); });

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
        fill.addChild<SVGWidget>([this](SVGWidget& check) {
            check.source(std::string("data:") + kCheckIconSvg).width(m_iconSize).height(m_iconSize)
                .alignment(Alignment::Center).color(m_checkColorOut);
        });
    });

    m_style.onChange([this](const CheckboxStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().checkbox);
}

void CheckboxWidget::applyStyle(const CheckboxStyle& value) {
    uncheckedColor(value.uncheckedColor);
    checkedColor(value.checkedColor);
    disabledColor(value.disabledColor);
    uncheckedBorderColor(value.uncheckedBorderColor);
    checkedBorderColor(value.checkedBorderColor);
    disabledBorderColor(value.disabledBorderColor);
    checkColor(value.checkColor);
    disabledCheckColor(value.disabledCheckColor);
    radius(value.radius);
    transition(value.transition);
}

void CheckboxWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

void CheckboxWidget::handleClick(MouseEvent& event) {
    if (enabled())
        m_checked.set(!m_checked.get());
    ButtonBase::handleClick(event);
}

void CheckboxWidget::updateColor(bool animate) {
    const bool isChecked = m_checked.get();
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get() : (isChecked ? m_checkedBorderColor.get() : m_uncheckedBorderColor.get());

    m_backgroundColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_uncheckedColor.get(), duration);
    m_contentColorOut.animateTo(!isEnabled ? m_disabledColor.get() : m_checkedColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
    m_checkColorOut.animateTo(isEnabled ? m_checkColor.get() : m_disabledCheckColor.get(), duration);
    m_contentOpacity.animateTo(isChecked ? 1.0f : 0.0f, duration);
}

void CheckboxWidget::updateGeometry() {
    const int size = std::min(width(), height());
    m_contentRadius.set(std::max(0, m_radius.get() - kContentMargin));
    m_iconSize.set(std::max(4, static_cast<int>((size - 2 * kContentMargin) * 0.7f)));
}

}
