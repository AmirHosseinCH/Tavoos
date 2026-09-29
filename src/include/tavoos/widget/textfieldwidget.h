#pragma once

#include <tavoos/animation/animatedstate.h>
#include <tavoos/animation/animationmanager.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/buttonbase.h>
#include <tavoos/widget/style/textfieldstyle.h>
#include <tavoos/widget/text.h>

#include <cstddef>
#include <functional>
#include <string>

namespace Tavoos {

class TAVOOS_EXPORT TextFieldWidget : public ButtonBase, private AnimatableBase {
public:
    TextFieldWidget(Object* parent);
    ~TextFieldWidget() override;

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

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_backgroundColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_borderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) focusedBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_focusedBorderColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledColor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) disabledBorderColor(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_disabledBorderColor);
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

    decltype(auto) caretColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_caretColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        self.m_borderWidth.set(width);
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

    decltype(auto) transition(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_transition);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const TextFieldStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<TextFieldStyle>& style) {
        self.m_style.set(style);
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
    Paint backgroundColor() const { return m_backgroundColor; }
    Paint borderColor() const { return m_borderColor; }
    Paint focusedBorderColor() const { return m_focusedBorderColor; }
    Paint disabledColor() const { return m_disabledColor; }
    Paint disabledBorderColor() const { return m_disabledBorderColor; }
    Paint textColor() const { return m_textColor; }
    Paint placeholderColor() const { return m_placeholderColor; }
    Paint caretColor() const { return m_caretColor; }
    int radius() const { return m_radius; }
    float borderWidth() const { return m_borderWidth; }
    float innerPaddingLeft() const { return m_innerPaddingLeft; }
    float innerPaddingTop() const { return m_innerPaddingTop; }
    float innerPaddingRight() const { return m_innerPaddingRight; }
    float innerPaddingBottom() const { return m_innerPaddingBottom; }
    float transition() const { return m_transition; }
    TextFieldStyle style() const {
        return { m_backgroundColor.get(), m_borderColor.get(), m_focusedBorderColor.get(),
                 m_disabledColor.get(), m_disabledBorderColor.get(), m_textColor.get(),
                 m_placeholderColor.get(), m_caretColor.get(), m_radius.get(),
                 m_borderWidth.get(), m_innerPaddingLeft.get(), m_innerPaddingTop.get(),
                 m_innerPaddingRight.get(), m_innerPaddingBottom.get(), m_font.get(), m_transition.get() };
    }

protected:
    void render(Renderer& renderer) override;
    bool hasHandlerFor(EventType type) override;
    void triggerPress(MouseEvent& event) override;
    void triggerFocusIn(Event& event) override;
    void triggerFocusOut(Event& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerKeyRelease(KeyEvent& event) override;
    void triggerTextInput(KeyEvent& event) override;

private:
    bool tick(float dt) override;

    void updateColor(bool animate);
    void updateDisplayedText();
    void updateCaretAndScroll();
    void resetCaretBlink();
    void insertText(const std::string& utf8);
    void deleteBackward();
    void deleteForward();
    void moveCursorLeft();
    void moveCursorRight();
    void applyStyle(const TextFieldStyle& style);

    Property<std::string> m_text{};
    State<std::string> m_textState{};
    Property<std::string> m_placeholder{};
    BindableState<Font> m_font;
    Property<Paint> m_backgroundColor{Color::White};
    Property<Paint> m_borderColor{Color::rgba(205, 208, 218)};
    Property<Paint> m_focusedBorderColor{Color::rgba(85, 112, 241)};
    Property<Paint> m_disabledColor{Color::rgba(228, 229, 235)};
    Property<Paint> m_disabledBorderColor{Color::rgba(220, 222, 230)};
    Property<Paint> m_textColor{Color::Black};
    Property<Paint> m_placeholderColor{Color::rgba(160, 163, 175)};
    BindableState<Paint> m_caretColor{Color::rgba(85, 112, 241)};
    BindableState<int> m_radius{6};
    BindableState<float> m_borderWidth{1.5f};
    BindableState<float> m_innerPaddingLeft{10.0f};
    BindableState<float> m_innerPaddingTop{0.0f};
    BindableState<float> m_innerPaddingRight{10.0f};
    BindableState<float> m_innerPaddingBottom{0.0f};
    Property<float> m_transition{0.12f};
    BindableState<TextFieldStyle> m_style;

    std::function<void(const std::string&)> m_onSubmit;

    AnimatedState<Paint> m_backgroundColorOut;
    AnimatedState<Paint> m_borderColorOut;

    TextWidget* m_textDisplay{nullptr};
    TextWidget* m_placeholderDisplay{nullptr};
    State<int> m_contentX{0};
    State<int> m_caretX{0};
    State<bool> m_caretVisible{false};

    std::size_t m_cursorByteIndex{0};
    int m_scrollOffset{0};
    float m_blinkElapsed{0.0f};
    bool m_blinking{false};
    bool m_settled{false};
};

}
