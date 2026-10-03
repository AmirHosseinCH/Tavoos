#include <tavoos/widget/templates/progressbarbase.h>

#include <tavoos/widget/templates/rangemath.h>

namespace Tavoos {

ProgressBarBase::ProgressBarBase(Object* parent) : Control{parent} {
    m_value.onChange([this](const int& value) {
        if (m_valueState.get() != value)
            m_valueState.set(value);
        updatePosition();
    });
    m_minValue.onChange([this](const int&) { updatePosition(); });
    m_maxValue.onChange([this](const int&) { updatePosition(); });
    updatePosition();
}

void ProgressBarBase::updatePosition() {
    const float fraction = detail::fractionInRange(m_value.get(), m_minValue.get(), m_maxValue.get());

    if (m_position.get() != fraction)
        m_position.set(fraction);
}

}
