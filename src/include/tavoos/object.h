#pragma once

#include <tavoos/export.hpp>

#include <memory>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT Object {
    friend class Widget;
    friend class Builder;

public:
    Object(Object* = nullptr);
    virtual ~Object() {};

    Object* parent() const { return m_parent; }
    const std::vector<std::unique_ptr<Object>>& children() const { return m_childrens; }

protected:
    void clearChildren() { m_childrens.clear(); }

private:
    virtual void appendChild(std::unique_ptr<Object>& child);
    virtual std::unique_ptr<Object> detachChild(Object* child);

    Object* m_parent{nullptr};
    std::vector<std::unique_ptr<Object>> m_childrens;
};

}