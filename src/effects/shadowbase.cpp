#include <tavoos/effects/shadowbase.h>

namespace Tavoos {

ShadowBase::ShadowBase(Widget& owner) : GraphicalEffect{owner} {
    bindInvalidateTriggers(m_offsetX, m_offsetY, m_radius, m_spread, m_color);
    m_style.onChange([this](const ShadowStyle& value) { applyStyle(value); });
}

void ShadowBase::applyStyle(const ShadowStyle& value) {
    m_offsetX.set(value.offsetX);
    m_offsetY.set(value.offsetY);
    m_radius.set(value.radius);
    m_spread.set(value.spread);
    m_color.set(value.color);
}

}
