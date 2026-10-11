#include <tavoos/widget/templates/swipeviewbase.h>

#include <tavoos/events/key.h>
#include <tavoos/gfx/renderer.h>
#include <tavoos/window.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

using Clock = std::chrono::steady_clock;

constexpr float kFlickSpeed = 400.0f;
constexpr float kSampleWindow = 0.1f;
constexpr float kStaleSample = 0.08f;
constexpr float kEpsilon = 1e-4f;

float secondsBetween(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<float>(to - from).count();
}

bool contains(const Object& root, const Widget* target) {
    for (const auto& child : root.children()) {
        if (child.get() == target || contains(*child, target))
            return true;
    }
    return false;
}

bool isInside(const Widget* widget, const Widget* ancestor) {
    for (const Object* object = widget; object; object = object->parent()) {
        if (object == ancestor)
            return true;
    }
    return false;
}

}

SwipeViewBase::SwipeViewBase(Object* parent) : Control{parent} {
    clip(true);
    background([](RectangleWidget& box) { box.color(Color::Transparent); });

    m_orientation.onChange([this](const Orientation&) { markLayoutDirty(); });
    m_currentIndex.onChange([this](const int&) { applyIndex(!m_jumping); });
}

SwipeViewBase::~SwipeViewBase() {
    stopAnimation();
}

std::vector<Widget*> SwipeViewBase::pages() const {
    std::vector<Widget*> result;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (widget && widget != backgroundSlot() && !widget->isOverlay())
            result.push_back(widget);
    }
    return result;
}

int SwipeViewBase::count() const {
    return static_cast<int>(pages().size());
}

Widget* SwipeViewBase::page(int index) const {
    const std::vector<Widget*> all = pages();
    if (index < 0 || index >= static_cast<int>(all.size()))
        return nullptr;
    return all[static_cast<std::size_t>(index)];
}

bool SwipeViewBase::horizontal() const {
    return m_orientation.get() == Orientation::Horizontal;
}

float SwipeViewBase::pageSize() const {
    const float width = std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight());
    const float height = std::max(0.0f, resolvedHeight() - paddingTop() - paddingBottom());
    return horizontal() ? width : height;
}

void SwipeViewBase::render(Renderer& renderer) {
    if (renderer.isCapturingMask()) {
        if (Widget* const shape = backgroundSlot())
            renderer.renderWidget(*shape);
        return;
    }
    refreshPages();
    renderChildren(renderer);
}

Widget::ContentArea SwipeViewBase::contentAreaFor(const Widget& child) const {
    if (&child == backgroundSlot())
        return { 0.0f, 0.0f, displayedWidth(), displayedHeight() };

    const float width = std::max(0.0f, resolvedWidth() - paddingLeft() - paddingRight());
    const float height = std::max(0.0f, resolvedHeight() - paddingTop() - paddingBottom());
    const std::vector<Widget*> all = pages();
    const auto found = std::ranges::find(all, &child);
    const float index = found == all.end() ? 0.0f : static_cast<float>(found - all.begin());
    const float shift = std::round((index - m_position) * pageSize());

    if (horizontal())
        return { paddingLeft() + shift, paddingTop(), width, height };
    return { paddingLeft(), paddingTop() + shift, width, height };
}

void SwipeViewBase::onResolvedSizeChanged() {
    refreshPages();
    markLayoutDirty();
}

void SwipeViewBase::refreshPages() {
    std::vector<Widget*> now = pages();
    if (now == m_pages)
        return;

    std::erase_if(m_focusMemory, [&now](const auto& entry) {
        return std::ranges::find(now, entry.first) == now.end();
    });
    m_pages = std::move(now);
    m_countState.setIfChanged(static_cast<int>(m_pages.size()));

    const auto current = std::ranges::find(m_pages, m_currentPage);
    m_jumping = true;
    if (current != m_pages.end())
        m_currentIndex.set(static_cast<int>(current - m_pages.begin()));
    else
        applyIndex(false);
    m_jumping = false;
    syncVisibility();
}

void SwipeViewBase::setCurrentIndex(int index, bool animated) {
    refreshPages();
    m_jumping = !animated;
    m_currentIndex.set(index);
    m_jumping = false;
}

void SwipeViewBase::next() {
    setCurrentIndex(m_currentIndexState.get() + 1);
}

void SwipeViewBase::previous() {
    setCurrentIndex(m_currentIndexState.get() - 1);
}

void SwipeViewBase::applyIndex(bool animated) {
    const std::vector<Widget*> all = pages();
    const int total = static_cast<int>(all.size());
    const int requested = m_currentIndex.get();
    const int index = std::clamp(requested, 0, std::max(0, total - 1));

    if (total > 0 && index != requested) {
        m_currentIndex.set(index);
        return;
    }

    Widget* const target = total > 0 ? all[static_cast<std::size_t>(index)] : nullptr;
    if (target != m_currentPage) {
        moveFocus(m_currentPage, target);
        m_currentPage = target;
    }

    const int previous = m_currentIndexState.get();
    if (index != previous) {
        m_currentIndexState.setIfChanged(index);
        if (m_onCurrentIndexChange)
            m_onCurrentIndexChange(index);
    }

    if (animated && !m_dragging && pageSize() > 0.0f && m_duration.get() > 0.0f && ownerWindow())
        animateTo(index);
    else if (!m_dragging) {
        stopAnimation();
        setPosition(static_cast<float>(index));
    }
}

