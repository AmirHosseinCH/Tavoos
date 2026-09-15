#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/fontenums.h>

#include <memory>
#include <vector>

struct GLFWwindow;

namespace Tavoos {

class Renderer;
class Window;

class TAVOOS_EXPORT Application {
    friend class Window;
    friend class Builder;

public:
    static Application* instance() noexcept;

    Application();
    ~Application();

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    void registerFont(const std::string& familyName, const std::string& sourcePath,
                      Tavoos::FontWeight weight = Tavoos::FontWeight::Regular,
                      Tavoos::FontStyle style = Tavoos::FontStyle::Normal);

    int run();
    void shutdown();

    static void quit();

private:
    static Application* m_instance;
    std::unique_ptr<Renderer> renderer;
    std::vector<std::unique_ptr<Window>> m_windows;
    bool m_shouldQuit{false};

    GLFWwindow* m_shareContext{nullptr};

    void initRenderer();
    void registerWindow(std::unique_ptr<Window>&);
    void unregisterWindow(Window*);
    GLFWwindow* shareContextHandle();
};

}