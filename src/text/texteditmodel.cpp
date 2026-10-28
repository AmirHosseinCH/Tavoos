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

void TextEditModel::setText(std::string text) {
    if (text == m_text)
        return;
    m_text = std::move(text);
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
    if (text.empty())
        return;
    const std::string inserted = m_singleLine ? withoutLineBreaks(text) : std::string{text};
    const std::size_t start = selectionStart();
    m_text.replace(start, selectionEnd() - start, inserted);
    finishEdit(start + inserted.size());
}

void TextEditModel::deleteBackward() {
    if (removeSelection() || m_cursor == 0)
        return;
    const std::size_t start = previousCodepointStart(m_text, m_cursor);
    m_text.erase(start, m_cursor - start);
    finishEdit(start);
}

void TextEditModel::deleteForward() {
    if (removeSelection() || m_cursor >= m_text.size())
        return;
    m_text.erase(m_cursor, decodeUtf8At(m_text, m_cursor).length);
    finishEdit(m_cursor);
}

void TextEditModel::deleteWordBackward() {
    if (removeSelection())
        return;
    const std::size_t start = previousWordStart(m_text, m_cursor);
    if (start == m_cursor)
        return;
    m_text.erase(start, m_cursor - start);
    finishEdit(start);
}

void TextEditModel::deleteWordForward() {
    if (removeSelection())
        return;
    const std::size_t end = nextWordStart(m_text, m_cursor);
    if (end == m_cursor)
        return;
    m_text.erase(m_cursor, end - m_cursor);
    finishEdit(m_cursor);
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

bool TextEditModel::removeSelection() {
    if (!hasSelection())
        return false;
    const std::size_t start = selectionStart();
    m_text.erase(start, selectionEnd() - start);
    finishEdit(start);
    return true;
}

void TextEditModel::finishEdit(std::size_t cursor) {
    const bool cursorMoved = cursor != m_cursor || cursor != m_anchor;
    m_cursor = cursor;
    m_anchor = cursor;
    if (m_onTextChanged)
        m_onTextChanged();
    if (cursorMoved && m_onCursorChanged)
        m_onCursorChanged();
}

void TextEditModel::moveTo(std::size_t cursor, bool extend) {
    setSelection(extend ? m_anchor : cursor, cursor);
}

void TextEditModel::setSelection(std::size_t anchor, std::size_t cursor) {
    if (anchor == m_anchor && cursor == m_cursor)
        return;
    m_anchor = anchor;
    m_cursor = cursor;
    if (m_onCursorChanged)
        m_onCursorChanged();
}

}
