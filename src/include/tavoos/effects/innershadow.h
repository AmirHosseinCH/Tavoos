#pragma once

#include <tavoos/effects/shadowbase.h>
#include <tavoos/export.hpp>

namespace Tavoos {

class TAVOOS_EXPORT InnerShadow : public ShadowBase {
public:
    explicit InnerShadow(Widget& owner);

protected:
    void renderInside(Renderer& renderer, Widget& widget) override;
};

}
