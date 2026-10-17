#include <tavoos/effects/innershadow.h>
#include <tavoos/gfx/renderer.h>

namespace Tavoos {

InnerShadow::InnerShadow(Widget& owner) : ShadowBase{owner} {}

void InnerShadow::renderInside(Renderer& renderer, Widget& widget) {
    renderer.renderInnerShadow(widget, *this);
}

}
