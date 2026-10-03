#include <tavoos/widget/templates/rangebase.h>

#include <tavoos/widget/templates/rangemath.h>

namespace Tavoos {

RangeBase::RangeBase(Object* parent) : Control{parent} {
    m_value.onChange([this](const int&) { syncRange(); });
    m_minValue.onChange([this](const int&) { syncRange(); });
    m_maxValue.onChange([this](const int&) { syncRange(); });
    syncRange();
}

bool RangeBase::commitValue(long long newValue) {
    const int clamped = static_cast<int>(detail::clampToRange(newValue, m_minValue.get(), m_maxValue.get()));
    if (clamped == m_value.get())
        return false;
    m_value.set(clamped);
    return true;
}

void RangeBase::syncRange() {
    m_valueState.setIfChanged(m_value.get());
    m_position.setIfChanged(detail::fractionInRange(m_value.get(), m_minValue.get(), m_maxValue.get()));
    onRangeChanged();
}

}
