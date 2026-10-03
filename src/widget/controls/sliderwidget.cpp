#include <tavoos/widget/controls/sliderwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

namespace {
constexpr int kTrackHeight = 6;
constexpr int kThumbSize = 18;
}

SliderWidget::SliderWidget(Object* parent) : SliderBase{parent} {
    m_trackColorOut.set(m_trackColor.get());
    m_fillColorOut.set(m_fillColor.get());
    m_thumbColorOut.set(m_thumbColor.get());

    enabledState().onChange([this](const bool&) { updateColor(true); });
    m_trackColor.onChange([this](const Paint&) { updateColor(false); });
    m_fillColor.onChange([this](const Paint&) { updateColor(false); });
    m_thumbColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledThumbColor.onChange([this](const Paint&) { updateColor(false); });

    width(200).height(kThumbSize);

    background([this](RectangleWidget& track) {
        track.fill(Fill::Width)
            .height(kTrackHeight)
            .marginLeft(kThumbSize / 2)
            .marginRight(kThumbSize / 2)
            .alignment(Alignment::CenterVertical)
            .radius(kTrackHeight / 2)
            .color(m_trackColorOut);

        track.addChild<RectangleWidget>([this](RectangleWidget& fill) {
            fill.fill(Fill::Height)
                .alignment(Alignment::Left | Alignment::CenterVertical)
                .widthFraction(positionState())
                .radius(kTrackHeight / 2)
                .color(m_fillColorOut);
        });
    });

    handle<RectangleWidget>([this](RectangleWidget& thumb) {
        thumb.alignment(Alignment::CenterVertical)
            .xFraction(positionState())
            .width(kThumbSize)
            .height(kThumbSize)
            .radius(kThumbSize / 2)
            .color(m_thumbColorOut);
    });

    m_style.onChange([this](const SliderStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().slider);
}

void SliderWidget::render(Renderer& renderer) {
    m_settled = true;
    SliderBase::render(renderer);
}

void SliderWidget::updateColor(bool animate) {
    const bool isEnabled = enabled();
    const float duration = (animate && m_settled) ? m_transition.get() : 0.0f;
    m_trackColorOut.animateTo(isEnabled ? m_trackColor.get() : m_disabledColor.get(), duration);
    m_fillColorOut.animateTo(isEnabled ? m_fillColor.get() : m_disabledColor.get(), duration);
    m_thumbColorOut.animateTo(isEnabled ? m_thumbColor.get() : m_disabledThumbColor.get(), duration);
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
