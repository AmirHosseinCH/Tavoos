#pragma once

#include <tavoos/reactive/property.h>
#include <tavoos/reactive/propertyarg.h>
#include <tavoos/reactive/state.h>

#include <cstddef>
#include <functional>

namespace Tavoos {

template <typename T>
class BindableState {
public:
    BindableState() {
        connect();
    }

    explicit BindableState(const T& value) : m_input{value}, m_output{value} {
        connect();
    }

    BindableState(const BindableState&) = delete;
    BindableState(BindableState&&) = delete;
    BindableState& operator=(const BindableState&) = delete;
    BindableState& operator=(BindableState&&) = delete;

    void set(PropertyArg<T> value) { value.applyTo(m_input); }

    const T& get() const noexcept { return m_output.get(); }
    operator const T&() const noexcept { return m_output.get(); }

    State<T>& state() noexcept { return m_output; }

    std::size_t onChange(std::function<void(const T&)> callback) { return m_output.onChange(std::move(callback)); }
    void removeOnChange(std::size_t id) { m_output.removeOnChange(id); }

private:
    void connect() {
        m_input.onChange([this](const T& value) { m_output.set(value); });
    }

    Property<T> m_input;
    State<T> m_output;
};

}
