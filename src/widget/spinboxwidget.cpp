#include <tavoos/widget/spinboxwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

namespace {

constexpr int kStepperWidth = 30;
constexpr int kStepperPadding = 5;
constexpr int kStepperButtonHeight = 14;

constexpr const char* kUpChevronSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 15 L12 9 L18 15" /></svg>)svg";
constexpr const char* kDownChevronSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9 L12 15 L18 9" /></svg>)svg";

}

SpinBoxWidget::SpinBoxWidget(Object* parent) : SpinBoxBase{parent} {
    width(140).height(40);

    m_backgroundColorOut.set(m_backgroundColor.get());
    m_borderColorOut.set(m_borderColor.get());

    enabledState().onChange([this](const bool&) {
        updateEnabled();
        updateColor(true);
    });

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

    content<TextFieldWidget>([this](TextFieldWidget& field) {
        m_field = &field;
        field.fill(Fill::Both).marginRight(kStepperWidth + 4);
        field.onSubmit([this](const std::string& text) { commitText(text); });
        field.focusedState().onChange([this](const bool&) { updateColor(true); });
    });

    up<ButtonWidget>([this](ButtonWidget& button) {
        m_upButton = &button;
        button.display(ButtonDisplay::IconOnly)
            .icon(std::string("data:") + kUpChevronSvg)
            .iconSize(12)
            .alignment(Alignment::Right | Alignment::Top)
            .marginRight(kStepperPadding)
            .marginTop(kStepperPadding)
            .width(kStepperWidth - 2 * kStepperPadding)
            .height(kStepperButtonHeight);
    });

    down<ButtonWidget>([this](ButtonWidget& button) {
        m_downButton = &button;
        button.display(ButtonDisplay::IconOnly)
            .icon(std::string("data:") + kDownChevronSvg)
            .iconSize(12)
            .alignment(Alignment::Right | Alignment::Bottom)
            .marginRight(kStepperPadding)
            .marginBottom(kStepperPadding)
            .width(kStepperWidth - 2 * kStepperPadding)
            .height(kStepperButtonHeight);
    });

    valueTextState().onChange([this](const std::string& text) { m_field->text(text); });
    m_field->text(valueText());

    m_style.onChange([this](const SpinBoxStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().spinBox);
}

void SpinBoxWidget::render(Renderer& renderer) {
    m_settled = true;
    SpinBoxBase::render(renderer);
}

void SpinBoxWidget::updateEnabled() {
    const bool value = enabled();
    if (m_field)
        m_field->enabled(value);
    if (m_upButton)
        m_upButton->enabled(value);
    if (m_downButton)
        m_downButton->enabled(value);
}

void SpinBoxWidget::updateColor(bool animate) {
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;

    const Paint& border = !isEnabled ? m_disabledBorderColor.get()
                         : (m_field && m_field->focused()) ? m_focusedBorderColor.get()
                                                             : m_borderColor.get();

    m_backgroundColorOut.animateTo(isEnabled ? m_backgroundColor.get() : m_disabledColor.get(), duration);
    m_borderColorOut.animateTo(border, duration);
}

void SpinBoxWidget::applyStyle(const SpinBoxStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    focusedBorderColor(value.focusedBorderColor);
    disabledColor(value.disabledColor);
    disabledBorderColor(value.disabledBorderColor);
    radius(value.radius);
    borderWidth(value.borderWidth);
    transition(value.transition);

    if (m_field) {
        m_field->style(value.field);
        m_field->backgroundColor(Color::Transparent);
        m_field->disabledColor(Color::Transparent);
        m_field->borderWidth(0.0f);
    }
    if (m_upButton)
        m_upButton->style(value.stepperButton);
    if (m_downButton)
        m_downButton->style(value.stepperButton);
}

}
