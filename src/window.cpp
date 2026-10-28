#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/events/events.h>
#include <tavoos/events/shortcut.h>
#include <tavoos/gfx/renderer.h>
#include <tavoos/widget/droparea.h>
#include <tavoos/widget/templates/overlaybase.h>
#include <tavoos/window.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Tavoos {

Window::Window() {
    if (Application::instance() == nullptr)
        throw std::runtime_error{"Tavoos::Application must be constructed before Tavoos::Window"};

    m_color.onChange([this](const auto&) { markDirty(); });
}

void Window::markDirty() {
    m_renderDirty = true;
}

bool Window::consumeDirty() {
    if (m_renderDirty) {
        m_renderDirty = false;
        return true;
    }
    return false;
}

void Window::clearReferencesTo(Object* subtreeRoot) {
    auto isOrDescendantOf = [subtreeRoot](const Object* object) {
        while (object) {
            if (object == subtreeRoot)
                return true;
            object = object->parent();
        }
        return false;
    };

    std::erase_if(m_hoveredChain, isOrDescendantOf);
    if (isOrDescendantOf(m_pressedWidget))
        m_pressedWidget = nullptr;
    std::erase_if(m_pressedChain, isOrDescendantOf);
    std::erase_if(m_shortcuts, isOrDescendantOf);
    if (isOrDescendantOf(m_dropTarget))
        m_dropTarget = nullptr;
    std::erase_if(m_dropRejected, isOrDescendantOf);
    if (isOrDescendantOf(m_dragSource)) {
        m_dragSource = nullptr;
        m_dropRejected.clear();
        leaveDropTarget();
    }
    if (isOrDescendantOf(m_lastClickWidget))
        m_lastClickWidget = nullptr;
    if (isOrDescendantOf(m_focusedWidget))
        setFocusedWidget(nullptr);
    std::erase_if(m_overlays, isOrDescendantOf);
    std::erase_if(m_retiredOverlays, isOrDescendantOf);
    std::erase_if(m_focusMemory, [&](const auto& entry) { return isOrDescendantOf(entry.first); });
    for (auto& entry : m_focusMemory)
        if (isOrDescendantOf(entry.second))
            entry.second = nullptr;
}

void Window::focusFirstIn(Widget* root) {
    std::vector<Widget*> chain;
    collectFocusable(root, chain);
    if (!chain.empty())
        setFocusedWidget(chain.front());
}

void Window::rememberFocus(Widget* key) {
    const auto known = std::ranges::find_if(m_focusMemory, [key](const auto& entry) { return entry.first == key; });
    if (known == m_focusMemory.end())
        m_focusMemory.emplace_back(key, m_focusedWidget);
}

void Window::restoreFocus(Widget* key) {
    Widget* restore = nullptr;
    const auto entry = std::ranges::find_if(m_focusMemory, [key](const auto& e) { return e.first == key; });
    if (entry != m_focusMemory.end()) {
        restore = entry->second;
        m_focusMemory.erase(entry);
    }

    for (Widget* w = m_focusedWidget; w; w = dynamic_cast<Widget*>(w->parent())) {
        if (w == key) {
            setFocusedWidget(restore);
            break;
        }
    }
}

void Window::transferFocusMemory(Widget* from, Widget* to) {
    const auto source = std::ranges::find_if(m_focusMemory, [from](const auto& entry) { return entry.first == from; });
    if (source == m_focusMemory.end())
        return;

    Widget* const saved = source->second;
    m_focusMemory.erase(source);

    const auto target = std::ranges::find_if(m_focusMemory, [to](const auto& entry) { return entry.first == to; });
    if (target != m_focusMemory.end())
        target->second = saved;
    else
        m_focusMemory.emplace_back(to, saved);
}

void Window::addOverlay(OverlayBase* overlay) {
    std::erase(m_overlays, overlay);
    std::erase(m_retiredOverlays, overlay);
    rememberFocus(overlay);
    m_overlays.push_back(overlay);
    focusFirstIn(overlay);
    markDirty();
}

void Window::removeOverlay(OverlayBase* overlay) {
    if (std::erase(m_overlays, overlay) == 0)
        return;
    restoreFocus(overlay);
    markDirty();
}

