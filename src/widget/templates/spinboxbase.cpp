#include <tavoos/widget/templates/spinboxbase.h>

#include <tavoos/widget/templates/rangemath.h>

#include <charconv>

namespace Tavoos {

SpinBoxBase::SpinBoxBase(Object* parent) : RangeBase{parent} {
    refreshValueText();
}

void SpinBoxBase::increase() {
    if (enabled())
        setValue(static_cast<long long>(value()) + m_step.get());
}

void SpinBoxBase::decrease() {
    if (enabled())
        setValue(static_cast<long long>(value()) - m_step.get());
}

void SpinBoxBase::commitText(const std::string& text) {
    int parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    const bool valid = result.ec == std::errc{} && result.ptr == text.data() + text.size();
    if (!valid) {
        refreshValueText();
        return;
    }
    setValue(parsed);
}

void SpinBoxBase::setValue(long long newValue) {
    if (!commitValue(newValue)) {
        refreshValueText();
        return;
    }
    if (m_onValueChange)
        m_onValueChange(value());
}

void SpinBoxBase::onRangeChanged() {
    refreshValueText();
}

void SpinBoxBase::refreshValueText() {
    m_valueText.set(std::to_string(detail::clampToRange(value(), minValue(), maxValue())));
}

}
