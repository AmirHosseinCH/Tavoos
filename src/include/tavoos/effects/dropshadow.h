#pragma once

#include <tavoos/effects/shadowbase.h>
#include <tavoos/export.hpp>

namespace Tavoos {

class TAVOOS_EXPORT DropShadow : public ShadowBase {
public:
    explicit DropShadow(Widget& owner);

protected:
    void renderBehind(Renderer& renderer, Widget& widget) override;
};

}