void Window::retireOverlay(OverlayBase* overlay) {
    if (std::erase(m_overlays, overlay) == 0)
        return;
    std::erase(m_retiredOverlays, overlay);
    m_retiredOverlays.push_back(overlay);
    restoreFocus(overlay);
    markDirty();
}

void Window::releaseOverlay(OverlayBase* overlay) {
    if (std::erase(m_retiredOverlays, overlay) > 0)
        markDirty();
}

void Window::deferDestruction(std::unique_ptr<Object> widget) {
    m_pendingDestruction.push_back(std::move(widget));
}

void Window::flushPendingDestruction() {
    m_pendingDestruction.clear();
}

Window::~Window() {
    if (m_window != nullptr) {
        glfwMakeContextCurrent(m_window);
        clearChildren();
        m_pendingDestruction.clear();
        Application::instance()->renderer->releaseWindowResources(*this);
        glfwDestroyWindow(m_window);
        for (const auto& [shape, cursor] : m_cursors)
            glfwDestroyCursor(cursor);
    }
}

void Window::setup() {
    m_window = glfwCreateWindow(m_width, m_height, m_title.get().c_str(), nullptr,
                                 Application::instance()->shareContextHandle());

    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowCloseCallback(m_window, &Window::closeCallback);
    glfwSetWindowSizeCallback(m_window, &Window::windowSizeCallback);
    glfwSetFramebufferSizeCallback(m_window, &Window::framebufferSizeCallback);
    glfwSetWindowContentScaleCallback(m_window, &Window::contentScaleCallback);
    glfwSetWindowFocusCallback(m_window, &Window::windowFocusCallback);
    glfwSetMouseButtonCallback(m_window, &Window::mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, &Window::cursorPosCallback);
    glfwSetScrollCallback(m_window, &Window::scrollCallback);
    glfwSetKeyCallback(m_window, &Window::keyCallback);
    glfwSetCharCallback(m_window, &Window::charCallback);
    glfwSetDropCallback(m_window, &Window::dropCallback);

    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    m_windowActive = glfwGetWindowAttrib(m_window, GLFW_FOCUSED) == GLFW_TRUE;

    glfwMakeContextCurrent(m_window);

    Application::instance()->initRenderer();

    Builder::currentItem = this;
    Widget::s_isBuilding = true;

    build();

    for (const auto& child : children())
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->updateEffectiveOpacity();

    Widget::s_isBuilding = false;
    Builder::currentItem = nullptr;
}

GLFWwindow* Window::handle() const {
    return m_window;
}

void Window::syncWindowSize() {
    if (m_window != nullptr)
        glfwSetWindowSize(m_window, m_width, m_height);
}

void Window::syncWindowTitle() {
    if (m_window != nullptr)
        glfwSetWindowTitle(m_window, m_title.get().c_str());
}

void Window::closeCallback(GLFWwindow* window) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    Application::instance()->unregisterWindow(self);
}

