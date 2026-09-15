#pragma once

#include <tavoos/export.hpp>
#include <tavoos/object.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <string>

struct GLFWwindow;

namespace Tavoos {

class Widget;

class TAVOOS_EXPORT Window : public Object {
    friend class Application;
    friend class Builder;
    friend class Renderer;
    friend class Widget;

public:
    Window();
    ~Window();

    Window(Window&&) noexcept = delete;
    Window& operator=(Window&&) noexcept = delete;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    decltype(auto) width(this auto&& self, PropertyArg<int> width) {
        width.applyTo(self.m_width);
        self.syncWindowSize();
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) height(this auto&& self, PropertyArg<int> height) {
        height.applyTo(self.m_height);
        self.syncWindowSize();
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) color(this auto&& self, PropertyArg<Color> color) {
        color.applyTo(self.m_color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) title(this auto&& self, PropertyArg<std::string> title) {
        title.applyTo(self.m_title);
        self.syncWindowTitle();
        return std::forward<decltype(self)>(self);
    }

    int width() const noexcept { return m_width; }
    int height() const noexcept { return m_height; }
    std::string title() const noexcept { return m_title; }
    Color color() const { return m_color; };
    int framebufferWidth() const noexcept { return m_framebufferWidth; }
    int framebufferHeight() const noexcept { return m_framebufferHeight; }

    void setFocusedWidget(Widget* widget);
    void markDirty();

protected:
    virtual void build() = 0;

private:
    void clearReferencesTo(Widget* subtreeRoot);
    void deferDestruction(std::unique_ptr<Object> widget);

    GLFWwindow* m_window{nullptr};
    Property<int> m_width, m_height;
    Property<Color> m_color;
    Property<std::string> m_title;

    int m_framebufferWidth{0};
    int m_framebufferHeight{0};
    bool m_renderDirty{true};
    bool m_windowActive{true};
    std::vector<std::unique_ptr<Object>> m_pendingDestruction;

    void setup();
    GLFWwindow *handle() const;
    bool consumeDirty();
    void flushPendingDestruction();
    void syncWindowSize();
    void syncWindowTitle();
    static void closeCallback(GLFWwindow*);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void contentScaleCallback(GLFWwindow* window, float xscale, float yscale);
    static void windowFocusCallback(GLFWwindow* window, int focused);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    template<typename EventT>
    static Widget* dispatchBubble(Widget* start, EventT& event, void (Widget::*trigger)(EventT&));

    static Widget* hitTestChildren(Window* self, double x, double y);
    static void collectFocusable(Object* root, std::vector<Widget*>& out);
    void focusNext(bool reverse);
    Widget* m_hoveredWidget{nullptr};
    Widget* m_pressedWidget{nullptr};
    Widget* m_focusedWidget{nullptr};

    Widget* m_lastClickWidget{nullptr};
    double m_lastClickTime{0.0};
    float m_lastClickX{0.0f};
    float m_lastClickY{0.0f};

    static constexpr double kDoubleClickTimeThreshold = 0.4;
    static constexpr float kDoubleClickDistanceThreshold = 5.0f;
};

}