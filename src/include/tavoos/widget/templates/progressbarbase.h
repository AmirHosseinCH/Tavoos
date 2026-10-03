#pragma once

#include <tavoos/export.hpp>
#include <tavoos/widget/templates/rangebase.h>

namespace Tavoos {

class TAVOOS_EXPORT ProgressBarBase : public RangeBase {
public:
    ProgressBarBase(Object* parent);
};

}
