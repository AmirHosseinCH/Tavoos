#include <tavoos/widget/templates/textfieldbase.h>

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

TextFieldBase::TextFieldBase(Object* parent) : Control{parent} {
    m_text.onChange([this](const std::string& value) {
        m_textState.setIfChanged(value);
        m_cursorByteIndex = std::min(m_cursorByteIndex, value.size());
        updateDisplayedText();
        updateCaretAndScroll();
    });
    m_placeholder.onChange([this](const std::string&) { updateDisplayedText(); });
    m_textColor.onChange([this](const Paint&) { updateDisplayedText(); });
    m_placeholderColor.onChange([this](const Paint&) { updateDisplayedText(); });
    m_disabledTextColor.onChange([this](const Paint&) { updateDisplayedText(); });
    enabledState().onChange([this](const bool& enabled) {
        focusable(enabled);
        updateDisplayedText();
    });
    focusable(true);

    widthProperty().onChange([this](const int&) { updateCaretAndScroll(); });
    m_innerPaddingLeft.onChange([this](const float&) { updateCaretAndScroll(); });
    m_innerPaddingRight.onChange([this](const float&) { updateCaretAndScroll(); });

    content<RectangleWidget>([this](RectangleWidget& viewport) {
        m_viewport = &viewport;
        viewport.fill(Fill::Both)
            .marginLeft(m_innerPaddingLeft.state())
            .marginTop(m_innerPaddingTop.state())
            .marginRight(m_innerPaddingRight.state())
            .marginBottom(m_innerPaddingBottom.state())
            .color(Color::Transparent)
            .clip(true);

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

    updateDisplayedText();
    updateCaretAndScroll();
}

TextFieldBase::~TextFieldBase() {
    if (m_blinking)
        AnimationManager::instance().unregisterAnimation(this);
}

bool TextFieldBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::MouseClick:
    case EventType::KeyPress:
    case EventType::KeyRelease:
    case EventType::TextInput:
        return true;
    default:
        return Control::hasHandlerFor(type);
    }
}

void TextFieldBase::triggerPress(MouseEvent& event) {
    if (enabled() && m_textDisplay) {
        const Point local = m_textDisplay->mapFromWindow(mapToWindow({event.x(), event.y()}));
        m_cursorByteIndex = m_textDisplay->byteIndexForXOffset(local.x);
        updateCaretAndScroll();
        resetCaretBlink();
    }
    Widget::triggerPress(event);
}

void TextFieldBase::triggerFocusIn(Event& event) {
    Widget::triggerFocusIn(event);
    resetCaretBlink();
    if (!m_blinking) {
        m_blinking = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

void TextFieldBase::triggerFocusOut(Event& event) {
    if (m_blinking) {
        m_blinking = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
    m_caretVisible.set(false);
    Widget::triggerFocusOut(event);
}

void TextFieldBase::triggerKeyPress(KeyEvent& event) {
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

void TextFieldBase::triggerKeyRelease(KeyEvent&) {
}

void TextFieldBase::triggerTextInput(KeyEvent& event) {
    if (!enabled())
        return;
    insertText(encodeUtf8(static_cast<char32_t>(event.keyCode())));
}

bool TextFieldBase::tick(float dt) {
    m_blinkElapsed += dt;
    if (m_blinkElapsed >= kBlinkInterval) {
        m_blinkElapsed -= kBlinkInterval;
        m_caretVisible.set(!m_caretVisible.get());
    }
    return true;
}

void TextFieldBase::resetCaretBlink() {
    m_blinkElapsed = 0.0f;
    if (m_blinking)
        m_caretVisible.set(true);
}

void TextFieldBase::insertText(const std::string& utf8) {
    std::string value = m_text.get();
    value.insert(m_cursorByteIndex, utf8);
    m_cursorByteIndex += utf8.size();
    m_text.set(value);
    resetCaretBlink();
}

void TextFieldBase::deleteBackward() {
    if (m_cursorByteIndex == 0)
        return;
    const std::size_t start = previousCodepointStart(m_text.get(), m_cursorByteIndex);
    std::string value = m_text.get();
    value.erase(start, m_cursorByteIndex - start);
    m_cursorByteIndex = start;
    m_text.set(value);
    resetCaretBlink();
}

void TextFieldBase::deleteForward() {
    const std::string& value = m_text.get();
    if (m_cursorByteIndex >= value.size())
        return;
    const Utf8Decoded d = decodeUtf8At(value, m_cursorByteIndex);
    std::string next = value;
    next.erase(m_cursorByteIndex, d.length);
    m_text.set(next);
    resetCaretBlink();
}

void TextFieldBase::moveCursorLeft() {
    if (m_cursorByteIndex == 0)
        return;
    m_cursorByteIndex = previousCodepointStart(m_text.get(), m_cursorByteIndex);
    updateCaretAndScroll();
    resetCaretBlink();
}

void TextFieldBase::moveCursorRight() {
    const std::string& value = m_text.get();
    if (m_cursorByteIndex >= value.size())
        return;
    const Utf8Decoded d = decodeUtf8At(value, m_cursorByteIndex);
    m_cursorByteIndex += d.length;
    updateCaretAndScroll();
    resetCaretBlink();
}

void TextFieldBase::updateDisplayedText() {
    if (!m_textDisplay)
        return;
    const std::string& value = m_text.get();
    const bool isEnabled = enabled();
    m_textDisplay->text(value);
    m_textDisplay->color(isEnabled ? m_textColor.get() : m_disabledTextColor.get());

    if (m_placeholderDisplay) {
        m_placeholderDisplay->text(m_placeholder.get());
        m_placeholderDisplay->color(isEnabled ? m_placeholderColor.get() : m_disabledTextColor.get());
        m_placeholderDisplay->visible(value.empty());
    }
}

void TextFieldBase::updateCaretAndScroll() {
    if (!m_textDisplay || !m_viewport)
        return;

    const float caretLocalX = m_textDisplay->xOffsetForByteIndex(m_cursorByteIndex);
    const float visibleWidth = std::max(0.0f, m_viewport->displayedWidth());
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

}
