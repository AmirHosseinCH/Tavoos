#include <tavoos/widget/controls/textareawidget.h>

#include <tavoos/application.h>

#include <algorithm>

namespace Tavoos {

namespace {

constexpr float kMinimumWidth = 200.0f;

}

TextAreaWidget::TextAreaWidget(Object* parent) : TextAreaBase{parent} {
    m_backgroundColorOut.set(m_backgroundColor.get());
    m_borderColorOut.set(m_borderColor.get());

    enabledState().onChange([this](const bool&) { updateColor(true); });
    focusedState().onChange([this](const bool&) { updateColor(true); });
    m_backgroundColor.onChange([this](const Paint&) { updateColor(false); });
    m_borderColor.onChange([this](const Paint&) { updateColor(false); });
    m_focusedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
            .color(m_backgroundColorOut)
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColorOut);
    });

    m_style.onChange([this](const TextAreaStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().textArea);
}

void TextAreaWidget::render(Renderer& renderer) {
    m_settled = true;
    TextAreaBase::render(renderer);
}

Widget::Size TextAreaWidget::computeIntrinsicSize() {
    Size size = TextAreaBase::computeIntrinsicSize();
    if (width() <= 0)
        size.width = std::max(size.width, kMinimumWidth);
    return size;
}

void TextAreaWidget::updateColor(bool animate) {
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get()
                          : focused() ? m_focusedBorderColor.get()
                                      : m_borderColor.get();

    m_backgroundColorOut.animateTo(isEnabled ? m_backgroundColor.get() : m_disabledColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
}

void TextAreaWidget::applyStyle(const TextAreaStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    focusedBorderColor(value.focusedBorderColor);
    disabledColor(value.disabledColor);
    disabledBorderColor(value.disabledBorderColor);
    textColor(value.textColor);
    placeholderColor(value.placeholderColor);
    disabledTextColor(value.disabledTextColor);
    caretColor(value.caretColor);
    selectionColor(value.selectionColor);
    selectedTextColor(value.selectedTextColor);
    radius(value.radius);
    borderWidth(value.borderWidth);
    innerPaddingLeft(value.innerPaddingLeft);
    innerPaddingTop(value.innerPaddingTop);
    innerPaddingRight(value.innerPaddingRight);
    innerPaddingBottom(value.innerPaddingBottom);
    font(value.font);
    transition(value.transition);
}

}
