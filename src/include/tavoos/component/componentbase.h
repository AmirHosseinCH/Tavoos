#pragma once

#include <tavoos/export.hpp>

namespace Tavoos {

class Builder;
class Widget;

class TAVOOS_EXPORT ComponentBase {
public:
    virtual ~ComponentBase() = default;

protected:
    virtual void build() {}

private:
    friend class Builder;
    friend class Widget;

    void runBuild();
};

}
