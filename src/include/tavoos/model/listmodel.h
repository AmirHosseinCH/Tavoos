#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

namespace Tavoos {

struct ListChange {
    enum class Kind { Inserted, Removed, Changed, Moved, Reset, Destroyed };

    Kind kind{Kind::Reset};
    int index{0};
    int count{0};
    int to{0};
};

class ListModelBase {
public:
    ListModelBase() = default;
    ListModelBase(const ListModelBase&) = delete;
    ListModelBase(ListModelBase&&) = delete;
    ListModelBase& operator=(const ListModelBase&) = delete;
    ListModelBase& operator=(ListModelBase&&) = delete;
    virtual ~ListModelBase() { notify({ListChange::Kind::Destroyed}); }

    virtual int size() const = 0;
    bool empty() const { return size() == 0; }

    std::size_t onChange(std::function<void(const ListChange&)> callback) {
        const std::size_t id = m_nextCallbackId++;
        m_callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void removeOnChange(std::size_t id) {
        std::erase_if(m_callbacks, [id](const auto& entry) { return entry.first == id; });
    }

protected:
    void notify(const ListChange& change) {
        const auto snapshot = m_callbacks;
        for (const auto& [id, callback] : snapshot) {
            const bool stillRegistered = std::ranges::any_of(m_callbacks, [id](const auto& entry) { return entry.first == id; });
            if (stillRegistered)
                callback(change);
        }
    }

private:
    std::vector<std::pair<std::size_t, std::function<void(const ListChange&)>>> m_callbacks;
    std::size_t m_nextCallbackId{1};
};

template <typename T>
class ListModel : public ListModelBase {
public:
    ListModel() = default;
    ListModel(std::initializer_list<T> items) : m_items(items) {}
    explicit ListModel(std::vector<T> items) : m_items(std::move(items)) {}

    int size() const override { return static_cast<int>(m_items.size()); }
    const T& operator[](int index) const { return m_items[static_cast<std::size_t>(index)]; }
    const T& at(int index) const { return m_items.at(static_cast<std::size_t>(index)); }
    const std::vector<T>& items() const noexcept { return m_items; }

    void append(T value) { insert(size(), std::move(value)); }

    void insert(int index, T value) {
        index = std::clamp(index, 0, size());
        m_items.insert(m_items.begin() + index, std::move(value));
        notify({ListChange::Kind::Inserted, index, 1});
    }

    void remove(int index, int count = 1) {
        const int first = std::clamp(index, 0, size());
        const int last = std::clamp(index + count, first, size());
        if (first == last)
            return;
        m_items.erase(m_items.begin() + first, m_items.begin() + last);
        notify({ListChange::Kind::Removed, first, last - first});
    }

    void set(int index, T value) {
        if (index < 0 || index >= size())
            return;
        m_items[static_cast<std::size_t>(index)] = std::move(value);
        notify({ListChange::Kind::Changed, index, 1});
    }

    template <typename Edit>
    void update(int index, Edit&& edit) {
        if (index < 0 || index >= size())
            return;
        std::forward<Edit>(edit)(m_items[static_cast<std::size_t>(index)]);
        notify({ListChange::Kind::Changed, index, 1});
    }

    void move(int from, int to) {
        if (from < 0 || from >= size())
            return;
        to = std::clamp(to, 0, size() - 1);
        if (from == to)
            return;
        const auto begin = m_items.begin();
        if (from < to)
            std::rotate(begin + from, begin + from + 1, begin + to + 1);
        else
            std::rotate(begin + to, begin + from, begin + from + 1);
        notify({ListChange::Kind::Moved, from, 1, to});
    }

    void clear() { remove(0, size()); }

    void assign(std::vector<T> items) {
        m_items = std::move(items);
        notify({ListChange::Kind::Reset});
    }

private:
    std::vector<T> m_items;
};

}
