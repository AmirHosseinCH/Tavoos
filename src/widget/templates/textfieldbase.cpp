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
    m_model.setSingleLine(true);
    m_text.onChange([this](const std::string& value) {
        m_changeFromCode = true;
        m_model.setText(value);
        m_changeFromCode = false;
    });
    m_maximumLength.onChange([this](const int& length) {
        m_changeFromCode = true;
        m_model.setMaximumLength(static_cast<std::size_t>(std::max(length, 0)));
        m_changeFromCode = false;
    });
    m_model.onTextChanged([this] {
        if (!m_changeFromCode && m_model.text() != m_text.get())
            m_text.set(m_model.text());
        m_textState.setIfChanged(m_model.text());
        refreshDisplay();
        m_canUndo.setIfChanged(m_model.canUndo());
        m_canRedo.setIfChanged(m_model.canRedo());
        m_acceptableInput.setIfChanged(m_model.validatorState() == ValidatorState::Acceptable);
        if (!m_changeFromCode && m_onTextEdited)
            m_onTextEdited(m_model.text());
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
    m_readOnly.onChange([this](const bool& value) {
        m_model.setReadOnly(value);
        resetCaretBlink();
    });
    m_echoMode.onChange([this](const EchoMode&) {
        m_passwordEditing = false;
        refreshDisplay();
    });
    m_passwordCharacter.onChange([this](const std::string&) { refreshDisplay(); });

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
    case EventType::ShortcutOverride:
    case EventType::TextInput:
        return true;
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
        if (m_selectByMouse.get() && enabled())
            return true;
        return Control::hasHandlerFor(type);
    default:
        return Control::hasHandlerFor(type);
    }
}

void TextFieldBase::triggerPress(MouseEvent& event) {
    if (enabled()) {
        m_pressPoint = mapToWindow({event.x(), event.y()});
        m_pressClickCount = m_selectByMouse.get() ? event.clickCount() : 1;
        if (m_pressClickCount == 2 && m_echoMode.get() != EchoMode::Normal)
            m_pressClickCount = 3;
        if (m_pressClickCount >= 3) {
            m_model.selectAll();
            resetCaretBlink();
        } else if (m_pressClickCount == 2) {
            m_pressWord = m_model.wordAt(characterAtPoint(m_pressPoint));
            m_model.select(m_pressWord.first, m_pressWord.second);
            resetCaretBlink();
        } else {
            moveCursorToPoint(m_pressPoint, m_selectByMouse.get() && hasFlag(event.modifiers(), KeyModifier::Shift));
        }
    }
    Widget::triggerPress(event);
}

void TextFieldBase::triggerDragMove(DragEvent& event) {
    if (m_selectByMouse.get() && enabled()) {
        const Point point{m_pressPoint.x + event.totalDx(), m_pressPoint.y + event.totalDy()};
        if (m_pressClickCount == 2)
            extendWordSelection(point);
        else if (m_pressClickCount == 1)
            moveCursorToPoint(point, true);
    }
    Widget::triggerDragMove(event);
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
    m_passwordEditing = false;
    refreshDisplay();
    if (m_onEditingFinished && m_acceptableInput.get())
        m_onEditingFinished(m_model.text());
    Widget::triggerFocusOut(event);
}

void TextFieldBase::triggerKeyPress(KeyEvent& event) {
    if (!enabled())
        return;

    const Key key = static_cast<Key>(event.keyCode());
    const auto action = textEditActionFor(key, event.modifiers());
    if (action && allows(*action)) {
        applyAction(*action);
        resetCaretBlink();
    } else if (key == Key::Enter || key == Key::KpEnter) {
        if (m_acceptableInput.get()) {
            if (m_onSubmit)
                m_onSubmit(m_model.text());
            if (m_onEditingFinished)
                m_onEditingFinished(m_model.text());
        }
    } else {
        event.ignore();
    }
}

