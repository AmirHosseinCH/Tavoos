#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/font.h>
#include <tavoos/text/texteditactions.h>
#include <tavoos/text/texteditmodel.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>
#include <tavoos/widget/text.h>

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <utility>

namespace Tavoos {

enum class EchoMode {
    Normal,
    Password,
    NoEcho,
    PasswordEchoOnEdit,
};

class TAVOOS_EXPORT TextFieldBase : public Control, private AnimatableBase {
public:
    TextFieldBase(Object* parent);
    ~TextFieldBase() override;

    decltype(auto) text(this auto&& self, PropertyArg<std::string> content) {
        content.applyTo(self.m_text);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) placeholder(this auto&& self, PropertyArg<std::string> content) {
        content.applyTo(self.m_placeholder);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, const Font& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, State<Font>& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) textColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_textColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) placeholderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_placeholderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledTextColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledTextColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) caretColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_caretColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) selectionColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_selectionColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) selectedTextColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_selectedTextColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) selectByMouse(this auto&& self, PropertyArg<bool> enabled) {
        enabled.applyTo(self.m_selectByMouse);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) readOnly(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_readOnly);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) maximumLength(this auto&& self, PropertyArg<int> length) {
        length.applyTo(self.m_maximumLength);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) echoMode(this auto&& self, PropertyArg<EchoMode> mode) {
        mode.applyTo(self.m_echoMode);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) passwordCharacter(this auto&& self, PropertyArg<std::string> character) {
        character.applyTo(self.m_passwordCharacter);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) cursorPosition(this auto&& self, int position) {
        self.moveCursorTo(position);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerPadding(this auto&& self, PropertyArg<float> value) {
        self.m_innerPaddingLeft.set(value);
        self.m_innerPaddingTop.set(value);
        self.m_innerPaddingRight.set(value);
        self.m_innerPaddingBottom.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerPaddingLeft(this auto&& self, PropertyArg<float> value) {
        self.m_innerPaddingLeft.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerPaddingTop(this auto&& self, PropertyArg<float> value) {
        self.m_innerPaddingTop.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerPaddingRight(this auto&& self, PropertyArg<float> value) {
        self.m_innerPaddingRight.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerPaddingBottom(this auto&& self, PropertyArg<float> value) {
        self.m_innerPaddingBottom.set(value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onSubmit(this auto&& self, std::function<void(const std::string&)> callback) {
        self.m_onSubmit = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    std::string text() const { return m_model.text(); }
    State<std::string>& textState() { return m_textState; }
    std::string placeholder() const { return m_placeholder; }
    Font font() const { return m_font; }
    Paint textColor() const { return m_textColor; }
    Paint placeholderColor() const { return m_placeholderColor; }
    Paint disabledTextColor() const { return m_disabledTextColor; }
    Paint caretColor() const { return m_caretColor; }
    Paint selectionColor() const { return m_selectionColor; }
    Paint selectedTextColor() const { return m_selectedTextColor; }
    float innerPaddingLeft() const { return m_innerPaddingLeft; }
    float innerPaddingTop() const { return m_innerPaddingTop; }
    float innerPaddingRight() const { return m_innerPaddingRight; }
    float innerPaddingBottom() const { return m_innerPaddingBottom; }
    bool selectByMouse() const { return m_selectByMouse; }
    bool readOnly() const { return m_readOnly; }
    int maximumLength() const { return m_maximumLength; }
    EchoMode echoMode() const { return m_echoMode; }
    std::string passwordCharacter() const { return m_passwordCharacter; }
    std::string displayText() const;

    int cursorPosition() const;
    int selectionStart() const;
    int selectionEnd() const;
    std::string selectedText() const { return m_model.selectedText(); }
    void select(int start, int end);
    void selectAll();
    void deselect();
    bool canUndo() const { return m_canUndo; }
    bool canRedo() const { return m_canRedo; }
    State<bool>& canUndoState() { return m_canUndo; }
    State<bool>& canRedoState() { return m_canRedo; }
    void undo();
    void redo();

protected:
    bool hasHandlerFor(EventType type) override;
    void triggerPress(MouseEvent& event) override;
    void triggerDragMove(DragEvent& event) override;
    void triggerFocusIn(Event& event) override;
    void triggerFocusOut(Event& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerKeyRelease(KeyEvent& event) override;
    void triggerShortcutOverride(KeyEvent& event) override;
    void triggerTextInput(KeyEvent& event) override;
    void onResolvedSizeChanged() override;

private:
    enum class Shown { Plain, Masked, Hidden };

    using Control::content;

    bool tick(float dt) override;

    Shown shown() const;
    std::string mask() const;
    std::size_t toDisplay(std::size_t position) const;
    std::size_t toModel(std::size_t position) const;
    bool allows(TextEditAction action) const;
    void applyAction(TextEditAction action);
    void beginPasswordEdit();
    void refreshDisplay();
    void updateDisplayedText();
    void updateCaretAndScroll();
    void updateSelection();
    void resetCaretBlink();
    void moveCursorTo(int position);
    void moveCursorToPoint(const Point& windowPoint, bool extend);
    void extendWordSelection(const Point& windowPoint);
    std::size_t characterAtPoint(const Point& windowPoint);

    Property<std::string> m_text{};
    State<std::string> m_textState{};
    Property<std::string> m_placeholder{};
    BindableState<Font> m_font;
    Property<Paint> m_textColor{Color::Black};
    Property<Paint> m_placeholderColor{Color::rgba(160, 163, 175)};
    Property<Paint> m_disabledTextColor{Color::rgba(160, 163, 175)};
    BindableState<Paint> m_caretColor{Color::rgba(85, 112, 241)};
    BindableState<Paint> m_selectionColor{Color::rgba(85, 112, 241)};
    BindableState<Paint> m_selectedTextColor{Color::White};
    BindableState<float> m_innerPaddingLeft{10.0f};
    BindableState<float> m_innerPaddingTop{0.0f};
    BindableState<float> m_innerPaddingRight{10.0f};
    BindableState<float> m_innerPaddingBottom{0.0f};
    Property<bool> m_selectByMouse{true};
    Property<bool> m_readOnly{false};
    Property<int> m_maximumLength{std::numeric_limits<int>::max()};
    Property<EchoMode> m_echoMode{EchoMode::Normal};
    Property<std::string> m_passwordCharacter{"\xE2\x80\xA2"};

    std::function<void(const std::string&)> m_onSubmit;

    TextEditModel m_model;

    TextWidget* m_textDisplay{nullptr};
    TextWidget* m_placeholderDisplay{nullptr};
    State<int> m_contentX{0};
    State<int> m_caretX{0};
    State<int> m_caretHeight{0};
    State<bool> m_caretVisible{false};
    State<bool> m_canUndo{false};
    State<bool> m_canRedo{false};

    Point m_pressPoint{};
    int m_pressClickCount{1};
    std::pair<std::size_t, std::size_t> m_pressWord{};
    bool m_changeFromCode{false};
    bool m_passwordEditing{false};
    int m_scrollOffset{0};
    float m_blinkElapsed{0.0f};
    bool m_blinking{false};
};

}
