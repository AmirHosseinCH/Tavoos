#pragma once

#include <tavoos/events/key.h>
#include <tavoos/events/mouseevent.h>
#include <tavoos/export.hpp>
#include <tavoos/text/texteditmodel.h>

#include <optional>

namespace Tavoos {

enum class TextEditAction {
    DeleteBackward,
    DeleteForward,
    MoveLeft,
    MoveRight,
    MoveToStart,
    MoveToEnd,
    SelectLeft,
    SelectRight,
    SelectToStart,
    SelectToEnd,
    SelectAll,
};

TAVOOS_EXPORT std::optional<TextEditAction> textEditActionFor(Key key, KeyModifier modifiers);
TAVOOS_EXPORT void applyTextEditAction(TextEditModel& model, TextEditAction action);
TAVOOS_EXPORT bool isTextInputKey(Key key, KeyModifier modifiers);

}