void TextFieldBase::triggerKeyRelease(KeyEvent&) {
}

void TextFieldBase::triggerShortcutOverride(KeyEvent& event) {
    const Key key = static_cast<Key>(event.keyCode());
    const auto action = textEditActionFor(key, event.modifiers());
    if (enabled() && ((action && allows(*action)) || (!m_readOnly.get() && isTextInputKey(key, event.modifiers()))))
        event.accept();
    Widget::triggerShortcutOverride(event);
}

void TextFieldBase::triggerTextInput(KeyEvent& event) {
    if (!enabled() || m_readOnly.get())
        return;
    beginPasswordEdit();
    m_model.type(encodeUtf8(static_cast<char32_t>(event.keyCode())));
    resetCaretBlink();
}

bool TextFieldBase::tick(float dt) {
    m_blinkElapsed += dt;
    if (m_blinkElapsed >= kBlinkInterval) {
        m_blinkElapsed -= kBlinkInterval;
        m_caretVisible.set(!m_readOnly.get() && !m_caretVisible.get());
    }
    return true;
}

void TextFieldBase::resetCaretBlink() {
    m_blinkElapsed = 0.0f;
    if (m_blinking)
        m_caretVisible.set(!m_readOnly.get());
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

void TextFieldBase::undo() {
    m_model.undo();
}

void TextFieldBase::redo() {
    m_model.redo();
}

void TextFieldBase::moveCursorTo(int position) {
    m_model.setCursor(byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(position, 0))));
}

void TextFieldBase::setValidator(TextValidator validator) {
    m_model.setValidator(std::move(validator));
    m_acceptableInput.setIfChanged(m_model.validatorState() == ValidatorState::Acceptable);
}

void TextFieldBase::moveCursorToPoint(const Point& windowPoint, bool extend) {
    if (!m_textDisplay)
        return;
    const Point local = m_textDisplay->mapFromWindow(windowPoint);
    m_model.setCursor(toModel(m_textDisplay->byteIndexForXOffset(local.x)), extend);
    resetCaretBlink();
}

void TextFieldBase::extendWordSelection(const Point& windowPoint) {
    const auto word = m_model.wordAt(characterAtPoint(windowPoint));
    if (word.first < m_pressWord.first)
        m_model.select(m_pressWord.second, word.first);
    else
        m_model.select(m_pressWord.first, std::max(word.second, m_pressWord.second));
    resetCaretBlink();
}

std::size_t TextFieldBase::characterAtPoint(const Point& windowPoint) {
    if (!m_textDisplay)
        return 0;
    const float x = m_textDisplay->mapFromWindow(windowPoint).x;
    const std::size_t index = m_textDisplay->byteIndexForXOffset(x);
    if (index > 0 && m_textDisplay->xOffsetForByteIndex(index) > x)
        return toModel(previousCodepointStart(m_textDisplay->text(), index));
    return toModel(index);
}

std::string TextFieldBase::displayText() const {
    switch (shown()) {
    case Shown::Plain:
        return m_model.text();
    case Shown::Hidden:
        return {};
    case Shown::Masked:
        break;
    }
    const std::string character = mask();
    const std::size_t count = characterCount(m_model.text());
    std::string result;
    result.reserve(count * character.size());
    for (std::size_t i = 0; i < count; ++i)
        result += character;
    return result;
}

TextFieldBase::Shown TextFieldBase::shown() const {
    switch (m_echoMode.get()) {
    case EchoMode::Normal:
        return Shown::Plain;
    case EchoMode::NoEcho:
        return Shown::Hidden;
    case EchoMode::PasswordEchoOnEdit:
        return m_passwordEditing ? Shown::Plain : Shown::Masked;
    case EchoMode::Password:
        break;
    }
    return Shown::Masked;
}

std::string TextFieldBase::mask() const {
    const std::string& character = m_passwordCharacter.get();
    if (character.empty())
        return "\xE2\x80\xA2";
    return character.substr(0, decodeUtf8At(character, 0).length);
}

