#include <tavoos/widget/textfieldwidget.h>

#include <tavoos/application.h>
#include <tavoos/events/key.h>
#include <tavoos/text/utf8.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

constexpr int kCaretWidth = 1;
constexpr int kCaretHeight = 18;
constexpr float kBlinkInterval = 0.5f;

std::size_t previousCodepointStart(const std::string& text, std::size_t i) {
    if (i == 0)
        return 0;
    std::size_t j = i - 1;
    while (j > 0 && (static_cast<unsigned char>(text[j]) & 0xC0) == 0x80)
        --j;
    return j;
}

}

TextFieldWidget::TextFieldWidget(Object* parent) : ButtonBase{parent} {
    m_backgroundColorOut.set(m_backgroundColor.get());
    m_borderColorOut.set(m_borderColor.get());

    m_text.onChange([this](const std::string& value) {
        if (m_textState.get() != value)
            m_textState.set(value);
        m_cursorByteIndex = std::min(m_cursorByteIndex, value.size());
        updateDisplayedText();
        updateCaretAndScroll();
    });
    m_placeholder.onChange([this](const std::string&) { updateDisplayedText(); });

    enabledState().onChange([this](const bool&) {
        updateColor(true);
        updateDisplayedText();
    });
    m_backgroundColor.onChange([this](const Paint&) { updateColor(false); });
    m_borderColor.onChange([this](const Paint&) { updateColor(false); });
    m_focusedBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateColor(false); });
    m_textColor.onChange([this](const Paint&) { updateDisplayedText(); });
    m_placeholderColor.onChange([this](const Paint&) { updateDisplayedText(); });

    widthProperty().onChange([this](const int&) { updateCaretAndScroll(); });
    m_innerPaddingLeft.onChange([this](const float&) { updateCaretAndScroll(); });
    m_innerPaddingRight.onChange([this](const float&) { updateCaretAndScroll(); });

    width(200).height(36);

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state())
        .color(m_backgroundColorOut)
            .borderWidth(m_borderWidth.state())
            .borderColor(m_borderColorOut)
            .paddingLeft(m_innerPaddingLeft.state())
            .paddingTop(m_innerPaddingTop.state())
            .paddingRight(m_innerPaddingRight.state())
            .paddingBottom(m_innerPaddingBottom.state());

        box.addChild<RectangleWidget>([this](RectangleWidget& viewport) {
            viewport.fill(Fill::Both).color(Color::Transparent).clip(true);

            viewport.addChild<TextWidget>([this](TextWidget& placeholder) {
                m_placeholderDisplay = &placeholder;
                placeholder.alignment(Alignment::CenterVertical)
                    .wrapMode(WrapMode::NoWrap)
                    .textAlignment(Alignment::Left | Alignment::CenterVertical)
                    .font(m_font.state());
            });

            viewport.addChild<TextWidget>([this](TextWidget& text) {
                m_textDisplay = &text;
                text.alignment(Alignment::CenterVertical)
                    .x(m_contentX)
                    .wrapMode(WrapMode::NoWrap)
                    .textAlignment(Alignment::Left | Alignment::CenterVertical)
                    .font(m_font.state());

                text.addChild<RectangleWidget>([this](RectangleWidget& caret) {
                    caret.alignment(Alignment::CenterVertical)
                    .width(kCaretWidth)
                        .height(kCaretHeight)
                        .x(m_caretX)
                        .color(m_caretColor.state())
                        .visible(m_caretVisible);
                });
            });
        });
    });

    updateDisplayedText();
    updateCaretAndScroll();

    m_style.onChange([this](const TextFieldStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().textField);
}

TextFieldWidget::~TextFieldWidget() {
    if (m_blinking)
        AnimationManager::instance().unregisterAnimation(this);
}

void TextFieldWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

bool TextFieldWidget::hasHandlerFor(EventType type) {
    if (type == EventType::TextInput)
        return true;
    return ButtonBase::hasHandlerFor(type);
}

void TextFieldWidget::triggerPress(MouseEvent& event) {
    if (enabled() && m_textDisplay) {
        m_cursorByteIndex = m_textDisplay->byteIndexForXOffset(event.x() - m_innerPaddingLeft.get() - m_contentX.get());
        updateCaretAndScroll();
    }
    ButtonBase::triggerPress(event);
}