template<typename EventT>
Widget* Window::dispatchBubble(Widget* start, EventT& event, void (Widget::*trigger)(EventT&)) {
    Widget* current = start;
    while (current) {
        if (current->hasHandlerFor(event.type())) {
            event.accept();
            (current->*trigger)(event);
            if (event.isAccepted())
                return current;
        }
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}

Widget* Window::dispatchMouseBubble(Widget* start, MouseEvent& event, const Point& windowPoint, void (Widget::*trigger)(MouseEvent&)) {
    Widget* current = start;
    while (current) {
        if (current->hasHandlerFor(event.type())) {
            event.setPosition(current->mapFromWindow(windowPoint));
            event.accept();
            (current->*trigger)(event);
            if (event.isAccepted())
                return current;
        }
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}

Widget* Window::hitTestChildren(Window* self, double x, double y, const Widget* skip) {
    for (auto it = self->m_overlays.rbegin(); it != self->m_overlays.rend(); ++it) {
        if (auto* hit = static_cast<Widget*>(*it)->hitTestTree(static_cast<float>(x), static_cast<float>(y), skip))
            return hit;
        if ((*it)->modalActive())
            return nullptr;
    }

    const std::vector<Widget*> ordered = Widget::zOrderedChildren(*self);
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
        if (auto* hit = (*it)->hitTestTree(static_cast<float>(x), static_cast<float>(y), skip))
            return hit;
    }
    return nullptr;
}

void Window::layoutTree() {
    for (const auto& child : children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout();
    }
    std::vector<OverlayBase*> overlays = m_retiredOverlays;
    overlays.insert(overlays.end(), m_overlays.begin(), m_overlays.end());
    for (OverlayBase* const overlay : overlays) {
        overlay->place();
        overlay->layout(true);
    }
}

Point Window::dragHotSpotInWindow() const {
    const Point fraction = m_dragSource->dragHotSpot();
    return m_dragSource->mapToWindow({fraction.x * m_dragSource->displayedWidth(), fraction.y * m_dragSource->displayedHeight()});
}

void Window::updateDropTarget() {
    layoutTree();
    const Point point = dragHotSpotInWindow();
    std::vector<DropAreaWidget*> candidates;
    for (Widget* const widget : hoverChainAt(hitTestChildren(this, point.x, point.y, m_dragSource), point)) {
        if (auto* const area = dynamic_cast<DropAreaWidget*>(widget); area && area->acceptsKeys(m_dragSource->dragKeys()))
            candidates.push_back(area);
    }
    std::erase_if(m_dropRejected, [&candidates](DropAreaWidget* area) { return !std::ranges::contains(candidates, area); });
    std::erase_if(candidates, [this](DropAreaWidget* area) { return std::ranges::contains(m_dropRejected, area); });

    for (DropAreaWidget* const area : candidates) {
        if (!m_dragSource)
            return;
        if (area == m_dropTarget) {
            DropEvent event{EventType::DropMove, m_dragSource, m_dragSource->dragKeys(), m_dragSource->dragData(), {}};
            event.setPosition(area->mapFromWindow(point));
            if (area->m_onPositionChanged)
                area->m_onPositionChanged(event);
            return;
        }
        DropEvent event{EventType::DropEnter, m_dragSource, m_dragSource->dragKeys(), m_dragSource->dragData(), {}};
        event.setPosition(area->mapFromWindow(point));
        if (area->m_onEntered)
            area->m_onEntered(event);
        if (!m_dragSource)
            return;
        if (!event.isAccepted()) {
            m_dropRejected.push_back(area);
            continue;
        }
        leaveDropTarget();
        m_dropTarget = area;
        area->m_dragSource = m_dragSource;
        area->m_containsDrag.setIfChanged(true);
        return;
    }
    leaveDropTarget();
}

void Window::leaveDropTarget() {
    DropAreaWidget* const area = std::exchange(m_dropTarget, nullptr);
    if (!area)
        return;
    area->m_dragSource = nullptr;
    area->m_containsDrag.setIfChanged(false);
    if (area->m_onExited)
        area->m_onExited();
}

void Window::finishDrag() {
    if (!m_dragSource)
        return;
    updateDropTarget();
    if (DropAreaWidget* const area = std::exchange(m_dropTarget, nullptr); area && m_dragSource) {
        DropEvent event{EventType::Drop, m_dragSource, m_dragSource->dragKeys(), m_dragSource->dragData(), {}};
        event.setPosition(area->mapFromWindow(dragHotSpotInWindow()));
        if (area->m_onDropped)
            area->m_onDropped(event);
        area->m_dragSource = nullptr;
        area->m_containsDrag.setIfChanged(false);
    }
    m_dragSource = nullptr;
    m_dropRejected.clear();
}

std::vector<Widget*> Window::hoverChainAt(Widget* hit, const Point& windowPoint) {
    std::vector<Widget*> chain;
    for (Widget* widget = hit; widget; widget = widget->isOverlay() ? nullptr : dynamic_cast<Widget*>(widget->parent())) {
        if (widget->hitTest(windowPoint.x, windowPoint.y))
            chain.push_back(widget);
    }
    return chain;
}

void Window::updateHover(Widget* hit, const Point& windowPoint) {
    const std::vector<Widget*> chain = hoverChainAt(hit, windowPoint);
    const std::vector<Widget*> previous = std::exchange(m_hoveredChain, chain);

    for (Widget* widget : previous) {
        if (std::ranges::contains(chain, widget) || !widget->hasHandlerFor(EventType::MouseLeave))
            continue;
        MouseEvent leaveEvent{EventType::MouseLeave, 0.0f, 0.0f, MouseButton::Unknown, KeyModifier::None};
        leaveEvent.setPosition(widget->mapFromWindow(windowPoint));
        widget->triggerMouseLeave(leaveEvent);
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        Widget* const widget = *it;
        if (std::ranges::contains(previous, widget) || !std::ranges::contains(m_hoveredChain, widget) || !widget->hasHandlerFor(EventType::MouseEnter))
            continue;
        MouseEvent enterEvent{EventType::MouseEnter, 0.0f, 0.0f, MouseButton::Unknown, KeyModifier::None};
        enterEvent.setPosition(widget->mapFromWindow(windowPoint));
        widget->triggerMouseEnter(enterEvent);
    }
}

static int glfwCursorShape(CursorShape shape) {
    switch (shape) {
    case CursorShape::IBeam:        return GLFW_IBEAM_CURSOR;
    case CursorShape::Crosshair:    return GLFW_CROSSHAIR_CURSOR;
    case CursorShape::PointingHand: return GLFW_POINTING_HAND_CURSOR;
    case CursorShape::ResizeEW:     return GLFW_RESIZE_EW_CURSOR;
    case CursorShape::ResizeNS:     return GLFW_RESIZE_NS_CURSOR;
    case CursorShape::ResizeNWSE:   return GLFW_RESIZE_NWSE_CURSOR;
    case CursorShape::ResizeNESW:   return GLFW_RESIZE_NESW_CURSOR;
    case CursorShape::ResizeAll:    return GLFW_RESIZE_ALL_CURSOR;
    case CursorShape::NotAllowed:   return GLFW_NOT_ALLOWED_CURSOR;
    default:                        return GLFW_ARROW_CURSOR;
    }
}

void Window::updateCursor() {
    const std::vector<Widget*>& chain = m_pressedWidget ? m_pressedChain : m_hoveredChain;
    const auto set = std::ranges::find_if(chain, [](const Widget* widget) { return widget->cursor() != CursorShape::Inherit; });
    const CursorShape shape = set != chain.end() ? (*set)->cursor() : CursorShape::Arrow;
    if (shape == m_cursorShape || !m_window)
        return;
    m_cursorShape = shape;
    glfwSetCursor(m_window, cursorFor(shape));
}

GLFWcursor* Window::cursorFor(CursorShape shape) {
    if (shape == CursorShape::Arrow)
        return nullptr;
    GLFWcursor*& cursor = m_cursors[shape];
    if (!cursor)
        cursor = glfwCreateStandardCursor(glfwCursorShape(shape));
    return cursor;
}

void Window::addShortcut(Shortcut* shortcut) {
    m_shortcuts.push_back(shortcut);
}

void Window::removeShortcut(Shortcut* shortcut) {
    std::erase(m_shortcuts, shortcut);
}

bool Window::claimsShortcut(const KeyEvent& key) const {
    for (Widget* widget = m_focusedWidget; widget; widget = dynamic_cast<Widget*>(widget->parent())) {
        if (!widget->hasHandlerFor(EventType::ShortcutOverride))
            continue;
        KeyEvent event{EventType::ShortcutOverride, key.keyCode(), key.modifiers()};
        widget->triggerShortcutOverride(event);
        if (event.isAccepted())
            return true;
    }
    return false;
}

bool Window::reachesShortcut(const Shortcut* shortcut) const {
    const auto modal = std::find_if(m_overlays.rbegin(), m_overlays.rend(), [](const OverlayBase* overlay) { return overlay->modalActive(); });
    if (modal == m_overlays.rend())
        return true;
    for (const Object* object = shortcut; object; object = object->parent()) {
        if (object == *modal)
            return true;
    }
    return false;
}

bool Window::runShortcut(const KeySequence& sequence, bool repeat) {
    std::vector<Shortcut*> matches;
    for (const auto& window : Application::instance()->m_windows) {
        for (Shortcut* shortcut : window->m_shortcuts) {
            if (!shortcut->matches(sequence, repeat))
                continue;
            if (shortcut->context() == ShortcutContext::Application || (window.get() == this && reachesShortcut(shortcut)))
                matches.push_back(shortcut);
        }
    }

    if (matches.empty())
        return false;
    if (matches.size() == 1) {
        if (matches.front()->m_onActivated)
            matches.front()->m_onActivated();
        return true;
    }
    for (Shortcut* shortcut : matches) {
        if (shortcut->m_onActivatedAmbiguously)
            shortcut->m_onActivatedAmbiguously();
    }
    return true;
}

static KeyModifier toKeyModifier(int mods) {
    KeyModifier km = KeyModifier::None;
    if (mods & GLFW_MOD_SHIFT)   km |= KeyModifier::Shift;
    if (mods & GLFW_MOD_CONTROL) km |= KeyModifier::Control;
    if (mods & GLFW_MOD_ALT)     km |= KeyModifier::Alt;
    if (mods & GLFW_MOD_SUPER)   km |= KeyModifier::Super;
    return km;
}

static Widget* nearestFocusable(Widget* start) {
    Widget* current = start;
    while (current) {
        if (current->focusable())
            return current;
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    MouseButton mb = MouseButton::Unknown;
    if (button == GLFW_MOUSE_BUTTON_LEFT)        mb = MouseButton::Left;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)  mb = MouseButton::Right;
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) mb = MouseButton::Middle;

    const KeyModifier km = toKeyModifier(mods);

    Widget* const hit = hitTestChildren(self, mx, my);
    const Point windowPoint{static_cast<float>(mx), static_cast<float>(my)};

    if (action == GLFW_PRESS && !self->m_overlays.empty()) {
        OverlayBase* const top = self->m_overlays.back();
        const ClosePolicy policy = top->closePolicy();
        const bool outside = !top->isContentHit(static_cast<Widget*>(top)->hitTestTree(static_cast<float>(mx), static_cast<float>(my)));
        if (outside && hasFlag(policy, ClosePolicy::ClickOutside)) {
            top->close();
        } else if (outside && hasFlag(policy, ClosePolicy::ClickOutsideParent)) {
            const Object* const anchor = static_cast<Widget*>(top)->parent();
            bool onAnchor = false;
            for (const Object* object = hit; object && !onAnchor; object = object->parent())
                onAnchor = object == anchor;
            if (!onAnchor)
                top->close();
        }
    }

    if (action == GLFW_PRESS) {
        self->m_pressedWidget = hit;
        self->m_pressedChain = hoverChainAt(hit, windowPoint);
        self->updateCursor();
        self->m_dragging = false;
        self->m_dragStartPoint = windowPoint;
        self->m_dragLastPoint = windowPoint;

        if (Widget* const focusTarget = nearestFocusable(hit))
            focusTarget->focus();

        if (hit) {
            const double now = glfwGetTime();
            const float dx = static_cast<float>(mx) - self->m_lastClickX;
            const float dy = static_cast<float>(my) - self->m_lastClickY;
            const bool isRepeat = hit == self->m_lastClickWidget &&
                                  (now - self->m_lastClickTime) <= Window::kDoubleClickTimeThreshold &&
                                  (dx * dx + dy * dy) <= Window::kDoubleClickDistanceThreshold * Window::kDoubleClickDistanceThreshold;
            self->m_clickCount = isRepeat ? self->m_clickCount + 1 : 1;

            MouseEvent pressEvent{EventType::MousePress, 0.0f, 0.0f, mb, km};
            pressEvent.setClickCount(self->m_clickCount);
            dispatchMouseBubble(hit, pressEvent, windowPoint, &Widget::triggerPress);

            if (isRepeat) {
                MouseEvent doubleClickEvent{EventType::MouseDoubleClick, 0.0f, 0.0f, mb, km};
                doubleClickEvent.setClickCount(self->m_clickCount);
                dispatchMouseBubble(hit, doubleClickEvent, windowPoint, &Widget::triggerDoubleClick);
            }
        }
    } else if (action == GLFW_RELEASE) {
        bool dragHandled = false;
        self->finishDrag();
        if (self->m_pressedWidget && self->m_dragging) {
            const float dx = windowPoint.x - self->m_dragLastPoint.x;
            const float dy = windowPoint.y - self->m_dragLastPoint.y;
            const float totalDx = windowPoint.x - self->m_dragStartPoint.x;
            const float totalDy = windowPoint.y - self->m_dragStartPoint.y;
            DragEvent dragEndEvent{EventType::DragEnd, dx, dy, totalDx, totalDy};
            dragHandled = dispatchBubble(self->m_pressedWidget, dragEndEvent, &Widget::triggerDragEnd) != nullptr;
            self->m_dragging = false;
        }

        if (self->m_pressedWidget) {
            MouseEvent releaseEvent{EventType::MouseRelease, 0.0f, 0.0f, mb, km};
            releaseEvent.setClickCount(self->m_clickCount);
            dispatchMouseBubble(self->m_pressedWidget, releaseEvent, windowPoint, &Widget::triggerRelease);
        }

        if (hit && hit == self->m_pressedWidget && !dragHandled) {
            MouseEvent clickEvent{EventType::MouseClick, 0.0f, 0.0f, mb, km};
            clickEvent.setClickCount(self->m_clickCount);
            dispatchMouseBubble(hit, clickEvent, windowPoint, &Widget::triggerClick);

            if (self->m_pressedWidget == hit) {
                self->m_lastClickWidget = hit;
                self->m_lastClickTime = glfwGetTime();
                self->m_lastClickX = static_cast<float>(mx);
                self->m_lastClickY = static_cast<float>(my);
            }
        }

        self->m_pressedWidget = nullptr;
        self->m_pressedChain.clear();
        self->updateCursor();
    }
}

