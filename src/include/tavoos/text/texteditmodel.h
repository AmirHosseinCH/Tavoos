#pragma once

#include <tavoos/export.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace Tavoos {

class TAVOOS_EXPORT TextEditModel {
public:
    const std::string& text() const { return m_text; }
    std::size_t cursor() const { return m_cursor; }
    std::size_t anchor() const { return m_anchor; }
    bool hasSelection() const { return m_anchor != m_cursor; }
    std::size_t selectionStart() const { return std::min(m_anchor, m_cursor); }
    std::size_t selectionEnd() const { return std::max(m_anchor, m_cursor); }
    std::string selectedText() const;
    std::pair<std::size_t, std::size_t> wordAt(std::size_t position) const;
    bool singleLine() const { return m_singleLine; }

    void setSingleLine(bool singleLine) { m_singleLine = singleLine; }
    void setText(std::string text);
    void setCursor(std::size_t position, bool extend = false);
    void select(std::size_t start, std::size_t end);
    void selectAll();
    void deselect();

    void insert(std::string_view text);
    void deleteBackward();
    void deleteForward();
    void deleteWordBackward();
    void deleteWordForward();

    void moveLeft(bool extend = false);
    void moveRight(bool extend = false);
    void moveToStart(bool extend = false);
    void moveToEnd(bool extend = false);
    void moveWordLeft(bool extend = false);
    void moveWordRight(bool extend = false);

    void onTextChanged(std::function<void()> callback) { m_onTextChanged = std::move(callback); }
    void onCursorChanged(std::function<void()> callback) { m_onCursorChanged = std::move(callback); }

private:
    bool removeSelection();
    void finishEdit(std::size_t cursor);
    void moveTo(std::size_t cursor, bool extend);
    void setSelection(std::size_t anchor, std::size_t cursor);

    std::string m_text;
    std::size_t m_cursor{0};
    std::size_t m_anchor{0};
    bool m_singleLine{false};
    std::function<void()> m_onTextChanged;
    std::function<void()> m_onCursorChanged;
};

}
