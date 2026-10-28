#include <tavoos/text/texteditmodel.h>

#include <tavoos/text/utf8.h>

namespace Tavoos {

namespace {

std::string withoutLineBreaks(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
            continue;
        result += text[i] == '\r' || text[i] == '\n' ? ' ' : text[i];
    }
    return result;
}

enum class CharacterKind { Word, Space, Punctuation };

CharacterKind kindOf(char32_t c) {
    if (c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == 0xA0 || (c >= 0x2000 && c <= 0x200A) || c == 0x3000)
        return CharacterKind::Space;
    if (c >= 0x80 || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || (c >= U'0' && c <= U'9') || c == U'_')
        return CharacterKind::Word;
    return CharacterKind::Punctuation;
}

CharacterKind kindAt(const std::string& text, std::size_t i) {
    return kindOf(decodeUtf8At(text, i).codepoint);
}

std::size_t nextWordStart(const std::string& text, std::size_t i) {
    if (i >= text.size())
        return text.size();
    const CharacterKind first = kindAt(text, i);
    if (first != CharacterKind::Space) {
        while (i < text.size() && kindAt(text, i) == first)
            i += decodeUtf8At(text, i).length;
    }
    while (i < text.size() && kindAt(text, i) == CharacterKind::Space)
        i += decodeUtf8At(text, i).length;
    return i;
}

std::size_t previousWordStart(const std::string& text, std::size_t i) {
    while (i > 0 && kindAt(text, previousCodepointStart(text, i)) == CharacterKind::Space)
        i = previousCodepointStart(text, i);
    if (i == 0)
        return 0;
    const CharacterKind last = kindAt(text, previousCodepointStart(text, i));
    while (i > 0 && kindAt(text, previousCodepointStart(text, i)) == last)
        i = previousCodepointStart(text, i);
    return i;
}

}

std::string TextEditModel::selectedText() const {
    return m_text.substr(selectionStart(), selectionEnd() - selectionStart());
}

std::pair<std::size_t, std::size_t> TextEditModel::wordAt(std::size_t position) const {
    if (m_text.empty())
        return {0, 0};
    std::size_t start = codepointStartAt(m_text, position);
    if (start == m_text.size())
        start = previousCodepointStart(m_text, start);
    const CharacterKind kind = kindAt(m_text, start);
    std::size_t end = start;
    while (end < m_text.size() && kindAt(m_text, end) == kind)
        end += decodeUtf8At(m_text, end).length;
    while (start > 0 && kindAt(m_text, previousCodepointStart(m_text, start)) == kind)
        start = previousCodepointStart(m_text, start);
    return {start, end};
}

void TextEditModel::setMaximumLength(std::size_t length) {
    m_maximumLength = length;
    setText(m_text);
}

void TextEditModel::setText(std::string text) {
    text.resize(byteOffsetOfCharacter(text, m_maximumLength));
    if (text == m_text)
        return;
    m_text = std::move(text);
    m_undoStack.clear();
    m_redoStack.clear();
    m_typing = false;
    finishEdit(codepointStartAt(m_text, m_cursor));
}

void TextEditModel::setCursor(std::size_t position, bool extend) {
    moveTo(codepointStartAt(m_text, position), extend);
}

void TextEditModel::select(std::size_t start, std::size_t end) {
    setSelection(codepointStartAt(m_text, start), codepointStartAt(m_text, end));
}

void TextEditModel::selectAll() {
    setSelection(0, m_text.size());
}

void TextEditModel::deselect() {
    setSelection(m_cursor, m_cursor);
}

void TextEditModel::insert(std::string_view text) {
    insertText(text, false);
}

void TextEditModel::type(std::string_view text) {
    insertText(text, true);
}

void TextEditModel::deleteBackward() {
    if (removeSelection() || m_cursor == 0)
        return;
    replace(previousCodepointStart(m_text, m_cursor), m_cursor, {}, false);
}

void TextEditModel::deleteForward() {
    if (removeSelection() || m_cursor >= m_text.size())
        return;
    replace(m_cursor, m_cursor + decodeUtf8At(m_text, m_cursor).length, {}, false);
}

void TextEditModel::deleteWordBackward() {
    if (removeSelection())
        return;
    const std::size_t start = previousWordStart(m_text, m_cursor);
    if (start != m_cursor)
        replace(start, m_cursor, {}, false);
}