void TextFieldWidget::triggerFocusIn(Event& event) {
    ButtonBase::triggerFocusIn(event);
    updateColor(true);
    resetCaretBlink();
    if (!m_blinking) {
        m_blinking = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

void TextFieldWidget::triggerFocusOut(Event& event) {
    if (m_blinking) {
        m_blinking = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
    m_caretVisible.set(false);
    ButtonBase::triggerFocusOut(event);
    updateColor(true);
}

void TextFieldWidget::triggerKeyPress(KeyEvent& event) {
    if (!enabled())
        return;

    const int key = event.keyCode();
    if (key == static_cast<int>(Key::Backspace)) {
        deleteBackward();
    } else if (key == static_cast<int>(Key::Delete)) {
        deleteForward();
    } else if (key == static_cast<int>(Key::Left)) {
        moveCursorLeft();
    } else if (key == static_cast<int>(Key::Right)) {
        moveCursorRight();
    } else if (key == static_cast<int>(Key::Home)) {
        m_cursorByteIndex = 0;
        updateCaretAndScroll();
    } else if (key == static_cast<int>(Key::End)) {
        m_cursorByteIndex = m_text.get().size();
        updateCaretAndScroll();
    } else if (key == static_cast<int>(Key::Enter) || key == static_cast<int>(Key::KpEnter)) {
        if (m_onSubmit)
            m_onSubmit(m_text.get());
    } else {
        event.ignore();
    }
}

void TextFieldWidget::triggerKeyRelease(KeyEvent&) {
}

void TextFieldWidget::triggerTextInput(KeyEvent& event) {
    if (!enabled())
        return;
    insertText(encodeUtf8(static_cast<char32_t>(event.keyCode())));
}

bool TextFieldWidget::tick(float dt) {
    m_blinkElapsed += dt;
    if (m_blinkElapsed >= kBlinkInterval) {
        m_blinkElapsed -= kBlinkInterval;
        m_caretVisible.set(!m_caretVisible.get());
    }
    return true;
}

void TextFieldWidget::resetCaretBlink() {
    m_blinkElapsed = 0.0f;
    m_caretVisible.set(true);
}

void TextFieldWidget::insertText(const std::string& utf8) {
    std::string value = m_text.get();
    value.insert(m_cursorByteIndex, utf8);
    m_cursorByteIndex += utf8.size();
    m_text.set(value);
    resetCaretBlink();
}

void TextFieldWidget::deleteBackward() {
    if (m_cursorByteIndex == 0)
        return;
    const std::size_t start = previousCodepointStart(m_text.get(), m_cursorByteIndex);
    std::string value = m_text.get();
    value.erase(start, m_cursorByteIndex - start);
    m_cursorByteIndex = start;
    m_text.set(value);
    resetCaretBlink();
}

void TextFieldWidget::deleteForward() {
    const std::string& value = m_text.get();
    if (m_cursorByteIndex >= value.size())
        return;
    const Utf8Decoded d = decodeUtf8At(value, m_cursorByteIndex);
    std::string next = value;
    next.erase(m_cursorByteIndex, d.length);
    m_text.set(next);
    resetCaretBlink();
}

void TextFieldWidget::moveCursorLeft() {
    if (m_cursorByteIndex == 0)
        return;
    m_cursorByteIndex = previousCodepointStart(m_text.get(), m_cursorByteIndex);
    updateCaretAndScroll();
    resetCaretBlink();
}

void TextFieldWidget::moveCursorRight() {
    const std::string& value = m_text.get();
    if (m_cursorByteIndex >= value.size())
        return;
    const Utf8Decoded d = decodeUtf8At(value, m_cursorByteIndex);
    m_cursorByteIndex += d.length;
    updateCaretAndScroll();
    resetCaretBlink();
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

void TextFieldWidget::updateDisplayedText() {
    if (!m_textDisplay)
        return;
    const std::string& value = m_text.get();
    m_textDisplay->text(value);
    m_textDisplay->color(enabled() ? m_textColor.get() : m_disabledColor.get());

    if (m_placeholderDisplay) {
        m_placeholderDisplay->text(m_placeholder.get());
        m_placeholderDisplay->color(enabled() ? m_placeholderColor.get() : m_disabledColor.get());
        m_placeholderDisplay->visible(value.empty());
    }
}

void TextFieldWidget::updateCaretAndScroll() {
    if (!m_textDisplay)
        return;

    const float caretLocalX = m_textDisplay->xOffsetForByteIndex(m_cursorByteIndex);
    const float visibleWidth = std::max(0.0f, displayedWidth() - m_innerPaddingLeft.get() - m_innerPaddingRight.get());
    const float maxDisplayedCaretX = std::max(0.0f, visibleWidth - static_cast<float>(kCaretWidth));

    const float displayedCaretX = caretLocalX - static_cast<float>(m_scrollOffset);
    if (displayedCaretX < 0.0f)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX));
    else if (displayedCaretX > maxDisplayedCaretX)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX - maxDisplayedCaretX));
    m_scrollOffset = std::max(0, m_scrollOffset);

    m_contentX.set(-m_scrollOffset);
    m_caretX.set(static_cast<int>(std::round(caretLocalX)));
}

void TextFieldWidget::applyStyle(const TextFieldStyle& value) {
    backgroundColor(value.backgroundColor);
    borderColor(value.borderColor);
    focusedBorderColor(value.focusedBorderColor);
    disabledColor(value.disabledColor);
    disabledBorderColor(value.disabledBorderColor);
    textColor(value.textColor);
    placeholderColor(value.placeholderColor);
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
