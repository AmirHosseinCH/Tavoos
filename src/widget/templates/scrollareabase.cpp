#include <tavoos/widget/templates/scrollareabase.h>

#include <algorithm>

namespace Tavoos {

ScrollAreaBase::ScrollAreaBase(Object* parent) : FlickAreaBase{parent} {
    dragScroll(false);
    keyNavigation(false);

    auto resync = [this](const auto&) {
        syncBars();
        wake();
    };
    m_verticalPolicy.onChange(resync);
    m_horizontalPolicy.onChange(resync);
    m_autoHide.onChange(resync);
    m_minThumbSize.onChange(resync);
    hoveredState().onChange([this](const bool&) { wake(); });
}

ScrollAreaBase::~ScrollAreaBase() {
    if (m_animating)
        AnimationManager::instance().unregisterAnimation(this);
}

bool ScrollAreaBase::isContentChild(const Widget& child) const {
    if (&child == m_verticalTrack || &child == m_verticalThumb ||
        &child == m_horizontalTrack || &child == m_horizontalThumb)
        return false;
    return FlickAreaBase::isContentChild(child);
}

bool ScrollAreaBase::fades(BarPolicy policy) const {
    return m_autoHide.get() && policy == BarPolicy::Auto;
}

void ScrollAreaBase::onOffsetsSynced(bool offsetsChanged) {
    syncBars();
    if (offsetsChanged)
        wake();
}

void ScrollAreaBase::syncBars() {
    const FlickDirection direction = flickDirection();
    const float maxX = maxContentX();
    const float maxY = maxContentY();
    const float viewportW = viewportWidth();
    const float viewportH = viewportHeight();
    const float minSize = m_minThumbSize.get();

    auto sizeFraction = [minSize](float viewport, float content) {
        if (viewport <= 0.0f || content <= 0.0f)
            return 1.0f;
        return std::clamp(std::max(viewport / content, minSize / viewport), 0.0f, 1.0f);
    };

    m_verticalThumbSize.setIfChanged(sizeFraction(viewportH, contentHeight()));
    m_horizontalThumbSize.setIfChanged(sizeFraction(viewportW, contentWidth()));
    m_verticalThumbPosition.setIfChanged(maxY > 0.0f ? contentY() / maxY : 0.0f);
    m_horizontalThumbPosition.setIfChanged(maxX > 0.0f ? contentX() / maxX : 0.0f);

    const BarPolicy vertical = m_verticalPolicy.get();
    const BarPolicy horizontal = m_horizontalPolicy.get();
    m_verticalWanted = hasFlag(direction, FlickDirection::Vertical) &&
                       (vertical == BarPolicy::Always || (vertical == BarPolicy::Auto && maxY > 0.0f));
    m_horizontalWanted = hasFlag(direction, FlickDirection::Horizontal) &&
                         (horizontal == BarPolicy::Always || (horizontal == BarPolicy::Auto && maxX > 0.0f));

    if (!fades(vertical))
        m_verticalOpacity.setIfChanged(1.0f);
    if (!fades(horizontal))
        m_horizontalOpacity.setIfChanged(1.0f);
    updateShown();
}

void ScrollAreaBase::updateShown() {
    m_verticalShown.setIfChanged(m_verticalWanted && m_verticalOpacity.get() > 0.001f);
    m_horizontalShown.setIfChanged(m_horizontalWanted && m_horizontalOpacity.get() > 0.001f);
}

void ScrollAreaBase::wake() {
    m_idle = 0.0f;
    const bool anyFades = fades(m_verticalPolicy.get()) || fades(m_horizontalPolicy.get());
    if (anyFades && !m_animating) {
        m_animating = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

bool ScrollAreaBase::tick(float dt) {
    m_idle += dt;
    if (hovered() || m_verticalThumbPressed.get() || m_horizontalThumbPressed.get())
        m_idle = 0.0f;

    const bool linger = m_idle < m_hideDelay.get();
    const float fade = m_fadeDuration.get();
    const float step = fade > 0.0f ? dt / fade : 1.0f;
    bool active = false;

    auto advance = [&](State<float>& opacity, BarPolicy policy, bool wanted) {
        if (!fades(policy))
            return;
        const float target = (wanted && linger) ? 1.0f : 0.0f;
        float value = opacity.get();
        if (value < target)
            value = std::min(target, value + step);
        else if (value > target)
            value = std::max(target, value - step);
        opacity.setIfChanged(value);
        if (value != target || (wanted && linger))
            active = true;
    };
    advance(m_verticalOpacity, m_verticalPolicy.get(), m_verticalWanted);
    advance(m_horizontalOpacity, m_horizontalPolicy.get(), m_horizontalWanted);
    updateShown();

    if (!active)
        m_animating = false;
    return active;
}

ScrollAreaBase::BarHit ScrollAreaBase::hitBar(const Point& point) const {
    auto inside = [&point](const Widget* w) {
        return w && w->visible() &&
               point.x >= w->resolvedX() && point.x <= w->resolvedX() + w->displayedWidth() &&
               point.y >= w->resolvedY() && point.y <= w->resolvedY() + w->displayedHeight();
    };
    if (m_verticalShown.get()) {
        if (inside(m_verticalThumb))
            return BarHit::VerticalThumb;
        if (inside(m_verticalTrack))
            return BarHit::VerticalTrack;
    }
    if (m_horizontalShown.get()) {
        if (inside(m_horizontalThumb))
            return BarHit::HorizontalThumb;
        if (inside(m_horizontalTrack))
            return BarHit::HorizontalTrack;
    }
    return BarHit::None;
}

void ScrollAreaBase::setPressed(Axis axis, bool pressed) {
    if (axis == Axis::Vertical)
        m_verticalThumbPressed.setIfChanged(pressed);
    else if (axis == Axis::Horizontal)
        m_horizontalThumbPressed.setIfChanged(pressed);
    wake();
}

bool ScrollAreaBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseMove:
        return m_verticalThumb || m_verticalTrack || m_horizontalThumb || m_horizontalTrack;
    case EventType::MouseRelease:
        return m_barDrag != Axis::None || m_verticalThumbPressed.get() || m_horizontalThumbPressed.get();
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
        return m_barDrag != Axis::None || FlickAreaBase::hasHandlerFor(type);
    default:
        return FlickAreaBase::hasHandlerFor(type);
    }
}

void ScrollAreaBase::triggerPress(MouseEvent& event) {
    const Point point{event.x(), event.y()};
    const BarHit hit = event.button() == MouseButton::Left ? hitBar(point) : BarHit::None;

    switch (hit) {
    case BarHit::VerticalThumb:
        m_barDrag = Axis::Vertical;
        setPressed(Axis::Vertical, true);
        break;
    case BarHit::HorizontalThumb:
        m_barDrag = Axis::Horizontal;
        setPressed(Axis::Horizontal, true);
        break;
    case BarHit::VerticalTrack: {
        const float thumbTop = m_verticalThumb ? m_verticalThumb->resolvedY() : point.y;
        scrollBy(0.0f, point.y < thumbTop ? -viewportHeight() : viewportHeight());
        break;
    }
    case BarHit::HorizontalTrack: {
        const float thumbLeft = m_horizontalThumb ? m_horizontalThumb->resolvedX() : point.x;
        scrollBy(point.x < thumbLeft ? -viewportWidth() : viewportWidth(), 0.0f);
        break;
    }
    case BarHit::None:
        event.ignore();
        break;
    }
    Widget::triggerPress(event);
}

void ScrollAreaBase::triggerRelease(MouseEvent& event) {
    setPressed(Axis::Vertical, false);
    setPressed(Axis::Horizontal, false);
    m_barDrag = Axis::None;
    Widget::triggerRelease(event);
}

void ScrollAreaBase::triggerMouseMove(MouseEvent& event) {
    const BarHit hit = hitBar(Point{event.x(), event.y()});
    m_verticalThumbHovered.setIfChanged(hit == BarHit::VerticalThumb);
    m_horizontalThumbHovered.setIfChanged(hit == BarHit::HorizontalThumb);
    event.ignore();
    Widget::triggerMouseMove(event);
}

void ScrollAreaBase::triggerMouseLeave(MouseEvent& event) {
    m_verticalThumbHovered.setIfChanged(false);
    m_horizontalThumbHovered.setIfChanged(false);
    FlickAreaBase::triggerMouseLeave(event);
}

void ScrollAreaBase::triggerDragMove(DragEvent& event) {
    if (m_barDrag == Axis::Vertical) {
        const float trackLength = m_verticalTrack ? m_verticalTrack->displayedHeight() : viewportHeight();
        const float thumbLength = m_verticalThumb ? m_verticalThumb->displayedHeight() : 0.0f;
        const float free = std::max(1.0f, trackLength - thumbLength);
        scrollBy(0.0f, event.dy() * maxContentY() / free);
        Widget::triggerDragMove(event);
        return;
    }
    if (m_barDrag == Axis::Horizontal) {
        const float trackLength = m_horizontalTrack ? m_horizontalTrack->displayedWidth() : viewportWidth();
        const float thumbLength = m_horizontalThumb ? m_horizontalThumb->displayedWidth() : 0.0f;
        const float free = std::max(1.0f, trackLength - thumbLength);
        scrollBy(event.dx() * maxContentX() / free, 0.0f);
        Widget::triggerDragMove(event);
        return;
    }
    FlickAreaBase::triggerDragMove(event);
}

void ScrollAreaBase::triggerDragEnd(DragEvent& event) {
    m_barDrag = Axis::None;
    setPressed(Axis::Vertical, false);
    setPressed(Axis::Horizontal, false);
    Widget::triggerDragEnd(event);
}

}
