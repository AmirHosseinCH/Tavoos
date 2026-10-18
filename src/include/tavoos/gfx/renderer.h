#pragma once

#include <tavoos/gfx/shader.h>
#include <tavoos/third_party/glad/glad.h>
#include <tavoos/types.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace Tavoos {

class Window;
class Widget;
class DropShadow;
class InnerShadow;
class ShadowBase;
struct BlurStyle;
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
    void renderDropShadow(Widget&, const DropShadow&);
    void renderInnerShadow(Widget&, const InnerShadow&);

    bool isCapturingMask() const { return m_capturingMask; }
    bool isCapturingOwn() const { return m_capturingMask || m_capturingOwn; }

    bool isInit() const { return m_isInit; }

private:
    Renderer() = default;
    void shutdown();

    void applyPaintUniforms(Shader& shader, const std::string& prefix, const std::string& flatColorUniform,
                             const Paint& paint, float opacityMultiplier);
    void drawQuad(float w, float h);
    void drawQuad(float x0, float y0, float x1, float y1);

    void ensureClipMask(Widget& widget, int texW, int texH);

    void renderClipLayer(Widget& widget, int texW, int texH);

    void compositeClipLayer(Widget& widget, int texW, int texH);

    void renderAlphaShadow(Widget& widget, const ShadowBase& shadow, bool inner);
    void renderWidgetContents(Widget& widget);
    void renderLayerBlur(Widget& widget, const BlurStyle& blur);
    void renderBackgroundBlur(Widget& widget, const BlurStyle& blur);
    GLuint blurLayerTexture(GLuint source, int texW, int texH, int width, int height, int pad, int taps,
                            const BlurStyle& blur);
    void captureWidget(Widget& widget, GLuint texture, int texW, int texH, int pad, bool ownOnly);
    void captureCoverage(Widget& widget, GLuint texture, int texW, int texH);
    void runPass(Shader& shader, GLuint source, GLuint target, int w, int h,
                 const std::function<void(Shader&)>& setup);
    GLuint blurAlpha(GLuint source, int w, int h, float sigma, int taps);
    GLuint morphAlpha(GLuint source, int w, int h, int radius, bool erode);
    GLuint acquireTexture(int w, int h);
    void releaseTexture(GLuint texture);

    void ensureWindowResources(Window& window);
    void releaseWindowResources(Window& window);

    bool m_isInit{false};

    GLuint quadVBO{0};
    GLuint quadVAO{0};
    Shader quadShader{};
    Shader shadowShader;
    Shader innerShadowShader;
    Shader effectBlurShader;
    Shader effectMorphShader;
    Shader alphaDropShadowShader;
    Shader alphaInnerShadowShader;
    Shader layerBlurShader;
    Shader layerBlurCompositeShader;
    Shader backdropResampleShader;
    Shader backgroundBlurCompositeShader;

    struct PooledTexture { GLuint id{0}; int width{0}; int height{0}; bool busy{false}; };
    std::vector<PooledTexture> m_texturePool;

    Shader imageShader;
    Shader svgShader;

    Shader textShader;
    GLuint textVAO{0};
    GLuint textVBO{0};

    GLuint clipFBO{0};
    Shader clipCompositeShader;
    bool m_capturingMask{false};
    bool m_capturingOwn{false};

    GLuint cameraUBO{0};
    glm::mat4 projection{1.0f};

    struct WindowGLResources { GLuint quadVAO{0}; GLuint textVAO{0}; GLuint clipFBO{0}; };
    std::unordered_map<Window*, WindowGLResources> m_windowResources;
};

}