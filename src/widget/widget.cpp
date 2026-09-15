#include <tavoos/gfx/renderer.h>
#include <tavoos/widget/widget.h>
#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

bool Widget::s_isBuilding = false;

Widget::~Widget() {
    if (m_clipMaskTexture != 0)
        glDeleteTextures(1, &m_clipMaskTexture);
    if (m_clipColorTexture != 0)
        glDeleteTextures(1, &m_clipColorTexture);
}

Widget::Widget(Object* parent) : Object{parent} {
    if (auto* parentWidget = dynamic_cast<Widget*>(parent))
        m_ownerWindow = parentWidget->m_ownerWindow;
    else if (auto* window = dynamic_cast<Window*>(parent))
        m_ownerWindow = window;

    bindRelayoutTriggers(m_x, m_y, m_width, m_height, m_alignment, m_fill,
                         m_padding.leftProperty(), m_padding.topProperty(),
                         m_padding.rightProperty(), m_padding.bottomProperty(),
                         m_margin.leftProperty(), m_margin.topProperty(),
                         m_margin.rightProperty(), m_margin.bottomProperty(),
                         m_rotation, m_scale, m_gridColumnSpan, m_gridRowSpan,
                         m_gridRow, m_gridColumn);

    m_opacity.onChange([this](const auto&) { updateEffectiveOpacity(); });
    bindRepaintTriggers(m_opacity, m_visible, m_clip);
}

void Widget::renderChildren(Renderer& renderer) {
    if (renderer.isCapturingMask())
        return;

    for (const auto& child : children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get())) {
            if (widget->visible())
                renderer.renderWidget(*widget);
        }
    }
}

bool Widget::parentIsLayouter() const {
    auto* const parentWidget = dynamic_cast<Widget*>(parent());
    return parentWidget && parentWidget->isLayouter();
}

bool Widget::hitTest(float px, float py) const {
    const glm::vec4 localPoint = glm::inverse(worldMatrix()) * glm::vec4{px, py, 0.0f, 1.0f};
    return localPoint.x >= 0.0f && localPoint.x <= m_displayedWidth &&
           localPoint.y >= 0.0f && localPoint.y <= m_displayedHeight;
}

Widget *Widget::hitTestTree(float px, float py) const {
    if (!visible())
        return nullptr;

    for (auto it = children().rbegin(); it != children().rend(); ++it) {
        if (auto* widget = dynamic_cast<Widget*>(it->get())) {
            if (auto* hit = widget->hitTestTree(px, py))
                return hit;
        }
    }

    if (hitTest(px, py))
        return const_cast<Widget*>(this);

    return nullptr;
}

bool Widget::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:       return static_cast<bool>(m_onPress);
    case EventType::MouseRelease:     return static_cast<bool>(m_onRelease);
    case EventType::MouseClick:       return static_cast<bool>(m_onClick);
    case EventType::MouseDoubleClick: return static_cast<bool>(m_onDoubleClick);
    case EventType::MouseMove:        return static_cast<bool>(m_onMouseMove);
    case EventType::MouseEnter:       return static_cast<bool>(m_onMouseEnter);
    case EventType::MouseLeave:       return static_cast<bool>(m_onMouseLeave);
    case EventType::Wheel:            return static_cast<bool>(m_onWheel);
    case EventType::KeyPress:         return static_cast<bool>(m_onKeyPress);
    case EventType::KeyRelease:       return static_cast<bool>(m_onKeyRelease);
    case EventType::TextInput:        return static_cast<bool>(m_onTextInput);
    case EventType::FocusIn:          return static_cast<bool>(m_onFocusIn);
    case EventType::FocusOut:         return static_cast<bool>(m_onFocusOut);
    case EventType::None:             return false;
    }
    return false;
}

void Widget::updateEffectiveOpacity() {
    float parentOpacity = 1.0f;
    if (auto* const parentWidget = dynamic_cast<Widget*>(parent()))
        parentOpacity = parentWidget->effectiveOpacity();

    m_effectiveOpacity = m_opacity.get() * parentOpacity;

    for (auto& child : children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->updateEffectiveOpacity();
    }
}

void Widget::markLayoutDirty() {
    m_layoutDirty = true;
    requestRepaint();
}

void Widget::requestRepaint() {
    if (m_ownerWindow)
        m_ownerWindow->markDirty();
}

void Widget::detachAndDefer(Widget* child) {
    Object* const parentObj = child->parent();
    if (!parentObj)
        return;

    std::unique_ptr<Object> detached = parentObj->detachChild(child);
    if (!detached)
        return;

    if (child->m_ownerWindow) {
        child->m_ownerWindow->clearReferencesTo(child);
        child->m_ownerWindow->markDirty();
        child->m_ownerWindow->deferDestruction(std::move(detached));
    }

    if (auto* const parentWidget = dynamic_cast<Widget*>(parentObj); parentWidget && parentWidget->isLayouter())
        parentWidget->requestRelayout();
}

