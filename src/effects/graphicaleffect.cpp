#include <tavoos/effects/graphicaleffect.h>
#include <tavoos/widget/widget.h>

namespace Tavoos {

GraphicalEffect::GraphicalEffect(Widget& owner) : m_owner{&owner} {}

GraphicalEffect::~GraphicalEffect() = default;

void GraphicalEffect::invalidate() {
    m_owner->requestRepaint();
}

}
