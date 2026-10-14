#include <tavoos/effects/dropshadow.h>
#include <tavoos/gfx/renderer.h>

namespace Tavoos {

DropShadow::DropShadow(Widget& owner) : GraphicalEffect{owner} {
    bindInvalidateTriggers(m_offsetX, m_offsetY, m_radius, m_spread, m_color);
}

void DropShadow::renderBehind(Renderer& renderer, Widget& widget) {
    renderer.renderDropShadow(widget, *this);
}

}