void Widget::removeSelf() {
    detachAndDefer(this);
}

void Widget::removeChild(Widget* child) {
    if (child->parent() != this)
        return;
    detachAndDefer(child);
}

bool Widget::isSizeBoundary() const {
    const bool widthFixed  = hasFlag(m_fill.get(), Fill::Width)  || m_width.get()  > 0;
    const bool heightFixed = hasFlag(m_fill.get(), Fill::Height) || m_height.get() > 0;
    return widthFixed && heightFixed;
}

void Widget::requestRelayout() {
    if (s_isBuilding)
        return;

    markLayoutDirty();
    m_intrinsicSizeCacheValid = false;

    auto* const parentWidget = dynamic_cast<Widget*>(parent());
    if (!parentWidget || !parentWidget->isLayouter())
        return;

    if (parentWidget->isSizeBoundary())
        parentWidget->markLayoutDirty();
    else
        parentWidget->requestRelayout();
}

bool Widget::consumeLayoutDirty() {
    if (m_layoutDirty) {
        m_layoutDirty = false;
        return true;
    }
    return false;
}

Widget::ContentArea Widget::resolveContentArea() {
    ContentArea area{0, 0, 0, 0};
    if (auto* const parentWidget = dynamic_cast<Widget*>(parent())) {
        area.x = parentWidget->paddingLeft();
        area.y = parentWidget->paddingTop();
        area.width  = parentWidget->displayedWidth()  - parentWidget->paddingLeft() - parentWidget->paddingRight();
        area.height = parentWidget->displayedHeight() - parentWidget->paddingTop()  - parentWidget->paddingBottom();
    } else if (auto* const window = dynamic_cast<Window*>(parent())) {
        area.width  = static_cast<float>(window->width());
        area.height = static_cast<float>(window->height());
    }
    return area;
}

void Widget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    if (parentIsLayouter()) {
        syncDisplayedGeometry();
        updateWorldMatrix();
        for (auto& child : children()) {
            if (auto* widget = dynamic_cast<Widget*>(child.get()))
                widget->layout(true);
        }
        return;
    }

    const ContentArea area = resolveContentArea();
    const Size natural = intrinsicSize();

    const float effX = area.x + marginLeft();
    const float effY = area.y + marginTop();
    const float effWidth = area.width - marginLeft() - marginRight();
    const float effHeight = area.height - marginTop() - marginBottom();

    m_resolvedWidth = hasFlag(m_fill.get(), Fill::Width)
                          ? effWidth
                          : (m_width.get() > 0) ? static_cast<float>(m_width.get()) : natural.width;

    m_resolvedHeight = hasFlag(m_fill.get(), Fill::Height)
                           ? effHeight
                           : (m_height.get() > 0) ? static_cast<float>(m_height.get()) : natural.height;

    const Alignment align = m_alignment.get();
    if (hasFlag(align, Alignment::Left))
        m_resolvedX = effX;
    else if (hasFlag(align, Alignment::Right))
        m_resolvedX = effX + effWidth - m_resolvedWidth;
    else if (hasFlag(align, Alignment::CenterHorizontal))
        m_resolvedX = effX + (effWidth - m_resolvedWidth) * 0.5f;
    else
        m_resolvedX = effX + static_cast<float>(m_x.get());

    if (hasFlag(align, Alignment::Top))
        m_resolvedY = effY;
    else if (hasFlag(align, Alignment::Bottom))
        m_resolvedY = effY + effHeight - m_resolvedHeight;
    else if (hasFlag(align, Alignment::CenterVertical))
        m_resolvedY = effY + (effHeight - m_resolvedHeight) * 0.5f;
    else
        m_resolvedY = effY + static_cast<float>(m_y.get());

    syncDisplayedGeometry();
    updateWorldMatrix();

    for (auto& child : children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout(true);
    }
}

void Widget::setResolved(float x, float y, float width, float height) {
    m_resolvedX = x; m_resolvedY = y;
    m_resolvedWidth = width; m_resolvedHeight = height;
}

void Widget::syncDisplayedGeometry() {
    syncPositionTween();
    syncSizeTween();
    m_layoutInitialized = true;
}

