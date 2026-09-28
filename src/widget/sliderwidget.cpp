#include <tavoos/widget/sliderwidget.h>

#include <tavoos/application.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {
constexpr int kTrackHeight = 6;
constexpr int kThumbSize = 18;
}

SliderWidget::SliderWidget(Object* parent) : ButtonBase{parent} {
    m_trackColorOut.set(m_trackColor.get());
    m_fillColorOut.set(m_fillColor.get());
    m_thumbColorOut.set(m_thumbColor.get());

    m_value.onChange([this](const int& value) {
        if (m_valueState.get() != value)
            m_valueState.set(value);
        updateGeometry();
    });
    m_minValue.onChange([this](const int&) { updateGeometry(); });
    m_maxValue.onChange([this](const int&) { updateGeometry(); });
    widthProperty().onChange([this](const int&) { updateGeometry(); });

    enabledState().onChange([this](const bool&) { updateColor(true); });
    m_trackColor.onChange([this](const Paint&) { updateColor(false); });
    m_fillColor.onChange([this](const Paint&) { updateColor(false); });
    m_thumbColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledThumbColor.onChange([this](const Paint&) { updateColor(false); });

    width(200).height(kThumbSize);
    updateGeometry();

    background([this](RectangleWidget& track) {
        track.fill(Fill::Width)
            .height(kTrackHeight)
            .alignment(Alignment::CenterVertical)
            .radius(kTrackHeight / 2)
            .color(m_trackColorOut);
    });

    content<RectangleWidget>([this](RectangleWidget& fill) {
        fill.alignment(Alignment::Left | Alignment::CenterVertical)
            .height(kTrackHeight)
            .width(m_fillWidth)
            .radius(kTrackHeight / 2)
            .color(m_fillColorOut);
    });

    addChild<RectangleWidget>([this](RectangleWidget& thumb) {
        thumb.alignment(Alignment::CenterVertical)
            .width(kThumbSize)
            .height(kThumbSize)
            .radius(kThumbSize / 2)
            .color(m_thumbColorOut)
            .x(m_thumbX);

        thumb.onDragStart([this](DragEvent&) {
            if (enabled())
                m_dragStartValue = m_value.get();
        });
        thumb.onDragMove([this](DragEvent& e) {
            if (!enabled())
                return;
            const int minV = m_minValue.get();
            const int maxV = std::max(minV + 1, m_maxValue.get());
            const int thumbTravel = std::max(1, width() - kThumbSize);
            const float valuePerPixel = static_cast<float>(maxV - minV) / static_cast<float>(thumbTravel);
            const int newValue = m_dragStartValue + static_cast<int>(std::lround(e.totalDx() * valuePerPixel));
            value(std::clamp(newValue, minV, maxV));
        });
    });

    m_style.onChange([this](const SliderStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().slider);
}

void SliderWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

void SliderWidget::handleClick(MouseEvent& event) {
    if (enabled())
        value(valueFromPosition(event.x()));
    ButtonBase::handleClick(event);
}

int SliderWidget::valueFromPosition(float x) const {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int thumbTravel = std::max(1, width() - kThumbSize);
    const float fraction = std::clamp((x - kThumbSize / 2.0f) / static_cast<float>(thumbTravel), 0.0f, 1.0f);
    return minV + static_cast<int>(std::lround(fraction * static_cast<float>(maxV - minV)));
}

void SliderWidget::updateColor(bool animate) {
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;
    m_trackColorOut.animateTo(isEnabled ? m_trackColor.get() : m_disabledColor.get(), duration);
    m_fillColorOut.animateTo(isEnabled ? m_fillColor.get() : m_disabledColor.get(), duration);
    m_thumbColorOut.animateTo(isEnabled ? m_thumbColor.get() : m_disabledThumbColor.get(), duration);
}

void SliderWidget::updateGeometry() {
    const int minV = m_minValue.get();
    const int maxV = std::max(minV + 1, m_maxValue.get());
    const int val = std::clamp(m_value.get(), minV, maxV);
    const float fraction = static_cast<float>(val - minV) / static_cast<float>(maxV - minV);

    const int thumbTravel = std::max(0, width() - kThumbSize);
    const int thumbX = static_cast<int>(std::round(fraction * static_cast<float>(thumbTravel)));
    if (m_thumbX.get() != thumbX)
        m_thumbX.set(thumbX);

    const int fillW = thumbX + kThumbSize / 2;
    if (m_fillWidth.get() != fillW)
        m_fillWidth.set(fillW);
}

void SliderWidget::applyStyle(const SliderStyle& value) {
    trackColor(value.trackColor);
    fillColor(value.fillColor);
    thumbColor(value.thumbColor);
    disabledColor(value.disabledColor);
    disabledThumbColor(value.disabledThumbColor);
    transition(value.transition);
}

}
