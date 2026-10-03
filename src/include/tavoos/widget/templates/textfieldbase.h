#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/font.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/control.h>
#include <tavoos/widget/text.h>

#include <cstddef>
#include <functional>
#include <string>

namespace Tavoos {

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

    std::string text() const { return m_text; }
    State<std::string>& textState() { return m_textState; }
    std::string placeholder() const { return m_placeholder; }
    Font font() const { return m_font; }
    Paint textColor() const { return m_textColor; }
    Paint placeholderColor() const { return m_placeholderColor; }
    Paint disabledTextColor() const { return m_disabledTextColor; }
    Paint caretColor() const { return m_caretColor; }
    float innerPaddingLeft() const { return m_innerPaddingLeft; }
    float innerPaddingTop() const { return m_innerPaddingTop; }
    float innerPaddingRight() const { return m_innerPaddingRight; }
    float innerPaddingBottom() const { return m_innerPaddingBottom; }

protected:
    bool hasHandlerFor(EventType type) override;
    void triggerPress(MouseEvent& event) override;
    void triggerFocusIn(Event& event) override;
    void triggerFocusOut(Event& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerKeyRelease(KeyEvent& event) override;
    void triggerTextInput(KeyEvent& event) override;
    void onResolvedSizeChanged() override;

private:
    using Control::content;

    bool tick(float dt) override;

    void updateDisplayedText();
    void updateCaretAndScroll();
    void resetCaretBlink();
    void insertText(const std::string& utf8);
    void deleteBackward();
    void deleteForward();
    void moveCursorLeft();
    void moveCursorRight();

    Property<std::string> m_text{};
    State<std::string> m_textState{};
    Property<std::string> m_placeholder{};
    BindableState<Font> m_font;
    Property<Paint> m_textColor{Color::Black};
    Property<Paint> m_placeholderColor{Color::rgba(160, 163, 175)};
    Property<Paint> m_disabledTextColor{Color::rgba(228, 229, 235)};
    BindableState<Paint> m_caretColor{Color::rgba(85, 112, 241)};
    BindableState<float> m_innerPaddingLeft{10.0f};
    BindableState<float> m_innerPaddingTop{0.0f};
    BindableState<float> m_innerPaddingRight{10.0f};
    BindableState<float> m_innerPaddingBottom{0.0f};

    std::function<void(const std::string&)> m_onSubmit;

    TextWidget* m_textDisplay{nullptr};
    TextWidget* m_placeholderDisplay{nullptr};
    State<int> m_contentX{0};
    State<int> m_caretX{0};
    State<bool> m_caretVisible{false};

    std::size_t m_cursorByteIndex{0};
    int m_scrollOffset{0};
    float m_blinkElapsed{0.0f};
    bool m_blinking{false};
};

}
