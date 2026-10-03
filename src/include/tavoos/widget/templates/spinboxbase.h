#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/control.h>

#include <functional>
#include <string>

namespace Tavoos {

class TAVOOS_EXPORT SpinBoxBase : public Control {
public:
    SpinBoxBase(Object* parent);

    template<typename W = RectangleWidget>
    decltype(auto) up(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installUp<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) down(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installDown<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) value(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_value);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) minValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_minValue);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) maxValue(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_maxValue);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) step(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_step);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onValueChange(this auto&& self, std::function<void(int)> callback) {
        self.m_onValueChange = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    int value() const { return m_value; }
    int minValue() const { return m_minValue; }
    int maxValue() const { return m_maxValue; }
    int step() const { return m_step; }
    const std::string& valueText() const { return m_valueText.get(); }

    State<int>& valueState() { return m_valueState; }
    State<std::string>& valueTextState() { return m_valueText; }

    void increase();
    void decrease();
    void commitText(const std::string& text);

private:
    template<typename W>
    void installUp(std::function<void(W&)> body) {
        if (m_up) {
            removeChild(m_up);
            m_up = nullptr;
        }
        m_up = addChild<W>([this, &body](W& slot) {
            if (body)
                body(slot);
            slot.onClick([this](MouseEvent&) { increase(); });
        });
    }

    template<typename W>
    void installDown(std::function<void(W&)> body) {
        if (m_down) {
            removeChild(m_down);
            m_down = nullptr;
        }
        m_down = addChild<W>([this, &body](W& slot) {
            if (body)
                body(slot);
            slot.onClick([this](MouseEvent&) { decrease(); });
        });
    }

    void setValue(int newValue);
    void syncValue();

    Property<int> m_value{0};
    State<int> m_valueState{0};
    State<std::string> m_valueText{"0"};
    Property<int> m_minValue{0};
    Property<int> m_maxValue{100};
    Property<int> m_step{1};

    std::function<void(int)> m_onValueChange;

    Widget* m_up{nullptr};
    Widget* m_down{nullptr};
};

}
