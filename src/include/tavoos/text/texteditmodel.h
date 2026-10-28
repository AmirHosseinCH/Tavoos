#pragma once

#include <tavoos/export.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace Tavoos {

class TAVOOS_EXPORT TextEditModel {
public:
    const std::string& text() const { return m_text; }
    std::size_t cursor() const { return m_cursor; }

    void setText(std::string text);
    void setCursor(std::size_t position);

    void insert(std::string_view text);
    void deleteBackward();
    void deleteForward();

    void moveLeft();
    void moveRight();
    void moveToStart();
    void moveToEnd();

    void onTextChanged(std::function<void()> callback) { m_onTextChanged = std::move(callback); }
    void onCursorChanged(std::function<void()> callback) { m_onCursorChanged = std::move(callback); }

private:
    void finishEdit(std::size_t cursor);
    void moveCursor(std::size_t cursor);

    std::string m_text;
    std::size_t m_cursor{0};
    std::function<void()> m_onTextChanged;
    std::function<void()> m_onCursorChanged;
};

}
