#pragma once

#include <tavoos/reactive/property.h>
#include <tavoos/reactive/state.h>

#include <concepts>
#include <utility>

namespace Tavoos {

template <typename T>
class PropertyArg {
public:
    PropertyArg(const T& value) : m_value{value} {}
    PropertyArg(T&& value) : m_value{std::move(value)} {}
    PropertyArg(State<T>& state) : m_state{&state}, m_isBind{true} {}
    template <typename U>
        requires std::convertible_to<U, T> && (!std::derived_from<std::remove_cvref_t<U>, State<T>>)
    PropertyArg(U&& value) : m_value(std::forward<U>(value)) {}

    void applyTo(Property<T>& property) {
        if (m_isBind)
            property.bind(*m_state);
        else
            property.set(m_value);
    }

private:
    T m_value{};
    State<T>* m_state{nullptr};
    bool m_isBind{false};
};

}