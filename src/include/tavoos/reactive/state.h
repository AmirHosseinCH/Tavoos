#pragma once

#include <algorithm>
#include <utility>
#include <vector>

namespace Tavoos {

template <typename T> class Property;

template <typename T>
class State {
    friend class Property<T>;

public:
    State() = default;
    State(const State&) = delete;
    State(State&&) = delete;
    State& operator=(const State&) = delete;
    State& operator=(State&&) = delete;

    State(const T& value) : m_value{value} {}
    State(T&& value) : m_value{std::move(value)} {}
    ~State() {
        auto observers = std::move(m_observers);
        for (const auto observer : observers) {
            observer->unbind(true);
        }
    }

    void notifyObservers() {
        const auto observersSnapshot = m_observers;
        for (const auto observer : observersSnapshot) {
            if (std::find(m_observers.begin(), m_observers.end(), observer) != m_observers.end())
                observer->notifyChange();
        }
    }

    void set(const T& value) {
        m_value = value;
        notifyObservers();
    }

    void set(T&& value) {
        m_value = std::move(value);
        notifyObservers();
    }

    const T& get() const noexcept { return m_value; }
    operator const T&() const noexcept { return m_value; }

private:
    T m_value{};
    std::vector<Property<T>*> m_observers;

    void registerObserver(Property<T>* observer) {
        m_observers.push_back(observer);
    }

    void unregisterObserver(Property<T>* observer) {
        std::erase(m_observers, observer);
    }
};

}