void Window::windowSizeCallback(GLFWwindow* window, int width, int height) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_width.set(width);
    self->m_height.set(height);
    self->markDirty();

    for (const auto& child : self->children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->requestRelayout();
    }
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_framebufferWidth = width;
    self->m_framebufferHeight = height;
    self->markDirty();

    for (const auto& child : self->children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->requestRelayout();
    }
}

void Window::contentScaleCallback(GLFWwindow* window, float /*xscale*/, float /*yscale*/) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->markDirty();

    for (const auto& child : self->children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->requestRelayout();
    }
}

void Window::windowFocusCallback(GLFWwindow* window, int focused) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    const bool active = (focused == GLFW_TRUE);
    if (active == self->m_windowActive)
        return;
    self->m_windowActive = active;

    if (!self->m_focusedWidget)
        return;

    if (active) {
        Event focusInEvent{EventType::FocusIn};
        self->m_focusedWidget->triggerFocusIn(focusInEvent);
    } else {
        Event focusOutEvent{EventType::FocusOut};
        self->m_focusedWidget->triggerFocusOut(focusOutEvent);
    }
}

void Window::cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    Widget* const hit = hitTestChildren(self, x, y);
    const Point windowPoint{static_cast<float>(x), static_cast<float>(y)};

    self->updateHover(hit, windowPoint);
    self->updateCursor();

    if (hit) {
        MouseEvent moveEvent{EventType::MouseMove, 0.0f, 0.0f, MouseButton::Unknown, KeyModifier::None};
        dispatchMouseBubble(hit, moveEvent, windowPoint, &Widget::triggerMouseMove);
    }

    if (self->m_pressedWidget) {
        const float totalDx = windowPoint.x - self->m_dragStartPoint.x;
        const float totalDy = windowPoint.y - self->m_dragStartPoint.y;

        bool justStarted = false;
        if (!self->m_dragging) {
            const float threshold = self->m_pressedWidget->dragThreshold();
            const float totalDist = std::sqrt(totalDx * totalDx + totalDy * totalDy);
            if (totalDist >= threshold) {
                self->m_dragging = true;
                justStarted = true;
            }
        }

        if (self->m_dragging) {
            const float dx = windowPoint.x - self->m_dragLastPoint.x;
            const float dy = windowPoint.y - self->m_dragLastPoint.y;

            if (justStarted) {
                DragEvent dragStartEvent{EventType::DragStart, dx, dy, totalDx, totalDy};
                Widget* const handler = dispatchBubble(self->m_pressedWidget, dragStartEvent, &Widget::triggerDragStart);
                if (handler && handler->draggable() && !handler->dragKeys().empty())
                    self->m_dragSource = handler;
            }

            DragEvent dragMoveEvent{EventType::DragMove, dx, dy, totalDx, totalDy};
            dispatchBubble(self->m_pressedWidget, dragMoveEvent, &Widget::triggerDragMove);
            self->m_dragLastPoint = windowPoint;
            if (self->m_dragSource)
                self->updateDropTarget();
        }
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    Widget* const hit = hitTestChildren(self, mx, my);

    if (hit) {
        WheelEvent event{static_cast<float>(xoffset), static_cast<float>(yoffset)};
        dispatchBubble(hit, event, &Widget::triggerWheel);
    }
}

