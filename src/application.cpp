#include <tavoos/animation/animationmanager.h>
#include <tavoos/application.h>
#include <tavoos/gfx/renderer.h>
#include <tavoos/gfx/svgtexturecache.h>
#include <tavoos/gfx/texturecache.h>
#include <tavoos/text/fontmanager.h>
#include <tavoos/window.h>

#include <algorithm>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace Tavoos {

namespace {

double fastestFrameInterval() {
    constexpr int kFallbackRefreshRate = 60;
    int fastest = 0;
    int count = 0;
    GLFWmonitor** const monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; ++i) {
        if (const GLFWvidmode* const mode = glfwGetVideoMode(monitors[i]))
            fastest = std::max(fastest, mode->refreshRate);
    }
    return 1.0 / (fastest > 0 ? fastest : kFallbackRefreshRate);
}

}

Application* Application::m_instance = nullptr;

Application* Application::instance() noexcept {
    return m_instance;
}

Application::Application() {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    if (m_instance != nullptr)
        throw std::runtime_error{"only one Tavoos::Application instance is allowed"};

    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error{"glfwInit failed"};

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_instance = this;
    renderer = std::unique_ptr<Renderer>(new Renderer());
}

Application::~Application() {
    if (!m_windows.empty()) {
        shutdown();
        m_windows.clear();
    }

    if (m_shareContext != nullptr)
        glfwDestroyWindow(m_shareContext);

    glfwTerminate();
}

void Application::registerFont(const std::string &familyName, const std::string &sourcePath, FontWeight weight, FontStyle style) {
    FontManager::instance().registerFont(familyName, sourcePath, weight, style);
}

int Application::run() {
    if (!renderer->isInit()) {
        spdlog::warn("no window has been created");
        return 0;
    }

    const double frameInterval = fastestFrameInterval();
    double lastTime = glfwGetTime();

    while (!m_shouldQuit) {
        const bool wasAnimating = AnimationManager::instance().hasActiveAnimations();
        if (!wasAnimating)
            glfwWaitEvents();
        else if (const double remaining = frameInterval - (glfwGetTime() - lastTime); remaining > 0.0)
            glfwWaitEventsTimeout(remaining);
        else
            glfwPollEvents();

        const double now = glfwGetTime();
        const float dt = wasAnimating ? static_cast<float>(now - lastTime) : 0.0f;
        lastTime = now;
        AnimationManager::instance().tick(dt);

        if (m_windows.empty())
            break;

        for (const auto& window : m_windows) {
            renderer->renderForWindow(*window);
        }
    }

    if (!m_windows.empty()) {
        shutdown();
        m_windows.clear();
    }

    return 0;
}

void Application::quit() {
    if (m_instance != nullptr)
        m_instance->m_shouldQuit = true;
}

void Application::shutdown() {
    if (m_windows.empty())
        return;

    glfwMakeContextCurrent(m_windows.front()->handle());
    FontManager::instance().releaseAll();
    TextureCache::releaseAll();
    SVGTextureCache::releaseAll();
    renderer->shutdown();
}

void Application::initRenderer() {
    if (!renderer->isInit()) {
        renderer->init();
    }
}

GLFWwindow* Application::shareContextHandle() {
    if (m_shareContext == nullptr) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_shareContext = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    }
    return m_shareContext;
}

void Application::registerWindow(std::unique_ptr<Window>& window) {
    m_windows.push_back(std::move(window));
    syncVsync();
}

void Application::unregisterWindow(Window* window) {
    if (m_windows.size() == 1)
        shutdown();

    std::erase_if(m_windows, [window](const std::unique_ptr<Window>& w) {
        return w.get() == window;
    });
    syncVsync();
}

void Application::syncVsync() {
    if (m_windows.empty()) {
        m_vsyncWindow = nullptr;
        return;
    }
    if (m_windows.front().get() == m_vsyncWindow)
        return;
    m_vsyncWindow = m_windows.front().get();
    glfwMakeContextCurrent(m_vsyncWindow->handle());
    glfwSwapInterval(1);
}

}