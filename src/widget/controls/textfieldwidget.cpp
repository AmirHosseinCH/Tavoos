#include <tavoos/widget/controls/textfieldwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

TextFieldWidget::TextFieldWidget(Object* parent) : TextFieldBase{parent} {
    m_backgroundColorOut.set(m_backgroundColor.get());
    m_borderColorOut.set(m_borderColor.get());

    enabledState().onChange([this](const bool&) { updateColor(true); });
    focusedState().onChange([this](const bool&) { updateColor(true); });
    m_backgroundColor.onChange([this](const Paint&) { updateColor(false); });
    m_borderColor.onChange([this](const Paint&) { updateColor(false); });
    m_focusedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });

    width(200).height(36);

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
            .color(m_backgroundColorOut)
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColorOut);
    });

    m_style.onChange([this](const TextFieldStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().textField);
}

void TextFieldWidget::render(Renderer& renderer) {
    m_settled = true;
    TextFieldBase::render(renderer);
}

void TextFieldWidget::updateColor(bool animate) {
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get()
                          : focused() ? m_focusedBorderColor.get()
                                      : m_borderColor.get();

    m_backgroundColorOut.animateTo(isEnabled ? m_backgroundColor.get() : m_disabledColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
}

void TextFieldWidget::applyStyle(const TextFieldStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    focusedBorderColor(value.focusedBorderColor);
    disabledColor(value.disabledColor);
    disabledBorderColor(value.disabledBorderColor);
    textColor(value.textColor);
    placeholderColor(value.placeholderColor);
    disabledTextColor(value.disabledColor);
    caretColor(value.caretColor);
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