std::size_t TextFieldBase::toDisplay(std::size_t position) const {
    switch (shown()) {
    case Shown::Plain:
        return position;
    case Shown::Hidden:
        return 0;
    case Shown::Masked:
        break;
    }
    return characterIndexAt(m_model.text(), position) * mask().size();
}

std::size_t TextFieldBase::toModel(std::size_t position) const {
    switch (shown()) {
    case Shown::Plain:
        return position;
    case Shown::Hidden:
        return m_model.cursor();
    case Shown::Masked:
        break;
    }
    return byteOffsetOfCharacter(m_model.text(), position / mask().size());
}

bool TextFieldBase::allows(TextEditAction action) const {
    if (m_readOnly.get() && changesText(action))
        return false;
    if (m_echoMode.get() != EchoMode::Normal && (action == TextEditAction::Copy || action == TextEditAction::Cut))
        return false;
    return true;
}

void TextFieldBase::applyAction(TextEditAction action) {
    if (m_echoMode.get() != EchoMode::Normal) {
        switch (action) {
        case TextEditAction::MoveWordLeft:    action = TextEditAction::MoveToStart; break;
        case TextEditAction::MoveWordRight:   action = TextEditAction::MoveToEnd; break;
        case TextEditAction::SelectWordLeft:  action = TextEditAction::SelectToStart; break;
        case TextEditAction::SelectWordRight: action = TextEditAction::SelectToEnd; break;
        case TextEditAction::DeleteWordBackward:
            m_model.moveToStart(true);
            m_model.deleteBackward();
            return;
        case TextEditAction::DeleteWordForward:
            m_model.moveToEnd(true);
            m_model.deleteForward();
            return;
        case TextEditAction::Paste:
            beginPasswordEdit();
            break;
        default:
            break;
        }
    }
    applyTextEditAction(m_model, action);
}

void TextFieldBase::beginPasswordEdit() {
    if (m_echoMode.get() != EchoMode::PasswordEchoOnEdit || m_passwordEditing)
        return;
    m_passwordEditing = true;
    m_model.setText({});
}

void TextFieldBase::refreshDisplay() {
    updateDisplayedText();
    updateCaretAndScroll();
    updateSelection();
}

void TextFieldBase::updateSelection() {
    if (m_textDisplay)
        m_textDisplay->selection(toDisplay(m_model.selectionStart()), toDisplay(m_model.selectionEnd()));
}

void TextFieldBase::updateDisplayedText() {
    if (!m_textDisplay)
        return;
    const std::string& value = m_model.text();
    const bool isEnabled = enabled();
    m_textDisplay->text(displayText());
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

    const float caretLocalX = m_textDisplay->xOffsetForByteIndex(toDisplay(m_model.cursor()));
    const float visibleWidth = std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight()
                                                 - m_innerPaddingLeft.get() - m_innerPaddingRight.get());
    const float maxDisplayedCaretX = std::max(0.0f, visibleWidth - static_cast<float>(kCaretWidth));

    const float displayedCaretX = caretLocalX - static_cast<float>(m_scrollOffset);
    if (displayedCaretX < 0.0f)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX));
    else if (displayedCaretX > maxDisplayedCaretX)
        m_scrollOffset = static_cast<int>(std::round(caretLocalX - maxDisplayedCaretX));
    const float textEndX = m_textDisplay->xOffsetForByteIndex(toDisplay(m_model.text().size()));
    const int maxScroll = std::max(0, static_cast<int>(std::round(textEndX + static_cast<float>(kCaretWidth) - visibleWidth)));
    m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);

    m_contentX.set(-m_scrollOffset);
    m_caretX.set(static_cast<int>(std::round(caretLocalX)));
    m_caretHeight.set(static_cast<int>(std::round(m_textDisplay->lineHeight())));
}

}
