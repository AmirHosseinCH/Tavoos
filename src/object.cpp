#include <tavoos/object.h>

namespace Tavoos {

Object::Object(Object* parent) : m_parent{parent} {}

void Object::appendChild(std::unique_ptr<Object>& child) {
    m_childrens.push_back(std::move(child));
}

std::unique_ptr<Object> Object::detachChild(Object* child) {
    for (auto it = m_childrens.begin(); it != m_childrens.end(); ++it) {
        if (it->get() == child) {
            std::unique_ptr<Object> detached = std::move(*it);
            m_childrens.erase(it);
            return detached;
        }
    }
    return nullptr;
}

}