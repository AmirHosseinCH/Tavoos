#pragma once

#include <tavoos/componentbase.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

#include <concepts>
#include <functional>
#include <utility>

namespace Tavoos {

template<typename Root>
    requires std::derived_from<Root, Widget>
class Component : public Root, public ComponentBase {
public:
    using Root::Root;
};

}

#define TAVOOS_PROPERTY(Type, name, defaultValue)                                      \
private:                                                                               \
    ::Tavoos::BindableState<Type> m_##name{defaultValue};                              \
public:                                                                                \
    decltype(auto) name(this auto&& self, ::Tavoos::PropertyArg<Type> value) {         \
        self.m_##name.set(value);                                                      \
        return std::forward<decltype(self)>(self);                                     \
    }                                                                                  \
    const Type& name() const { return m_##name.get(); }                                \
    ::Tavoos::State<Type>& name##State() { return m_##name.state(); }

#define TAVOOS_CALLBACK(name, ...)                                                     \
protected:                                                                             \
    std::function<__VA_ARGS__> m_##name;                                               \
public:                                                                                \
    decltype(auto) name(this auto&& self, std::function<__VA_ARGS__> callback) {       \
        self.m_##name = std::move(callback);                                           \
        return std::forward<decltype(self)>(self);                                     \
    }
