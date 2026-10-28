#include <tavoos/text/texteditmodel.h>

#include <tavoos/text/utf8.h>

namespace Tavoos {

void TextEditModel::setText(std::string text) {
    if (text == m_text)
        return;
    m_text = std::move(text);
    finishEdit(codepointStartAt(m_text, m_cursor));
}

void TextEditModel::setCursor(std::size_t position) {
    moveCursor(codepointStartAt(m_text, position));
}

void TextEditModel::insert(std::string_view text) {
    if (text.empty())
        return;
    m_text.insert(m_cursor, text);
    finishEdit(m_cursor + text.size());
}

void TextEditModel::deleteBackward() {
    if (m_cursor == 0)
        return;
    const std::size_t start = previousCodepointStart(m_text, m_cursor);
    m_text.erase(start, m_cursor - start);
    finishEdit(start);
}

void TextEditModel::deleteForward() {
    if (m_cursor >= m_text.size())
        return;
    m_text.erase(m_cursor, decodeUtf8At(m_text, m_cursor).length);
    finishEdit(m_cursor);
}

void TextEditModel::moveLeft() {
    if (m_cursor > 0)
        moveCursor(previousCodepointStart(m_text, m_cursor));
}

void TextEditModel::moveRight() {
    if (m_cursor < m_text.size())
        moveCursor(m_cursor + decodeUtf8At(m_text, m_cursor).length);
}

void TextEditModel::moveToStart() {
    moveCursor(0);
}

void TextEditModel::moveToEnd() {
    moveCursor(m_text.size());
}

void TextEditModel::finishEdit(std::size_t cursor) {
    const bool cursorMoved = cursor != m_cursor;
    m_cursor = cursor;
    if (m_onTextChanged)
        m_onTextChanged();
    if (cursorMoved && m_onCursorChanged)
        m_onCursorChanged();
}

void TextEditModel::moveCursor(std::size_t cursor) {
    if (cursor == m_cursor)
        return;
    m_cursor = cursor;
    if (m_onCursorChanged)
        m_onCursorChanged();
}

}
