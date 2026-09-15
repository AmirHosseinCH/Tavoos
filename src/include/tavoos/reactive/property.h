#pragma once

#include <functional>
#include <utility>
#include <vector>

namespace Tavoos {

template <typename T> class State;

template <typename T>
class Property {
    friend class State<T>;

public:
    Property() = default;
    Property(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(const Property&) = delete;
    Property& operator=(Property&&) = delete;

    Property(const T& value) : m_value{value} {}
    Property(T&& value) : m_value{std::move(value)} {}
    Property(State<T>& state) {
        state.registerObserver(this);
        m_isBind = true;
        m_state = &state;
    }
    ~Property() {
        unbind();
    }

    void bind(State<T>& state) {
        unbind();
        state.registerObserver(this);
        m_isBind = true;
        m_state = &state;

        notifyChange();
    }

    void unbind(bool saveValue = false) {
        if (m_isBind) {

            if (saveValue)
                m_value = m_state->get();

            m_isBind = false;
            m_state->unregisterObserver(this);
            m_state = nullptr;
        }
    }

    void onChange(std::function<void(const T&)> callback) {
        m_onChangeCallbacks.push_back(std::move(callback));
    }

    void set(const T& value) {
        m_value = value;

        unbind();

        notifyChange();
    }

    void set(T&& value) {
        m_value = std::move(value);

        unbind();

        notifyChange();
    }

    const T& get() const noexcept {
        if (m_isBind)
            return m_state->get();
        else
            return m_value;
    }
    operator const T&() const noexcept {
        if (m_isBind)
            return m_state->get();
        else
            return m_value;
    }

private:
    T m_value{};
    State<T>* m_state{nullptr};
    bool m_isBind{false};
    std::vector<std::function<void(const T&)>> m_onChangeCallbacks;

    void notifyChange() {
        for (const auto& callback : m_onChangeCallbacks) {
            if (m_isBind)
                callback(m_state->get());
            else
                callback(m_value);
        }
    }
};

}