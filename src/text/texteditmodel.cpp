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

}

std::string TextEditModel::selectedText() const {
    return m_text.substr(selectionStart(), selectionEnd() - selectionStart());
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
