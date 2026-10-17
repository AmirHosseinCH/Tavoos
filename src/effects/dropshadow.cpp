#include <tavoos/effects/dropshadow.h>
#include <tavoos/gfx/renderer.h>

namespace Tavoos {

DropShadow::DropShadow(Widget& owner) : ShadowBase{owner} {}

void DropShadow::renderBehind(Renderer& renderer, Widget& widget) {
    renderer.renderDropShadow(widget, *this);
}

}
