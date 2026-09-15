#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/events/events.h>
#include <tavoos/gfx/renderer.h>
#include <tavoos/window.h>

#include <algorithm>
#include <stdexcept>

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

void Window::clearReferencesTo(Widget* subtreeRoot) {
    auto isOrDescendantOf = [subtreeRoot](Widget* w) {
        while (w) {
            if (w == subtreeRoot)
                return true;
            w = dynamic_cast<Widget*>(w->parent());
        }
        return false;
    };

    if (isOrDescendantOf(m_hoveredWidget))
        m_hoveredWidget = nullptr;
    if (isOrDescendantOf(m_pressedWidget))
        m_pressedWidget = nullptr;
    if (isOrDescendantOf(m_lastClickWidget))
        m_lastClickWidget = nullptr;
    if (isOrDescendantOf(m_focusedWidget))
        setFocusedWidget(nullptr);
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
    }
}

void Window::setup() {
    m_window = glfwCreateWindow(m_width, m_height, m_title.get().c_str(), nullptr,
                                 Application::instance()->shareContextHandle());

    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowCloseCallback(m_window, &Window::closeCallback);
    glfwSetFramebufferSizeCallback(m_window, &Window::framebufferSizeCallback);
    glfwSetWindowContentScaleCallback(m_window, &Window::contentScaleCallback);
    glfwSetWindowFocusCallback(m_window, &Window::windowFocusCallback);
    glfwSetMouseButtonCallback(m_window, &Window::mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, &Window::cursorPosCallback);
    glfwSetScrollCallback(m_window, &Window::scrollCallback);
    glfwSetKeyCallback(m_window, &Window::keyCallback);
    glfwSetCharCallback(m_window, &Window::charCallback);

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

Widget* Window::hitTestChildren(Window* self, double x, double y) {
    for (const auto& child : self->children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get())) {
            if (auto* hit = widget->hitTestTree(static_cast<float>(x), static_cast<float>(y)))
                return hit;
        }
    }
    return nullptr;
}

static KeyModifier toKeyModifier(int mods) {
    KeyModifier km = KeyModifier::None;
    if (mods & GLFW_MOD_SHIFT)   km |= KeyModifier::Shift;
    if (mods & GLFW_MOD_CONTROL) km |= KeyModifier::Control;
    if (mods & GLFW_MOD_ALT)     km |= KeyModifier::Alt;
    if (mods & GLFW_MOD_SUPER)   km |= KeyModifier::Super;
    return km;
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

    if (action == GLFW_PRESS) {
        self->m_pressedWidget = hit;

        if (hit) {
            MouseEvent pressEvent{EventType::MousePress, static_cast<float>(mx), static_cast<float>(my), mb, km};
            dispatchBubble(hit, pressEvent, &Widget::triggerPress);

            const double now = glfwGetTime();
            const float dx = static_cast<float>(mx) - self->m_lastClickX;
            const float dy = static_cast<float>(my) - self->m_lastClickY;
            const bool isDoubleClick = hit == self->m_lastClickWidget &&
                                  (now - self->m_lastClickTime) <= Window::kDoubleClickTimeThreshold &&
                                  (dx * dx + dy * dy) <= Window::kDoubleClickDistanceThreshold * Window::kDoubleClickDistanceThreshold;

            if (isDoubleClick) {
                MouseEvent doubleClickEvent{EventType::MouseDoubleClick, static_cast<float>(mx), static_cast<float>(my), mb, km};
                dispatchBubble(hit, doubleClickEvent, &Widget::triggerDoubleClick);
            }
        }
    } else if (action == GLFW_RELEASE) {
        if (self->m_pressedWidget) {
            MouseEvent releaseEvent{EventType::MouseRelease, static_cast<float>(mx), static_cast<float>(my), mb, km};
            dispatchBubble(self->m_pressedWidget, releaseEvent, &Widget::triggerRelease);
        }

        if (hit && hit == self->m_pressedWidget) {
            MouseEvent clickEvent{EventType::MouseClick, static_cast<float>(mx), static_cast<float>(my), mb, km};
            dispatchBubble(hit, clickEvent, &Widget::triggerClick);

            if (self->m_pressedWidget == hit) {
                self->m_lastClickWidget = hit;
                self->m_lastClickTime = glfwGetTime();
                self->m_lastClickX = static_cast<float>(mx);
                self->m_lastClickY = static_cast<float>(my);
            }
        }

        self->m_pressedWidget = nullptr;
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

    if (hit != self->m_hoveredWidget) {
        if (self->m_hoveredWidget) {
            MouseEvent leaveEvent{EventType::MouseLeave, static_cast<float>(x), static_cast<float>(y), MouseButton::Unknown, KeyModifier::None};
            dispatchBubble(self->m_hoveredWidget, leaveEvent, &Widget::triggerMouseLeave);
        }
        if (hit) {
            MouseEvent enterEvent{EventType::MouseEnter, static_cast<float>(x), static_cast<float>(y), MouseButton::Unknown, KeyModifier::None};
            dispatchBubble(hit, enterEvent, &Widget::triggerMouseEnter);
        }
        self->m_hoveredWidget = hit;
    }

    if (hit) {
        MouseEvent moveEvent{EventType::MouseMove, static_cast<float>(x), static_cast<float>(y), MouseButton::Unknown, KeyModifier::None};
        dispatchBubble(hit, moveEvent, &Widget::triggerMouseMove);
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

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        self->focusNext(mods & GLFW_MOD_SHIFT);
        return;
    }

    if (!self->m_focusedWidget)
        return;

    const KeyModifier km = toKeyModifier(mods);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        KeyEvent event{EventType::KeyPress, key, km};
        dispatchBubble(self->m_focusedWidget, event, &Widget::triggerKeyPress);
    } else if (action == GLFW_RELEASE) {
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
    std::vector<Widget*> chain;
    collectFocusable(this, chain);
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