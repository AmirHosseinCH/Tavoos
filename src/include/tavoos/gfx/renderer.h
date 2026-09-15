#pragma once

#include <tavoos/gfx/shader.h>
#include <tavoos/third_party/glad/glad.h>
#include <tavoos/types.h>

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Tavoos {

class Window;
class Widget;
class RectangleWidget;
class ImageWidget;
class SVGWidget;
class TextWidget;

class Renderer {
    friend class Application;
    friend class Window;

public:
    ~Renderer();

    void init();

    void renderForWindow(Window&);

    void renderRectangle(RectangleWidget&);
    void renderImage(ImageWidget&);
    void renderSVG(SVGWidget& svg);
    void renderText(TextWidget& text);
    void renderWidget(Widget&);

    bool isCapturingMask() const { return m_capturingMask; }

    bool isInit() const { return m_isInit; }

private:
    Renderer() = default;
    void shutdown();

    void applyPaintUniforms(Shader& shader, const std::string& prefix, const std::string& flatColorUniform,
                             const Paint& paint, float opacityMultiplier);
    void drawQuad(float w, float h);

    void ensureClipMask(Widget& widget, int texW, int texH);

    void renderClipLayer(Widget& widget, int texW, int texH);

    void compositeClipLayer(Widget& widget, int texW, int texH);

    void ensureWindowResources(Window& window);
    void releaseWindowResources(Window& window);

    bool m_isInit{false};

    GLuint quadVBO{0};
    GLuint quadVAO{0};
    Shader quadShader{};

    Shader imageShader;
    Shader svgShader;

    Shader textShader;
    GLuint textVAO{0};
    GLuint textVBO{0};

    GLuint clipFBO{0};
    Shader clipCompositeShader;
    bool m_capturingMask{false};

    GLuint cameraUBO{0};
    glm::mat4 projection{1.0f};

    struct WindowGLResources { GLuint quadVAO{0}; GLuint textVAO{0}; GLuint clipFBO{0}; };
    std::unordered_map<Window*, WindowGLResources> m_windowResources;
};

}