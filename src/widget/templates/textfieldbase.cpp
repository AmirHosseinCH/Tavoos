#include <tavoos/widget/templates/textfieldbase.h>

#include <tavoos/events/key.h>
#include <tavoos/text/texteditactions.h>
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
        updateSelection();
    });
    m_model.onCursorChanged([this] {
        updateCaretAndScroll();
        updateSelection();
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
                .font(m_font.state())
                .selectionColor(m_selectionColor.state())
                .selectedTextColor(m_selectedTextColor.state());

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
    m_model.deselect();
    Widget::triggerFocusOut(event);
}

void TextFieldBase::triggerKeyPress(KeyEvent& event) {
    if (!enabled())
        return;

    const Key key = static_cast<Key>(event.keyCode());
    if (const auto action = textEditActionFor(key, event.modifiers())) {
        applyTextEditAction(m_model, *action);
        resetCaretBlink();
    } else if (key == Key::Enter || key == Key::KpEnter) {
        if (m_onSubmit)
            m_onSubmit(m_model.text());
    } else {
        event.ignore();
    }
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

int TextFieldBase::cursorPosition() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.cursor()));
}

int TextFieldBase::selectionStart() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.selectionStart()));
}

int TextFieldBase::selectionEnd() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.selectionEnd()));
}

void TextFieldBase::select(int start, int end) {
    m_model.select(byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(start, 0))),
                   byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(end, 0))));
}

void TextFieldBase::selectAll() {
    m_model.selectAll();
}

void TextFieldBase::deselect() {
    m_model.deselect();
}

void TextFieldBase::moveCursorTo(int position) {
    m_model.setCursor(byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(position, 0))));
}

void TextFieldBase::updateSelection() {
    if (m_textDisplay)
        m_textDisplay->selection(m_model.selectionStart(), m_model.selectionEnd());
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