void Window::dropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    const Point windowPoint{static_cast<float>(mx), static_cast<float>(my)};

    const std::vector<std::string> keys{"text/uri-list"};
    for (Widget* const widget : hoverChainAt(hitTestChildren(self, mx, my), windowPoint)) {
        auto* const area = dynamic_cast<DropAreaWidget*>(widget);
        if (!area || !area->acceptsKeys(keys))
            continue;
        if (area->m_onDropped) {
            DropEvent event{EventType::Drop, nullptr, keys, {}, std::vector<std::string>(paths, paths + count)};
            event.setPosition(area->mapFromWindow(windowPoint));
            area->m_onDropped(event);
        }
        return;
    }
}

void Window::setFocusedWidget(Widget* widget) {
    if (widget == m_focusedWidget)
        return;

    if (m_focusedWidget) {
        Event focusOutEvent{EventType::FocusOut};
        m_focusedWidget->triggerFocusOut(focusOutEvent);
    }

    m_focusedWidget = widget;

    if (m_focusedWidget) {
        Event focusInEvent{EventType::FocusIn};
        m_focusedWidget->triggerFocusIn(focusInEvent);
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && !self->m_overlays.empty()) {
        OverlayBase* const top = self->m_overlays.back();
        if (hasFlag(top->closePolicy(), ClosePolicy::Escape)) {
            top->close();
            return;
        }
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        self->focusNext(mods & GLFW_MOD_SHIFT);
        return;
    }

    const KeyModifier km = toKeyModifier(mods);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        KeyEvent event{EventType::KeyPress, key, km};
        if (!self->claimsShortcut(event) && self->runShortcut(KeySequence{km, static_cast<Key>(key)}, action == GLFW_REPEAT))
            return;
        if (self->m_focusedWidget)
            dispatchBubble(self->m_focusedWidget, event, &Widget::triggerKeyPress);
    } else if (action == GLFW_RELEASE && self->m_focusedWidget) {
        KeyEvent event{EventType::KeyRelease, key, km};
        dispatchBubble(self->m_focusedWidget, event, &Widget::triggerKeyRelease);
    }
}

void Window::collectFocusable(Object* root, std::vector<Widget*>& out) {
    for (const auto& child : root->children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || !widget->visible())
            continue;

        if (widget->focusable())
            out.push_back(widget);

        collectFocusable(widget, out);
    }
}

void Window::focusNext(bool reverse) {
    Object* focusRoot = this;
    if (!m_overlays.empty() && m_overlays.back()->modalActive())
        focusRoot = m_overlays.back();

    std::vector<Widget*> chain;
    collectFocusable(focusRoot, chain);
    if (chain.empty())
        return;

    auto it = std::find(chain.begin(), chain.end(), m_focusedWidget);
    if (it == chain.end()) {
        setFocusedWidget(reverse ? chain.back() : chain.front());
        return;
    }

    if (reverse) {
        if (it == chain.begin())
            it = chain.end();
        --it;
    } else {
        ++it;
        if (it == chain.end())
            it = chain.begin();
    }
    setFocusedWidget(*it);
}

void Window::charCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (!self->m_focusedWidget)
        return;

    KeyEvent event{EventType::TextInput, static_cast<int>(codepoint), KeyModifier::None};
    dispatchBubble(self->m_focusedWidget, event, &Widget::triggerTextInput);
}

}