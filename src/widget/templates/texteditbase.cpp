#include <tavoos/widget/templates/texteditbase.h>

#include <tavoos/events/key.h>
#include <tavoos/text/utf8.h>
#include <tavoos/window.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Tavoos {

namespace {

constexpr float kBlinkInterval = 0.5f;

class TextDisplay : public TextWidget {
public:
    TextDisplay(Object* parent) : TextWidget{parent} {}

    std::function<void()> resized;

protected:
    void onResolvedSizeChanged() override {
        if (resized)
            resized();
    }
};

}

TextEditBase::TextEditBase(Object* parent) : Control{parent} {
    cursor(CursorShape::IBeam);
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
        m_goalX.reset();
        refreshDisplay();
        m_canUndo.setIfChanged(m_model.canUndo());
        m_canRedo.setIfChanged(m_model.canRedo());
        onModelTextChanged();
        if (!m_changeFromCode && m_onTextEdited)
            m_onTextEdited(m_model.text());
    });
    m_model.onCursorChanged([this] {
        if (!m_movingVertically)
            m_goalX.reset();
        updateCaret();
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

    m_innerPaddingLeft.onChange([this](const float&) {
        updateCaret();
        onDisplayChanged();
    });
    m_innerPaddingTop.onChange([this](const float&) { onDisplayChanged(); });
    m_innerPaddingRight.onChange([this](const float&) {
        updateCaret();
        onDisplayChanged();
    });
    m_innerPaddingBottom.onChange([this](const float&) { onDisplayChanged(); });
    m_readOnly.onChange([this](const bool& value) {
        m_model.setReadOnly(value);
        resetCaretBlink();
    });

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
            placeholder.font(m_font.state());
        });

        viewport.addChild<TextDisplay>([this](TextDisplay& text) {
            m_textDisplay = &text;
            text.resized = [this] { onDisplayResized(); };
            text.font(m_font.state())
                .selectionColor(m_selectionColor.state())
                .selectedTextColor(m_selectedTextColor.state());

            text.addChild<RectangleWidget>([this](RectangleWidget& caret) {
                m_caret = &caret;
                caret.width(CaretWidth)
                    .height(m_caretHeight)
                    .x(m_caretX)
                    .y(m_caretY)
                    .color(m_caretColor.state())
                    .visible(m_caretVisible);
            });
        });
    });

    m_font.onChange([this](const Font&) {
        updateCaret();
        onDisplayChanged();
    });

    updateDisplayedText();
    updateCaret();
}

TextEditBase::~TextEditBase() {
    if (m_blinking)
        AnimationManager::instance().unregisterAnimation(this);
}