void SwipeViewBase::animateTo(int index) {
    const float target = static_cast<float>(index);
    if (std::fabs(m_position - target) < kEpsilon) {
        stopAnimation();
        setPosition(target);
        return;
    }

    m_from = m_position;
    m_to = target;
    m_elapsed = 0.0f;
    if (!m_animating) {
        m_animating = true;
        AnimationManager::instance().registerAnimation(this);
    }
}

void SwipeViewBase::stopAnimation() {
    if (m_animating) {
        m_animating = false;
        AnimationManager::instance().unregisterAnimation(this);
    }
}

bool SwipeViewBase::tick(float dt) {
    m_elapsed += dt;
    const float duration = m_duration.get();
    const float t = duration > 0.0f ? std::min(m_elapsed / duration, 1.0f) : 1.0f;
    const EasingFn& easing = m_easing.get();
    setPosition(m_from + (m_to - m_from) * (easing ? easing(t) : t));

    if (t < 1.0f)
        return true;

    setPosition(m_to);
    m_animating = false;
    return false;
}

void SwipeViewBase::setPosition(float value) {
    const float limit = static_cast<float>(std::max(0, count() - 1));
    value = std::clamp(value, 0.0f, limit);
    if (value == m_position)
        return;
    m_position = value;
    syncVisibility();
    markLayoutDirty();
}

void SwipeViewBase::syncVisibility() {
    const std::vector<Widget*> all = pages();
    const int first = static_cast<int>(std::floor(m_position + kEpsilon));
    const int last = static_cast<int>(std::ceil(m_position - kEpsilon));
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        const bool shown = i >= first && i <= last;
        Widget* const widget = all[static_cast<std::size_t>(i)];
        if (widget->visible() != shown)
            widget->visible(shown);
    }
}

void SwipeViewBase::moveFocus(Widget* from, Widget* to) {
    Window* const window = ownerWindow();
    if (!window || !from || !to)
        return;

    Widget* const focused = window->focusedWidget();
    if (!focused || !isInside(focused, from))
        return;

    const auto known = std::ranges::find_if(m_focusMemory, [from](const auto& entry) { return entry.first == from; });
    if (known != m_focusMemory.end())
        known->second = focused;
    else
        m_focusMemory.emplace_back(from, focused);

    const auto saved = std::ranges::find_if(m_focusMemory, [to](const auto& entry) { return entry.first == to; });
    if (saved != m_focusMemory.end() && saved->second && contains(*to, saved->second)) {
        window->setFocusedWidget(saved->second);
        return;
    }

    window->setFocusedWidget(nullptr);
    window->focusFirstIn(to);
}

bool SwipeViewBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
        return m_interactive.get() && count() > 1;
    case EventType::KeyPress:
        return m_keyNavigation.get() && count() > 1;
    default:
        return Control::hasHandlerFor(type);
    }
}

void SwipeViewBase::beginDrag() {
    refreshPages();
    stopAnimation();
    m_dragging = true;
    m_samples.clear();
}

void SwipeViewBase::recordSample() {
    const Clock::time_point now = Clock::now();
    m_samples.push_back({now, m_position * pageSize()});
    while (m_samples.size() > 1 && secondsBetween(m_samples.front().time, now) > kSampleWindow)
        m_samples.erase(m_samples.begin());
}

float SwipeViewBase::releaseVelocity() const {
    if (m_samples.size() < 2)
        return 0.0f;

    const Sample& first = m_samples.front();
    const Sample& last = m_samples.back();
    if (secondsBetween(last.time, Clock::now()) > kStaleSample)
        return 0.0f;

    const float elapsed = secondsBetween(first.time, last.time);
    return elapsed > 0.0f ? (last.pixels - first.pixels) / elapsed : 0.0f;
}

void SwipeViewBase::triggerDragStart(DragEvent& event) {
    beginDrag();
    Widget::triggerDragStart(event);
}

void SwipeViewBase::triggerDragMove(DragEvent& event) {
    if (!m_dragging)
        beginDrag();

    const float size = pageSize();
    if (size > 0.0f) {
        const float delta = horizontal() ? event.dx() : event.dy();
        setPosition(m_position - delta / size);
        recordSample();
    }
    Widget::triggerDragMove(event);
}

void SwipeViewBase::triggerDragEnd(DragEvent& event) {
    if (m_dragging) {
        const float velocity = releaseVelocity();
        int target = static_cast<int>(std::lround(m_position));
        if (std::fabs(velocity) > kFlickSpeed)
            target = velocity > 0.0f ? static_cast<int>(std::ceil(m_position - kEpsilon))
                                     : static_cast<int>(std::floor(m_position + kEpsilon));
        m_dragging = false;
        m_samples.clear();
        m_currentIndex.set(target);
    }
    Widget::triggerDragEnd(event);
}

void SwipeViewBase::triggerKeyPress(KeyEvent& event) {
    const int key = event.keyCode();
    const bool isHorizontal = horizontal();

    int step = 0;
    if (key == static_cast<int>(isHorizontal ? Key::Left : Key::Up))
        step = -1;
    else if (key == static_cast<int>(isHorizontal ? Key::Right : Key::Down))
        step = 1;

    const int before = m_currentIndexState.get();
    if (step != 0)
        setCurrentIndex(before + step);

    if (step == 0 || m_currentIndexState.get() == before)
        event.ignore();
    Widget::triggerKeyPress(event);
}

}
