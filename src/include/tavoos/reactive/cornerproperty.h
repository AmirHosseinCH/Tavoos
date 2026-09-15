#pragma once
#include <tavoos/reactive/property.h>

namespace Tavoos {

template<typename T>
class CornerProperty {
public:
    CornerProperty() = default;
    explicit CornerProperty(const T& uniform)
        : m_topLeft{uniform}, m_topRight{uniform}, m_bottomRight{uniform}, m_bottomLeft{uniform} {}

    Property<T>& topLeftProperty()     { return m_topLeft; }
    Property<T>& topRightProperty()    { return m_topRight; }
    Property<T>& bottomRightProperty() { return m_bottomRight; }
    Property<T>& bottomLeftProperty()  { return m_bottomLeft; }

    T topLeft()     const { return m_topLeft; }
    T topRight()    const { return m_topRight; }
    T bottomRight() const { return m_bottomRight; }
    T bottomLeft()  const { return m_bottomLeft; }

private:
    Property<T> m_topLeft{}, m_topRight{}, m_bottomRight{}, m_bottomLeft{};
};

}
