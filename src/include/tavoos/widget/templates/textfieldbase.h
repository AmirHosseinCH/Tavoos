#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/texteditactions.h>
#include <tavoos/text/texteditmodel.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/texteditbase.h>

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace Tavoos {

enum class EchoMode {
    Normal,
    Password,
    NoEcho,
    PasswordEchoOnEdit,
};

class TAVOOS_EXPORT TextFieldBase : public TextEditBase {
public:
    TextFieldBase(Object* parent);

    decltype(auto) echoMode(this auto&& self, PropertyArg<EchoMode> mode) {
        mode.applyTo(self.m_echoMode);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) passwordCharacter(this auto&& self, PropertyArg<std::string> character) {
        character.applyTo(self.m_passwordCharacter);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) validator(this auto&& self, TextValidator validator) {
        self.setValidator(std::move(validator));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onSubmit(this auto&& self, std::function<void(const std::string&)> callback) {
        self.m_onSubmit = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    EchoMode echoMode() const { return m_echoMode; }
    std::string passwordCharacter() const { return m_passwordCharacter; }
    std::string displayText() const override;
    bool acceptableInput() const override { return m_acceptableInput; }
    State<bool>& acceptableInputState() { return m_acceptableInput; }

protected:
    void triggerFocusOut(Event& event) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerTextInput(KeyEvent& event) override;

    std::size_t toDisplay(std::size_t position) const override;
    std::size_t toModel(std::size_t position) const override;
    bool allows(TextEditAction action) const override;
    void applyAction(TextEditAction action) override;
    bool selectsWords() const override { return m_echoMode.get() == EchoMode::Normal; }
    void onModelTextChanged() override;
    void keepCaretVisible(const Rect& caret) override;

private:
    enum class Shown { Plain, Masked, Hidden };

    Shown shown() const;
    std::string mask() const;
    void beginPasswordEdit();
    void setValidator(TextValidator validator);

    Property<EchoMode> m_echoMode{EchoMode::Normal};
    Property<std::string> m_passwordCharacter{"\xE2\x80\xA2"};

    std::function<void(const std::string&)> m_onSubmit;

    State<int> m_contentX{0};
    State<bool> m_acceptableInput{true};

    bool m_passwordEditing{false};
    int m_scrollOffset{0};
};

}
