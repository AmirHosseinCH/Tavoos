#include <tavoos/widget/rectangle.h>
#include <tavoos/gfx/renderer.h>

namespace Tavoos {

RectangleWidget::RectangleWidget(Object* parent) : Widget{parent} {
    bindRepaintTriggers(m_color, m_borderWidth, m_borderColor,
                         m_radius.topLeftProperty(), m_radius.topRightProperty(),
                         m_radius.bottomRightProperty(), m_radius.bottomLeftProperty());
}

void RectangleWidget::render(Renderer& r) {
    r.renderRectangle(*this);
    renderChildren(r);
}

}