bool TextEditBase::hasHandlerFor(EventType type) {
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

void TextEditBase::triggerPress(MouseEvent& event) {
    if (enabled()) {
        m_goalX.reset();
        m_pressPoint = mapToWindow({event.x(), event.y()});
        m_pressClickCount = m_selectByMouse.get() ? event.clickCount() : 1;
        if (m_pressClickCount == 2 && !selectsWords())
            m_pressClickCount = 3;
        if (m_pressClickCount >= 3) {
            const auto paragraph = m_model.paragraphAt(characterAtPoint(m_pressPoint));
            m_model.select(paragraph.first, paragraph.second);
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

void TextEditBase::triggerDragMove(DragEvent& event) {
    if (m_selectByMouse.get() && enabled()) {
        const Point point{m_pressPoint.x + event.totalDx(), m_pressPoint.y + event.totalDy()};
        if (m_pressClickCount == 2)
            extendWordSelection(point);
        else if (m_pressClickCount == 1)
            moveCursorToPoint(point, true);
    }
    Widget::triggerDragMove(event);
}

void TextEditBase::triggerFocusIn(Event& event) {
    Widget::triggerFocusIn(event);
    resetCaretBlink();
    if (!m_blinking) {
        m_blinking = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

void TextEditBase::triggerFocusOut(Event& event) {
    if (m_blinking) {
        m_blinking = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
    m_caretVisible.set(false);
    m_model.deselect();
    if (acceptableInput())
        sendEditingFinished();
    Widget::triggerFocusOut(event);
}

void TextEditBase::triggerKeyPress(KeyEvent& event) {
    if (!enabled())
        return;

    const auto action = textEditActionFor(static_cast<Key>(event.keyCode()), event.modifiers());
    if (action && allows(*action)) {
        applyAction(*action);
        resetCaretBlink();
    } else {
        event.ignore();
    }
}

void TextEditBase::triggerKeyRelease(KeyEvent&) {
}

void TextEditBase::triggerShortcutOverride(KeyEvent& event) {
    const Key key = static_cast<Key>(event.keyCode());
    const auto action = textEditActionFor(key, event.modifiers());
    if (enabled() && ((action && allows(*action)) || (!m_readOnly.get() && isTextInputKey(key, event.modifiers()))))
        event.accept();
    Widget::triggerShortcutOverride(event);
}

void TextEditBase::triggerTextInput(KeyEvent& event) {
    if (!enabled() || m_readOnly.get())
        return;
    m_model.type(encodeUtf8(static_cast<char32_t>(event.keyCode())));
    resetCaretBlink();
}

bool TextEditBase::tick(float dt) {
    m_blinkElapsed += dt;
    if (m_blinkElapsed >= kBlinkInterval) {
        m_blinkElapsed -= kBlinkInterval;
        m_caretVisible.set(!m_readOnly.get() && !m_caretVisible.get());
    }
    return true;
}

void TextEditBase::resetCaretBlink() {
    m_blinkElapsed = 0.0f;
    if (m_blinking)
        m_caretVisible.set(!m_readOnly.get());
}

int TextEditBase::cursorPosition() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.cursor()));
}

int TextEditBase::selectionStart() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.selectionStart()));
}

int TextEditBase::selectionEnd() const {
    return static_cast<int>(characterIndexAt(m_model.text(), m_model.selectionEnd()));
}

void TextEditBase::select(int start, int end) {
    m_model.select(byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(start, 0))),
                   byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(end, 0))));
}

void TextEditBase::selectAll() {
    m_model.selectAll();
}

void TextEditBase::deselect() {
    m_model.deselect();
}

void TextEditBase::undo() {
    m_model.undo();
}

void TextEditBase::redo() {
    m_model.redo();
}

void TextEditBase::moveCursorTo(int position) {
    m_model.setCursor(byteOffsetOfCharacter(m_model.text(), static_cast<std::size_t>(std::max(position, 0))));
}

void TextEditBase::moveCursorToPoint(const Point& windowPoint, bool extend) {
    if (!m_textDisplay)
        return;
    const Point local = m_textDisplay->mapFromWindow(windowPoint);
    m_model.setCursor(toModel(m_textDisplay->positionAt(local.x, local.y)), extend);
    resetCaretBlink();
}

void TextEditBase::extendWordSelection(const Point& windowPoint) {
    const auto word = m_model.wordAt(characterAtPoint(windowPoint));
    if (word.first < m_pressWord.first)
        m_model.select(m_pressWord.second, word.first);
    else
        m_model.select(m_pressWord.first, std::max(word.second, m_pressWord.second));
    resetCaretBlink();
}

std::size_t TextEditBase::characterAtPoint(const Point& windowPoint) {
    if (!m_textDisplay)
        return 0;
    const Point local = m_textDisplay->mapFromWindow(windowPoint);
    const std::size_t index = m_textDisplay->positionAt(local.x, local.y);
    if (index > 0 && m_textDisplay->positionToRectangle(index).x > local.x)
        return toModel(previousCodepointStart(m_textDisplay->text(), index));
    return toModel(index);
}

bool TextEditBase::allows(TextEditAction action) const {
    return !(m_readOnly.get() && changesText(action));
}

void TextEditBase::applyAction(TextEditAction action) {
    if (needsLayout(action))
        applyLayoutAction(action);
    else
        applyTextEditAction(m_model, action);
}

