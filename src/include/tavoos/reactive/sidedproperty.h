#pragma once
#include <tavoos/reactive/property.h>

namespace Tavoos {

template<typename T>
class SidedProperty {
public:
    SidedProperty() = default;
    explicit SidedProperty(const T& uniform) : m_left{uniform}, m_top{uniform}, m_right{uniform}, m_bottom{uniform} {}

    Property<T>& leftProperty()   { return m_left; }
    Property<T>& topProperty()    { return m_top; }
    Property<T>& rightProperty()  { return m_right; }
    Property<T>& bottomProperty() { return m_bottom; }

    T left()   const { return m_left; }
    T top()    const { return m_top; }
    T right()  const { return m_right; }
    T bottom() const { return m_bottom; }

private:
    Property<T> m_left{}, m_top{}, m_right{}, m_bottom{};
};

}