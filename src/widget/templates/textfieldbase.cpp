#include <tavoos/widget/templates/textfieldbase.h>

#include <tavoos/events/key.h>
#include <tavoos/text/utf8.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

constexpr int kCaretWidth = 1;
constexpr float kBlinkInterval = 0.5f;

}

TextFieldBase::TextFieldBase(Object* parent) : Control{parent} {
    cursor(CursorShape::IBeam);
    m_text.onChange([this](const std::string& value) {
        m_textState.setIfChanged(value);
        m_model.setText(value);
    });
    m_model.onTextChanged([this] {
        if (m_model.text() != m_text.get())
            m_text.set(m_model.text());
        updateDisplayedText();
        updateCaretAndScroll();
    });
    m_model.onCursorChanged([this] { updateCaretAndScroll(); });
    m_placeholder.onChange([this](const std::string&) { updateDisplayedText(); });
    m_textColor.onChange([this](const Paint&) { updateDisplayedText(); });
    m_placeholderColor.onChange([this](const Paint&) { updateDisplayedText(); });
    m_disabledTextColor.onChange([this](const Paint&) { updateDisplayedText(); });
    enabledState().onChange([this](const bool& enabled) {
        focusable(enabled);
        updateDisplayedText();
    });
    focusable(true);

    m_innerPaddingLeft.onChange([this](const float&) { updateCaretAndScroll(); });
    m_innerPaddingRight.onChange([this](const float&) { updateCaretAndScroll(); });

    content<RectangleWidget>([this](RectangleWidget& viewport) {
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
                    .height(m_caretHeight)
                    .x(m_caretX)
                    .color(m_caretColor.state())
                    .visible(m_caretVisible);
            });
        });
    });

    m_font.onChange([this](const Font&) { updateCaretAndScroll(); });

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
        m_model.setCursor(m_textDisplay->byteIndexForXOffset(local.x));
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
        m_model.deleteBackward();
    } else if (key == static_cast<int>(Key::Delete)) {
        m_model.deleteForward();
    } else if (key == static_cast<int>(Key::Left)) {
        m_model.moveLeft();
    } else if (key == static_cast<int>(Key::Right)) {
        m_model.moveRight();
    } else if (key == static_cast<int>(Key::Home)) {
        m_model.moveToStart();
    } else if (key == static_cast<int>(Key::End)) {
        m_model.moveToEnd();
    } else if (key == static_cast<int>(Key::Enter) || key == static_cast<int>(Key::KpEnter)) {
        if (m_onSubmit)
            m_onSubmit(m_model.text());
        return;
    } else {
        event.ignore();
        return;
    }
    resetCaretBlink();
}

void TextFieldBase::triggerKeyRelease(KeyEvent&) {
}

void TextFieldBase::triggerTextInput(KeyEvent& event) {
    if (!enabled())
        return;
    m_model.insert(encodeUtf8(static_cast<char32_t>(event.keyCode())));
    resetCaretBlink();
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

void TextFieldBase::updateDisplayedText() {
    if (!m_textDisplay)
        return;
    const std::string& value = m_model.text();
    const bool isEnabled = enabled();
    m_textDisplay->text(value);
    m_textDisplay->color(isEnabled ? m_textColor.get() : m_disabledTextColor.get());

    if (m_placeholderDisplay) {
        m_placeholderDisplay->text(m_placeholder.get());
        m_placeholderDisplay->color(isEnabled ? m_placeholderColor.get() : m_disabledTextColor.get());
        m_placeholderDisplay->visible(value.empty());
    }
}

void TextFieldBase::onResolvedSizeChanged() {
    updateCaretAndScroll();
}

void TextFieldBase::updateCaretAndScroll() {
    if (!m_textDisplay)
        return;

    const float caretLocalX = m_textDisplay->xOffsetForByteIndex(m_model.cursor());
    const float visibleWidth = std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight()
                                                 - m_innerPaddingLeft.get() - m_innerPaddingRight.get());
    const float maxDisplayedCaretX = std::max(0.0f, visibleWidth - static_cast<float>(kCaretWidth));

    const float displayedCaretX = caretLocalX - static_cast<float>(m_scrollOffset);
    if (displayedCaretX < 0.0f)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX));
    else if (displayedCaretX > maxDisplayedCaretX)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX - maxDisplayedCaretX));
    const float textEndX = m_textDisplay->xOffsetForByteIndex(m_model.text().size());
    const int maxScroll = std::max(0, static_cast<int>(std::round(textEndX + static_cast<float>(kCaretWidth) - visibleWidth)));
    m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);

    m_contentX.set(-m_scrollOffset);
    m_caretX.set(static_cast<int>(std::round(caretLocalX)));
    m_caretHeight.set(static_cast<int>(std::round(m_textDisplay->lineHeight())));
}

}
