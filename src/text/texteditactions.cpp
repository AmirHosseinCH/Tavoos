#include <tavoos/text/texteditactions.h>

#include <tavoos/clipboard.h>

namespace Tavoos {

std::optional<TextEditAction> textEditActionFor(Key key, KeyModifier modifiers) {
    const bool plain = modifiers == KeyModifier::None;
    const bool shift = modifiers == KeyModifier::Shift;
    const bool control = modifiers == KeyModifier::Control;
    const bool controlShift = modifiers == (KeyModifier::Control | KeyModifier::Shift);

    switch (key) {
    case Key::Backspace:
        if (plain || shift)
            return TextEditAction::DeleteBackward;
        if (control)
            return TextEditAction::DeleteWordBackward;
        break;
    case Key::Delete:
        if (plain)
            return TextEditAction::DeleteForward;
        if (control)
            return TextEditAction::DeleteWordForward;
        break;
    case Key::Left:
        if (plain)
            return TextEditAction::MoveLeft;
        if (shift)
            return TextEditAction::SelectLeft;
        if (control)
            return TextEditAction::MoveWordLeft;
        if (controlShift)
            return TextEditAction::SelectWordLeft;
        break;
    case Key::Right:
        if (plain)
            return TextEditAction::MoveRight;
        if (shift)
            return TextEditAction::SelectRight;
        if (control)
            return TextEditAction::MoveWordRight;
        if (controlShift)
            return TextEditAction::SelectWordRight;
        break;
    case Key::Home:
        if (plain)
            return TextEditAction::MoveToStart;
        if (shift)
            return TextEditAction::SelectToStart;
        break;
    case Key::End:
        if (plain)
            return TextEditAction::MoveToEnd;
        if (shift)
            return TextEditAction::SelectToEnd;
        break;
    case Key::A:
        if (control)
            return TextEditAction::SelectAll;
        break;
    case Key::C:
        if (control)
            return TextEditAction::Copy;
        break;
    case Key::X:
        if (control)
            return TextEditAction::Cut;
        break;
    case Key::V:
        if (control)
            return TextEditAction::Paste;
        break;
    default:
        break;
    }
    return std::nullopt;
}

void applyTextEditAction(TextEditModel& model, TextEditAction action) {
    switch (action) {
    case TextEditAction::DeleteBackward:     model.deleteBackward(); break;
    case TextEditAction::DeleteForward:      model.deleteForward(); break;
    case TextEditAction::DeleteWordBackward: model.deleteWordBackward(); break;
    case TextEditAction::DeleteWordForward:  model.deleteWordForward(); break;
    case TextEditAction::MoveLeft:           model.moveLeft(); break;
    case TextEditAction::MoveRight:          model.moveRight(); break;
    case TextEditAction::MoveToStart:        model.moveToStart(); break;
    case TextEditAction::MoveToEnd:          model.moveToEnd(); break;
    case TextEditAction::MoveWordLeft:       model.moveWordLeft(); break;
    case TextEditAction::MoveWordRight:      model.moveWordRight(); break;
    case TextEditAction::SelectLeft:         model.moveLeft(true); break;
    case TextEditAction::SelectRight:        model.moveRight(true); break;
    case TextEditAction::SelectToStart:      model.moveToStart(true); break;
    case TextEditAction::SelectToEnd:        model.moveToEnd(true); break;
    case TextEditAction::SelectWordLeft:     model.moveWordLeft(true); break;
    case TextEditAction::SelectWordRight:    model.moveWordRight(true); break;
    case TextEditAction::SelectAll:          model.selectAll(); break;
    case TextEditAction::Copy:
        if (model.hasSelection())
            Clipboard::setText(model.selectedText());
        break;
    case TextEditAction::Cut:
        if (model.hasSelection()) {
            Clipboard::setText(model.selectedText());
            model.deleteBackward();
        }
        break;
    case TextEditAction::Paste:
        model.insert(Clipboard::text());
        break;
    }
}

bool isTextInputKey(Key key, KeyModifier modifiers) {
    if (hasFlag(modifiers, KeyModifier::Control) || hasFlag(modifiers, KeyModifier::Alt) || hasFlag(modifiers, KeyModifier::Super))
        return false;
    const int code = static_cast<int>(key);
    const bool printable = code >= static_cast<int>(Key::Space) && code <= static_cast<int>(Key::GraveAccent);
    const bool keypad = code >= static_cast<int>(Key::Kp0) && code <= static_cast<int>(Key::KpEqual) && key != Key::KpEnter;
    return printable || keypad || key == Key::World1 || key == Key::World2;
}

}
