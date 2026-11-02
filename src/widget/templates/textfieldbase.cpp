#include <tavoos/widget/templates/textfieldbase.h>

#include <tavoos/events/key.h>
#include <tavoos/text/utf8.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

TextFieldBase::TextFieldBase(Object* parent) : TextEditBase{parent} {
    m_model.setSingleLine(true);
    m_echoMode.onChange([this](const EchoMode&) {
        m_passwordEditing = false;
        refreshDisplay();
    });
    m_passwordCharacter.onChange([this](const std::string&) { refreshDisplay(); });

    m_placeholderDisplay->alignment(Alignment::CenterVertical)
        .wrapMode(WrapMode::NoWrap)
        .textAlignment(Alignment::Left | Alignment::CenterVertical);
    m_textDisplay->alignment(Alignment::CenterVertical)
        .x(m_contentX)
        .wrapMode(WrapMode::NoWrap)
        .textAlignment(Alignment::Left | Alignment::CenterVertical);
    m_caret->alignment(Alignment::CenterVertical);

    refreshDisplay();
}

void TextFieldBase::triggerFocusOut(Event& event) {
    m_passwordEditing = false;
    refreshDisplay();
    TextEditBase::triggerFocusOut(event);
}

void TextFieldBase::triggerKeyPress(KeyEvent& event) {
    const Key key = static_cast<Key>(event.keyCode());
    if (enabled() && (key == Key::Enter || key == Key::KpEnter)) {
        if (m_acceptableInput.get()) {
            if (m_onSubmit)
                m_onSubmit(m_model.text());
            sendEditingFinished();
        }
        return;
    }
    TextEditBase::triggerKeyPress(event);
}

void TextFieldBase::triggerTextInput(KeyEvent& event) {
    if (enabled() && !readOnly())
        beginPasswordEdit();
    TextEditBase::triggerTextInput(event);
}

void TextFieldBase::setValidator(TextValidator validator) {
    m_model.setValidator(std::move(validator));
    m_acceptableInput.setIfChanged(m_model.validatorState() == ValidatorState::Acceptable);
}

void TextFieldBase::onModelTextChanged() {
    m_acceptableInput.setIfChanged(m_model.validatorState() == ValidatorState::Acceptable);
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
    switch (action) {
    case TextEditAction::InsertNewline:
    case TextEditAction::InsertTab:
    case TextEditAction::MoveUp:
    case TextEditAction::MoveDown:
    case TextEditAction::MovePageUp:
    case TextEditAction::MovePageDown:
    case TextEditAction::SelectUp:
    case TextEditAction::SelectDown:
    case TextEditAction::SelectPageUp:
    case TextEditAction::SelectPageDown:
        return false;
    case TextEditAction::Copy:
    case TextEditAction::Cut:
        if (m_echoMode.get() != EchoMode::Normal)
            return false;
        break;
    default:
        break;
    }
    return TextEditBase::allows(action);
}

void TextFieldBase::applyAction(TextEditAction action) {
    switch (action) {
    case TextEditAction::MoveToLineStart:   action = TextEditAction::MoveToStart; break;
    case TextEditAction::MoveToLineEnd:     action = TextEditAction::MoveToEnd; break;
    case TextEditAction::SelectToLineStart: action = TextEditAction::SelectToStart; break;
    case TextEditAction::SelectToLineEnd:   action = TextEditAction::SelectToEnd; break;
    default: break;
    }
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
    TextEditBase::applyAction(action);
}

void TextFieldBase::beginPasswordEdit() {
    if (m_echoMode.get() != EchoMode::PasswordEchoOnEdit || m_passwordEditing)
        return;
    m_passwordEditing = true;
    m_model.setText({});
}

void TextFieldBase::keepCaretVisible(const Rect& caret) {
    const float visibleWidth = std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight()
                                                 - innerPaddingLeft() - innerPaddingRight());
    const float maxDisplayedCaretX = std::max(0.0f, visibleWidth - static_cast<float>(CaretWidth));

    const float displayedCaretX = caret.x - static_cast<float>(m_scrollOffset);
    if (displayedCaretX < 0.0f)
        m_scrollOffset = static_cast<int>(std::round(caret.x));
    else if (displayedCaretX > maxDisplayedCaretX)
        m_scrollOffset = static_cast<int>(std::round(caret.x - maxDisplayedCaretX));
    const float textEndX = m_textDisplay->positionToRectangle(toDisplay(m_model.text().size())).x;
    const int maxScroll = std::max(0, static_cast<int>(std::round(textEndX + static_cast<float>(CaretWidth) - visibleWidth)));
    m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);

    m_contentX.set(-m_scrollOffset);
}

}
