#include <tavoos/widget/templates/textareabase.h>

#include <algorithm>

namespace Tavoos {

TextAreaBase::TextAreaBase(Object* parent) : TextEditBase{parent} {
    m_model.setSingleLine(false);
    m_wrapMode.onChange([this](const WrapMode& mode) {
        m_textDisplay->wrapMode(mode);
        m_placeholderDisplay->wrapMode(mode);
        onDisplayChanged();
    });
    m_tabStopDistance.onChange([this](const float& distance) {
        m_textDisplay->tabStopDistance(distance);
        m_placeholderDisplay->tabStopDistance(distance);
        onDisplayChanged();
    });
}

Widget::Size TextAreaBase::computeIntrinsicSize() {
    const Size text = m_textDisplay->intrinsicSize();
    const Size placeholder = m_placeholderDisplay->intrinsicSize();
    const float contentWidth = std::max(text.width + static_cast<float>(CaretWidth), placeholder.width)
                             + innerPaddingLeft() + innerPaddingRight() + paddingLeft() + paddingRight();
    const float contentHeight = std::max(text.height, placeholder.height)
                              + innerPaddingTop() + innerPaddingBottom() + paddingTop() + paddingBottom();
    return { width() > 0 ? static_cast<float>(width()) : contentWidth,
             height() > 0 ? static_cast<float>(height()) : contentHeight };
}

void TextAreaBase::onResolvedSizeChanged() {
    applyWrapWidth();
    TextEditBase::onResolvedSizeChanged();
}

void TextAreaBase::onDisplayChanged() {
    applyWrapWidth();
    requestRelayout();
}

void TextAreaBase::onDisplayResized() {
    updateCaret();
}

void TextAreaBase::applyWrapWidth() {
    const float available = resolvedWidth() - paddingLeft() - paddingRight()
                          - innerPaddingLeft() - innerPaddingRight() - static_cast<float>(CaretWidth);
    const int wrapWidth = m_wrapMode.get() == WrapMode::NoWrap || resolvedWidth() <= 0.0f
                        ? 0 : std::max(1, static_cast<int>(available));
    if (m_textDisplay->width() == wrapWidth)
        return;
    m_textDisplay->width(wrapWidth);
    m_placeholderDisplay->width(wrapWidth);
    requestRelayout();
}

}