void Widget::syncPositionTween() {
    if (m_positionTween.isAnimating()) {
        bool sameTarget = m_resolvedX == m_positionTween.targetA() && m_resolvedY == m_positionTween.targetB();
        if (sameTarget)
            return;
    } else if (m_layoutInitialized && m_resolvedX == m_displayedX && m_resolvedY == m_displayedY) {
        return;
    }

    bool animate = m_layoutInitialized && m_alignmentAnimationEnabled.get() && m_alignmentAnimationDuration.get() > 0.0f;
    if (!animate) {
        if (m_positionTween.isAnimating())
            m_positionTween.cancel();
        m_displayedX = m_resolvedX;
        m_displayedY = m_resolvedY;
        return;
    }

    m_positionTween.animateTo(this, &m_displayedX, &m_displayedY,
        m_displayedX, m_displayedY, m_resolvedX, m_resolvedY,
        m_alignmentAnimationDuration.get(), m_alignmentAnimationEasing.get());
}

void Widget::syncSizeTween() {
    if (m_sizeTween.isAnimating()) {
        bool sameTarget = m_resolvedWidth == m_sizeTween.targetA() && m_resolvedHeight == m_sizeTween.targetB();
        if (sameTarget)
            return;
    } else if (m_layoutInitialized && m_resolvedWidth == m_displayedWidth && m_resolvedHeight == m_displayedHeight) {
        return;
    }

    bool animate = m_layoutInitialized && m_fillAnimationEnabled.get() && m_fillAnimationDuration.get() > 0.0f;
    if (!animate) {
        if (m_sizeTween.isAnimating())
            m_sizeTween.cancel();
        m_displayedWidth = m_resolvedWidth;
        m_displayedHeight = m_resolvedHeight;
        return;
    }

    m_sizeTween.animateTo(this, &m_displayedWidth, &m_displayedHeight,
        m_displayedWidth, m_displayedHeight, m_resolvedWidth, m_resolvedHeight,
        m_fillAnimationDuration.get(), m_fillAnimationEasing.get());
}

void Widget::PairTween::animateTo(Widget* owner, float* a, float* b,
                                   float fromA, float fromB, float toA, float toB,
                                   float duration, EasingFn easing) {
    m_owner = owner;
    m_a = a;
    m_b = b;

    if (duration <= 0.0f) {
        cancel();
        *a = toA;
        *b = toB;
        return;
    }

    m_fromA = fromA; m_fromB = fromB;
    m_toA = toA; m_toB = toB;
    m_duration = duration;
    m_elapsed = 0.0f;
    m_easing = std::move(easing);

    if (!m_animating) {
        m_animating = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

void Widget::PairTween::cancel() {
    if (m_animating) {
        m_animating = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
}

bool Widget::PairTween::tick(float dt) {
    m_elapsed += dt;
    const float t = std::min(m_elapsed / m_duration, 1.0f);
    const float eased = m_easing ? m_easing(t) : t;

    *m_a = lerp(m_fromA, m_toA, eased);
    *m_b = lerp(m_fromB, m_toB, eased);
    m_owner->markLayoutDirty();

    if (t >= 1.0f) {
        m_animating = false;
        return false;
    }
    return true;
}

Widget::Size Widget::intrinsicSize() {
    if (!m_intrinsicSizeCacheValid) {
        m_cachedIntrinsicSize = computeIntrinsicSize();
        m_intrinsicSizeCacheValid = true;
    }
    return m_cachedIntrinsicSize;
}

Widget::Size Widget::computeIntrinsicSize() {
    return { static_cast<float>(width()), static_cast<float>(height()) };
}

glm::mat4 Widget::localMatrix() const {
    glm::vec3 position{m_displayedX, m_displayedY, 0.0f};
    glm::vec3 pivot{m_displayedWidth * 0.5f, m_displayedHeight * 0.5f, 0.0f};

    glm::mat4 t = glm::translate(glm::mat4{1.0f}, position);
    glm::mat4 toCenter = glm::translate(glm::mat4{1.0f}, pivot);
    glm::mat4 r = glm::rotate(glm::mat4{1.0f}, glm::radians(static_cast<float>(m_rotation)), glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 s = glm::scale(glm::mat4{1.0f}, glm::vec3{static_cast<float>(m_scale), static_cast<float>(m_scale), 1.0f});
    glm::mat4 fromCenter = glm::translate(glm::mat4{1.0f}, -pivot);

    return t * toCenter * r * s * fromCenter;
}

const glm::mat4& Widget::worldMatrix() const {
    return m_worldMatrix;
}

void Widget::updateWorldMatrix() {
    glm::mat4 parentWorld{1.0f};
    if (auto* parentWidget = dynamic_cast<Widget*>(parent()))
        parentWorld = parentWidget->worldMatrix();
    m_worldMatrix = parentWorld * localMatrix();
}

void Widget::focus() {
    if (m_ownerWindow)
        m_ownerWindow->setFocusedWidget(this);
}

}