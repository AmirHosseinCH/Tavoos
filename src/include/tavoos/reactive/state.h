#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
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

        const auto callbacksSnapshot = m_callbacks;
        for (const auto& [id, callback] : callbacksSnapshot) {
            const bool stillRegistered = std::any_of(m_callbacks.begin(), m_callbacks.end(),
                                                     [id](const auto& entry) { return entry.first == id; });
            if (stillRegistered)
                callback(m_value);
        }
    }

    std::size_t onChange(std::function<void(const T&)> callback) {
        const std::size_t id = m_nextCallbackId++;
        m_callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void removeOnChange(std::size_t id) {
        std::erase_if(m_callbacks, [id](const auto& entry) { return entry.first == id; });
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
    std::vector<std::pair<std::size_t, std::function<void(const T&)>>> m_callbacks;
    std::size_t m_nextCallbackId{1};

    void registerObserver(Property<T>* observer) {
        m_observers.push_back(observer);
    }

    void unregisterObserver(Property<T>* observer) {
        std::erase(m_observers, observer);
    }
};

}