#include <tavoos/widget/templates/spinboxbase.h>

#include <algorithm>
#include <charconv>

namespace Tavoos {

SpinBoxBase::SpinBoxBase(Object* parent) : Control{parent} {
    m_value.onChange([this](const int&) { syncValue(); });
    m_minValue.onChange([this](const int&) { syncValue(); });
    m_maxValue.onChange([this](const int&) { syncValue(); });
    syncValue();
}

void SpinBoxBase::increase() {
    if (enabled())
        setValue(m_value.get() + m_step.get());
}

void SpinBoxBase::decrease() {
    if (enabled())
        setValue(m_value.get() - m_step.get());
}

void SpinBoxBase::commitText(const std::string& text) {
    int parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    const bool valid = result.ec == std::errc{} && result.ptr == text.data() + text.size();
    if (!valid) {
        syncValue();
        return;
    }
    setValue(parsed);
}

void SpinBoxBase::setValue(int newValue) {
    const int clamped = std::clamp(newValue, m_minValue.get(), m_maxValue.get());
    if (clamped == m_value.get()) {
        syncValue();
        return;
    }
    m_value.set(clamped);
    if (m_onValueChange)
        m_onValueChange(clamped);
}

void SpinBoxBase::syncValue() {
    if (m_valueState.get() != m_value.get())
        m_valueState.set(m_value.get());
    m_valueText.set(std::to_string(std::clamp(m_value.get(), m_minValue.get(), m_maxValue.get())));
}

}