void TextEditBase::applyLayoutAction(TextEditAction action) {
    if (!m_textDisplay)
        return;

    const Rect caret = m_textDisplay->positionToRectangle(toDisplay(m_model.cursor()));
    const float middle = caret.y + caret.height * 0.5f;
    switch (action) {
    case TextEditAction::MoveToLineStart:
    case TextEditAction::SelectToLineStart:
        m_model.setCursor(toModel(m_textDisplay->positionAt(std::numeric_limits<float>::lowest(), middle)),
                          action == TextEditAction::SelectToLineStart);
        break;
    case TextEditAction::MoveToLineEnd:
    case TextEditAction::SelectToLineEnd:
        m_model.setCursor(toModel(m_textDisplay->positionAt(std::numeric_limits<float>::max(), middle)),
                          action == TextEditAction::SelectToLineEnd);
        break;
    case TextEditAction::MoveUp:       moveVertically(caret, -caret.height, false); break;
    case TextEditAction::MoveDown:     moveVertically(caret, caret.height, false); break;
    case TextEditAction::MovePageUp:   moveVertically(caret, -pageHeight(caret.height), false); break;
    case TextEditAction::MovePageDown: moveVertically(caret, pageHeight(caret.height), false); break;
    case TextEditAction::SelectUp:       moveVertically(caret, -caret.height, true); break;
    case TextEditAction::SelectDown:     moveVertically(caret, caret.height, true); break;
    case TextEditAction::SelectPageUp:   moveVertically(caret, -pageHeight(caret.height), true); break;
    case TextEditAction::SelectPageDown: moveVertically(caret, pageHeight(caret.height), true); break;
    default:
        break;
    }
}

void TextEditBase::moveVertically(const Rect& caret, float distance, bool extend) {
    if (!m_goalX)
        m_goalX = caret.x;

    const std::size_t end = toDisplay(m_model.text().size());
    const float target = caret.y + caret.height * 0.5f + distance;
    const Rect last = m_textDisplay->positionToRectangle(end);
    std::size_t position = 0;
    if (target >= last.y + last.height)
        position = end;
    else if (target >= m_textDisplay->positionToRectangle(0).y)
        position = m_textDisplay->positionAt(*m_goalX, target);

    m_movingVertically = true;
    m_model.setCursor(toModel(position), extend);
    m_movingVertically = false;
}

float TextEditBase::pageHeight(float lineHeight) const {
    const auto verticalExtent = [](const Widget& widget) {
        float top = std::numeric_limits<float>::max();
        float bottom = std::numeric_limits<float>::lowest();
        for (const Point corner : {Point{0.0f, 0.0f}, Point{widget.displayedWidth(), 0.0f},
                                   Point{0.0f, widget.displayedHeight()}, Point{widget.displayedWidth(), widget.displayedHeight()}}) {
            const float y = widget.mapToWindow(corner).y;
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        }
        return std::pair{top, bottom};
    };

    auto [top, bottom] = verticalExtent(*m_textDisplay);
    for (auto* widget = dynamic_cast<const Widget*>(m_textDisplay->parent()); widget;
         widget = dynamic_cast<const Widget*>(widget->parent())) {
        if (widget->clip()) {
            const auto [clipTop, clipBottom] = verticalExtent(*widget);
            top = std::max(top, clipTop);
            bottom = std::min(bottom, clipBottom);
        }
        if (widget->isOverlay())
            break;
    }
    if (const Window* window = ownerWindow()) {
        top = std::max(top, 0.0f);
        bottom = std::min(bottom, static_cast<float>(window->height()));
    }
    return std::max(bottom - top, lineHeight);
}

void TextEditBase::sendEditingFinished() {
    if (m_onEditingFinished)
        m_onEditingFinished(m_model.text());
}

void TextEditBase::refreshDisplay() {
    updateDisplayedText();
    updateCaret();
    updateSelection();
}

void TextEditBase::updateSelection() {
    if (m_textDisplay)
        m_textDisplay->selection(toDisplay(m_model.selectionStart()), toDisplay(m_model.selectionEnd()));
}

void TextEditBase::updateDisplayedText() {
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
    onDisplayChanged();
}

void TextEditBase::onResolvedSizeChanged() {
    updateCaret();
}

void TextEditBase::updateCaret() {
    if (!m_textDisplay)
        return;

    const Rect caret = m_textDisplay->positionToRectangle(toDisplay(m_model.cursor()));
    keepCaretVisible(caret);
    m_caretX.set(static_cast<int>(std::round(caret.x)));
    m_caretY.set(static_cast<int>(std::round(caret.y)));
    m_caretHeight.set(static_cast<int>(std::round(caret.height)));
}

}
