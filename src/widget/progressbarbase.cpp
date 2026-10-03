#include <tavoos/widget/progressbarbase.h>

#include <algorithm>

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
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int val = std::clamp(m_value.get(), minV, maxV);
    const float fraction = static_cast<float>(val - minV) / static_cast<float>(maxV - minV);

    if (m_position.get() != fraction)
        m_position.set(fraction);
}

}