void TextEditModel::deleteWordForward() {
    if (removeSelection())
        return;
    const std::size_t end = nextWordStart(m_text, m_cursor);
    if (end != m_cursor)
        replace(m_cursor, end, {}, false);
}

void TextEditModel::moveLeft(bool extend) {
    if (!extend && hasSelection())
        moveTo(selectionStart(), false);
    else if (m_cursor > 0)
        moveTo(previousCodepointStart(m_text, m_cursor), extend);
}

void TextEditModel::moveRight(bool extend) {
    if (!extend && hasSelection())
        moveTo(selectionEnd(), false);
    else if (m_cursor < m_text.size())
        moveTo(m_cursor + decodeUtf8At(m_text, m_cursor).length, extend);
}

void TextEditModel::moveToStart(bool extend) {
    moveTo(0, extend);
}

void TextEditModel::moveToEnd(bool extend) {
    moveTo(m_text.size(), extend);
}

void TextEditModel::moveWordLeft(bool extend) {
    moveTo(previousWordStart(m_text, m_cursor), extend);
}

void TextEditModel::moveWordRight(bool extend) {
    moveTo(nextWordStart(m_text, m_cursor), extend);
}

void TextEditModel::undo() {
    if (m_readOnly || m_undoStack.empty())
        return;
    Edit edit = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    m_text.replace(edit.position, edit.inserted.size(), edit.removed);
    m_typing = false;
    const std::size_t anchor = edit.anchorBefore;
    const std::size_t cursor = edit.cursorBefore;
    m_redoStack.push_back(std::move(edit));
    finishChange(anchor, cursor);
}

void TextEditModel::redo() {
    if (m_readOnly || m_redoStack.empty())
        return;
    Edit edit = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    m_text.replace(edit.position, edit.removed.size(), edit.inserted);
    m_typing = false;
    const std::size_t cursor = edit.position + edit.inserted.size();
    m_undoStack.push_back(std::move(edit));
    finishEdit(cursor);
}

bool TextEditModel::removeSelection() {
    if (!hasSelection())
        return false;
    replace(selectionStart(), selectionEnd(), {}, false);
    return true;
}

void TextEditModel::insertText(std::string_view text, bool typing) {
    std::string inserted = m_singleLine ? withoutLineBreaks(text) : std::string{text};
    const std::size_t selected = characterIndexAt(m_text, selectionEnd()) - characterIndexAt(m_text, selectionStart());
    const std::size_t kept = characterCount(m_text) - selected;
    inserted.resize(byteOffsetOfCharacter(inserted, m_maximumLength > kept ? m_maximumLength - kept : 0));
    if (inserted.empty())
        return;
    replace(selectionStart(), selectionEnd(), std::move(inserted), typing);
}

void TextEditModel::replace(std::size_t start, std::size_t end, std::string inserted, bool typing) {
    if (m_readOnly)
        return;
    const bool merge = typing && m_typing && start == end && !m_undoStack.empty()
                       && m_undoStack.back().position + m_undoStack.back().inserted.size() == start;
    if (merge)
        m_undoStack.back().inserted += inserted;
    else
        m_undoStack.push_back({start, m_text.substr(start, end - start), inserted, m_anchor, m_cursor});
    m_redoStack.clear();
    m_text.replace(start, end - start, inserted);
    m_typing = typing;
    finishEdit(start + inserted.size());
}

void TextEditModel::finishEdit(std::size_t cursor) {
    finishChange(cursor, cursor);
}

void TextEditModel::finishChange(std::size_t anchor, std::size_t cursor) {
    const bool cursorMoved = anchor != m_anchor || cursor != m_cursor;
    m_anchor = anchor;
    m_cursor = cursor;
    if (m_onTextChanged)
        m_onTextChanged();
    if (cursorMoved && m_onCursorChanged)
        m_onCursorChanged();
}

void TextEditModel::moveTo(std::size_t cursor, bool extend) {
    setSelection(extend ? m_anchor : cursor, cursor);
}

void TextEditModel::setSelection(std::size_t anchor, std::size_t cursor) {
    m_typing = false;
    if (anchor == m_anchor && cursor == m_cursor)
        return;
    m_anchor = anchor;
    m_cursor = cursor;
    if (m_onCursorChanged)
        m_onCursorChanged();
}

}
