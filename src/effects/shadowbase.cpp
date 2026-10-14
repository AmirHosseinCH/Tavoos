#include <tavoos/effects/shadowbase.h>

namespace Tavoos {

ShadowBase::ShadowBase(Widget& owner) : GraphicalEffect{owner} {
    bindInvalidateTriggers(m_offsetX, m_offsetY, m_radius, m_spread, m_color);
}

}
