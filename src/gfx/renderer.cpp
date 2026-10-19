#include <tavoos/effects/blurstyle.h>
#include <tavoos/effects/dropshadow.h>
#include <tavoos/effects/glassstyle.h>
#include <tavoos/effects/innershadow.h>
#include <tavoos/effects/shadowbase.h>
#include <tavoos/gfx/renderer.h>
#include <tavoos/gfx/shader_sources.h>
#include <tavoos/text/text.h>
#include <tavoos/widget/image.h>
#include <tavoos/widget/rectangle.h>
#include <tavoos/widget/svg.h>
#include <tavoos/widget/templates/overlaybase.h>
#include <tavoos/widget/text.h>
#include <tavoos/widget/widget.h>
#include <tavoos/window.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace Tavoos {

Renderer::~Renderer() {}

void Renderer::init() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error{"failed to initialize GLAD: unable to load OpenGL function pointers"};
    }

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &textVBO);

    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &projection, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);

    std::string quadFragmentSource = std::string(fragmentShaderPrelude) + quadFragmentShaderHead + roundedBoxGLSL + paintSamplingGLSL + quadFragmentShaderTail;
    std::string shadowFragmentSource = std::string(fragmentShaderPrelude) + roundedBoxGLSL + blurredBoxGLSL + shadowFragmentShader;
    std::string innerShadowFragmentSource = std::string(fragmentShaderPrelude) + roundedBoxGLSL + blurredBoxGLSL + innerShadowFragmentShader;
    std::string svgFragmentSource  = std::string(fragmentShaderPrelude) + svgFragmentShaderHead  + paintSamplingGLSL + svgFragmentShaderTail;
    std::string textFragmentSource = std::string(fragmentShaderPrelude) + textFragmentShaderHead + paintSamplingGLSL + textFragmentShaderTail;

    quadShader.compile(quadVertexShader, quadFragmentSource);
    shadowShader.compile(quadVertexShader, shadowFragmentSource);
    innerShadowShader.compile(quadVertexShader, innerShadowFragmentSource);
    effectBlurShader.compile(clipCompositeVertexShader, effectBlurFragmentShader);
    effectMorphShader.compile(clipCompositeVertexShader, effectMorphFragmentShader);
    alphaDropShadowShader.compile(effectCompositeVertexShader, alphaDropShadowFragmentShader);
    alphaInnerShadowShader.compile(effectCompositeVertexShader, alphaInnerShadowFragmentShader);
    layerBlurShader.compile(clipCompositeVertexShader, layerBlurFragmentShader);
    layerBlurCompositeShader.compile(effectCompositeVertexShader, layerBlurCompositeFragmentShader);
    backdropResampleShader.compile(clipCompositeVertexShader, backdropResampleFragmentShader);
    backgroundBlurCompositeShader.compile(
        effectCompositeVertexShader,
        std::string(fragmentShaderPrelude) + roundedBoxGLSL + backgroundBlurCompositeFragmentShader);
    glassCompositeShader.compile(
        effectCompositeVertexShader,
        std::string(fragmentShaderPrelude) + roundedBoxGLSL + glassCompositeFragmentShader);
    imageShader.compile(imageVertexShader, imageFragmentShader);
    svgShader.compile(svgVertexShader, svgFragmentSource);
    textShader.compile(textVertexShader, textFragmentSource);
    clipCompositeShader.compile(clipCompositeVertexShader, clipCompositeFragmentShader);

    m_isInit = true;
}

void Renderer::ensureWindowResources(Window& window) {
    auto it = m_windowResources.find(&window);
    if (it != m_windowResources.end()) {
        quadVAO = it->second.quadVAO;
        textVAO = it->second.textVAO;
        clipFBO = it->second.clipFBO;
        return;
    }

    WindowGLResources res;

    glGenVertexArrays(1, &res.quadVAO);
    glBindVertexArray(res.quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &res.textVAO);
    glBindVertexArray(res.textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    glGenFramebuffers(1, &res.clipFBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    quadVAO = res.quadVAO;
    textVAO = res.textVAO;
    clipFBO = res.clipFBO;
    m_windowResources[&window] = res;
}

void Renderer::releaseWindowResources(Window& window) {
    auto it = m_windowResources.find(&window);
    if (it == m_windowResources.end())
        return;

    glDeleteVertexArrays(1, &it->second.quadVAO);
    glDeleteVertexArrays(1, &it->second.textVAO);
    glDeleteFramebuffers(1, &it->second.clipFBO);
    m_windowResources.erase(it);
}

void Renderer::renderForWindow(Window& window) {
    if (!window.consumeDirty())
        return;

    glfwMakeContextCurrent(window.handle());
    window.flushPendingDestruction();
    ensureWindowResources(window);

    glViewport(0, 0, window.framebufferWidth(), window.framebufferHeight());

    projection = glm::ortho(0.0f, static_cast<float>(window.width()),
                            static_cast<float>(window.height()), 0.0f,
                            -1.0f, 1.0f);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection);

    Color backgroundColor = window.color();
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& child : window.children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout();
    }

    for (Widget* const widget : Widget::zOrderedChildren(window)) {
        if (widget->visible())
            renderWidget(*widget);
    }

    std::vector<OverlayBase*> overlays = window.retiredOverlays();
    overlays.insert(overlays.end(), window.overlays().begin(), window.overlays().end());
    for (OverlayBase* const overlay : overlays) {
        overlay->place();
        overlay->layout(true);
    }
    for (OverlayBase* const overlay : overlays) {
        if (overlay->visible())
            renderWidget(*overlay);
    }

    glfwSwapBuffers(window.handle());
}

void Renderer::applyPaintUniforms(Shader& shader, const std::string& prefix, const std::string& flatColorUniform,
                                   const Paint& paint, float opacityMultiplier) {
    constexpr int kMaxGradientStops = 8;

    const bool isGradient = paint.kind != Paint::Kind::Solid && paint.stops.size() >= 2;
    int kindValue = 0;
    if (isGradient) {
        switch (paint.kind) {
            case Paint::Kind::LinearGradient: kindValue = 1; break;
            case Paint::Kind::RadialGradient: kindValue = 2; break;
            case Paint::Kind::ConicGradient:  kindValue = 3; break;
            case Paint::Kind::Solid: break;
        }
    }
    shader.setInt(prefix + "PaintKind", kindValue);

    if (!isGradient) {
        const Color c = paint.solid;
        shader.setVec4(flatColorUniform, glm::vec4{c.r, c.g, c.b, c.a * opacityMultiplier});
        shader.setInt(prefix + "StopCount", 0);
        return;
    }

    const int count = std::min<int>(static_cast<int>(paint.stops.size()), kMaxGradientStops);
    std::array<glm::vec4, kMaxGradientStops> colors{};
    std::array<float, kMaxGradientStops> positions{};
    for (int i = 0; i < count; ++i) {
        const Paint::Stop& stop = paint.stops[i];
        colors[i] = glm::vec4{stop.color.r, stop.color.g, stop.color.b, stop.color.a * opacityMultiplier};
        positions[i] = stop.position;
    }

    shader.setVec4Array(prefix + "StopColors", colors.data(), count);
    shader.setFloatArray(prefix + "StopPositions", positions.data(), count);
    shader.setInt(prefix + "StopCount", count);
    shader.setFloat(prefix + "GradientAngle", glm::radians(paint.angle));
    shader.setVec2(prefix + "GradientCenter", glm::vec2{paint.centerX, paint.centerY});
    shader.setFloat(prefix + "GradientRadius", paint.radius);
}

void Renderer::drawQuad(float w, float h) {
    drawQuad(0.0f, 0.0f, w, h);
}

void Renderer::drawQuad(float x0, float y0, float x1, float y1) {
    const float vertices[] = { x0, y0, x1, y0, x1, y1, x0, y0, x1, y1, x0, y1 };
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::renderRectangle(RectangleWidget& rectangle) {
    quadShader.use();
    quadShader.setInt("uMaskMode", m_capturingMask ? 1 : 0);
    quadShader.setMat4("world", rectangle.worldMatrix());
    quadShader.setVec2("uHalfSize", glm::vec2{rectangle.displayedWidth(), rectangle.displayedHeight()} * 0.5f);
    quadShader.setVec4("uRadius", glm::vec4{rectangle.radiusTopLeft(), rectangle.radiusTopRight(),
                                             rectangle.radiusBottomRight(), rectangle.radiusBottomLeft()});

    const float op = rectangle.effectiveOpacity();
    applyPaintUniforms(quadShader, "u", "uColor", rectangle.color(), op);
    applyPaintUniforms(quadShader, "uBorder", "uBorderColor", rectangle.borderColor(), op);
    quadShader.setFloat("uBorderWidth", rectangle.borderWidth());

    drawQuad(rectangle.displayedWidth(), rectangle.displayedHeight());
}

void Renderer::renderDropShadow(Widget& widget, const DropShadow& shadow) {
    const std::optional<CornerRadii> radii = widget.shapeRadii();
    const float width = widget.displayedWidth();
    const float height = widget.displayedHeight();
    if (!radii) {
        renderAlphaShadow(widget, shadow, false);
        return;
    }
    if (width <= 0.0f || height <= 0.0f)
        return;

    const Color color = shadow.color();
    const float alpha = color.a * widget.effectiveOpacity();
    if (alpha <= 0.0f)
        return;

    const float blur = std::max(0.0f, shadow.radius());
    const float sigma = blur * 0.5f;
    const float reach = std::max(shadow.spread(), 0.0f) + sigma * 3.0f + 1.0f;

    shadowShader.use();
    shadowShader.setMat4("world", widget.worldMatrix());
    shadowShader.setVec2("uHalfSize", glm::vec2{width, height} * 0.5f);
    shadowShader.setVec4("uRadius", glm::vec4{radii->topLeft, radii->topRight, radii->bottomRight, radii->bottomLeft});
    shadowShader.setVec2("uOffset", glm::vec2{shadow.offsetX(), shadow.offsetY()});
    shadowShader.setFloat("uSpread", shadow.spread());
    shadowShader.setFloat("uSigma", sigma);
    shadowShader.setVec4("uColor", glm::vec4{color.r, color.g, color.b, alpha});

    drawQuad(shadow.offsetX() - reach, shadow.offsetY() - reach,
             width + shadow.offsetX() + reach, height + shadow.offsetY() + reach);
}

void Renderer::renderInnerShadow(Widget& widget, const InnerShadow& shadow) {
    const std::optional<CornerRadii> radii = widget.shapeRadii();
    const float width = widget.displayedWidth();
    const float height = widget.displayedHeight();
    if (!radii) {
        renderAlphaShadow(widget, shadow, true);
        return;
    }
    if (width <= 0.0f || height <= 0.0f)
        return;

    const Color color = shadow.color();
    const float alpha = color.a * widget.effectiveOpacity();
    if (alpha <= 0.0f)
        return;

    innerShadowShader.use();
    innerShadowShader.setMat4("world", widget.worldMatrix());
    innerShadowShader.setVec2("uHalfSize", glm::vec2{width, height} * 0.5f);
    innerShadowShader.setVec4("uRadius", glm::vec4{radii->topLeft, radii->topRight, radii->bottomRight, radii->bottomLeft});
    innerShadowShader.setVec2("uOffset", glm::vec2{shadow.offsetX(), shadow.offsetY()});
    innerShadowShader.setFloat("uSpread", shadow.spread());
    innerShadowShader.setFloat("uSigma", std::max(0.0f, shadow.radius()) * 0.5f);
    innerShadowShader.setVec4("uColor", glm::vec4{color.r, color.g, color.b, alpha});

    drawQuad(width, height);
}

void Renderer::renderImage(ImageWidget& image) {
    if (image.texture() == 0)
        return;

    imageShader.use();
    imageShader.setInt("uMaskMode", m_capturingMask ? 1 : 0);
    imageShader.setMat4("world", image.worldMatrix());
    imageShader.setVec2("uRectSize", glm::vec2{image.displayedWidth(), image.displayedHeight()});
    imageShader.setFloat("uOpacity", image.effectiveOpacity());

    const float boxW = image.displayedWidth();
    const float boxH = image.displayedHeight();
    const float natW = static_cast<float>(image.naturalWidth());
    const float natH = static_cast<float>(image.naturalHeight());

    glm::vec2 uvScale{1.0f, 1.0f};
    glm::vec2 uvOffset{0.0f, 0.0f};
    bool useDiscard = false;

    if (natW > 0.0f && natH > 0.0f && boxW > 0.0f && boxH > 0.0f) {
        const float boxAspect = boxW / boxH;
        const float imgAspect = natW / natH;

        switch (image.fillMode()) {
        case ImageFillMode::Stretch:
            break;

        case ImageFillMode::PreserveAspectFit: {
            useDiscard = true;
            const bool wideBasis = (imgAspect > boxAspect);
            if (wideBasis) {
                const float shownHeight = boxW / imgAspect;
                uvScale.y = boxH / shownHeight;
                uvOffset.y = -(uvScale.y - 1.0f) * 0.5f;
            } else {
                const float shownWidth = boxH * imgAspect;
                uvScale.x = boxW / shownWidth;
                uvOffset.x = -(uvScale.x - 1.0f) * 0.5f;
            }
            break;
        }

        case ImageFillMode::PreserveAspectCrop: {
            const bool wideBasis = (imgAspect < boxAspect);
            if (wideBasis) {
                const float shownHeight = boxW / imgAspect;
                uvScale.y = boxH / shownHeight;
                uvOffset.y = -(uvScale.y - 1.0f) * 0.5f;
            } else {
                const float shownWidth = boxH * imgAspect;
                uvScale.x = boxW / shownWidth;
                uvOffset.x = -(uvScale.x - 1.0f) * 0.5f;
            }
            break;
        }

        case ImageFillMode::Tile:
            uvScale = { boxW / natW, boxH / natH };
            break;

        case ImageFillMode::TileVertically:
            uvScale = { 1.0f, boxH / natH };
            break;

        case ImageFillMode::TileHorizontally:
            uvScale = { boxW / natW, 1.0f };
            break;
        }
    }

    imageShader.setVec2("uUVScale", uvScale);
    imageShader.setVec2("uUVOffset", uvOffset);
    imageShader.setInt("uDiscardOutOfRange", useDiscard);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image.texture());
    imageShader.setInt("uTexture", 0);

    drawQuad(boxW, boxH);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderSVG(SVGWidget& svg) {
    if (svg.texture() == 0)
        return;

    svgShader.use();
    svgShader.setInt("uMaskMode", m_capturingMask ? 1 : 0);
    svgShader.setMat4("world", svg.worldMatrix());
    svgShader.setVec2("uRectSize", glm::vec2{svg.displayedWidth(), svg.displayedHeight()});
    svgShader.setInt("uHasColor", svg.hasColor() ? 1 : 0);
    svgShader.setFloat("uOpacity", svg.effectiveOpacity());
    if (svg.hasColor()) {
        applyPaintUniforms(svgShader, "u", "uColor", svg.color(), 1.0f);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, svg.texture());
    svgShader.setInt("uTexture", 0);

    drawQuad(svg.displayedWidth(), svg.displayedHeight());
}

void Renderer::renderText(TextWidget& text) {
    FontFace* const face = text.resolvedFace();
    if (!face) return;

    const std::string content = text.text();
    if (content.empty()) return;

    const float scale = text.fontSize() / FontFace::ReferencePixelSize;
    const float lineHeight = face->lineHeight() * scale;

    const float wrapWidth = (text.wrapMode() != WrapMode::NoWrap)
                          ? text.resolvedWidth() - text.paddingLeft() - text.paddingRight()
                          : 0.0f;
    const float elideWidth = text.resolvedWidth() - text.paddingLeft() - text.paddingRight();

    const auto& lines = text.layoutLines(text.m_renderLineCache, *face, scale,
                                          wrapWidth, elideWidth, text.resolvedHeight());

    float blockWidth = 0.0f;
    for (auto& line : lines)
        blockWidth = std::max(blockWidth, line.width);
    const float blockHeight = lineHeight * static_cast<float>(lines.size());

    const float boxWidth  = text.resolvedWidth()  - text.paddingLeft() - text.paddingRight();
    const float boxHeight = text.resolvedHeight() - text.paddingTop()  - text.paddingBottom();

    const Alignment align = text.textAlignment();

    auto lineStartX = [&](std::size_t idx) -> float {
        float w = lines[idx].width;
        if (hasFlag(align, Alignment::Right))            return text.paddingLeft() + boxWidth - w;
        if (hasFlag(align, Alignment::CenterHorizontal))  return text.paddingLeft() + (boxWidth - w) * 0.5f;
        return text.paddingLeft();
    };

    float blockStartY = text.paddingTop();
    if (hasFlag(align, Alignment::Bottom))
        blockStartY = text.paddingTop() + boxHeight - blockHeight;
    else if (hasFlag(align, Alignment::CenterVertical))
        blockStartY = text.paddingTop() + (boxHeight - blockHeight) * 0.5f;

    struct Vertex { float x, y, u, v, lu, lv; };
    std::vector<Vertex> vertices;
    vertices.reserve(content.size() * 6);
    int currentPage = -1;

    textShader.use();
    textShader.setInt("uMaskMode", m_capturingMask ? 1 : 0);
    textShader.setMat4("world", text.worldMatrix());
    const float op = text.effectiveOpacity();
    applyPaintUniforms(textShader, "u", "uColor", text.color(), op);

    const float boxW = text.resolvedWidth();
    const float boxH = text.resolvedHeight();

    auto flush = [&]() {
        if (vertices.empty() || currentPage < 0) return;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, face->atlasTexture(currentPage));
        textShader.setInt("uAtlas", 0);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);
        vertices.clear();
    };

    auto emitGlyph = [&](const GlyphMetrics& m, float& penX, float baselineY) {
        if (m.atlasPage >= 0) {
            if (m.atlasPage != currentPage) { flush(); currentPage = m.atlasPage; }
            float gx0 = penX + m.bearingX * scale;
            float gy0 = baselineY - m.bearingY * scale;
            float gx1 = gx0 + m.width * scale;
            float gy1 = gy0 + m.height * scale;

            float lu0 = boxW > 0.0f ? gx0 / boxW : 0.0f;
            float lv0 = boxH > 0.0f ? gy0 / boxH : 0.0f;
            float lu1 = boxW > 0.0f ? gx1 / boxW : 0.0f;
            float lv1 = boxH > 0.0f ? gy1 / boxH : 0.0f;

            vertices.push_back({gx0, gy0, m.u0, m.v0, lu0, lv0});
            vertices.push_back({gx1, gy0, m.u1, m.v0, lu1, lv0});
            vertices.push_back({gx1, gy1, m.u1, m.v1, lu1, lv1});
            vertices.push_back({gx0, gy0, m.u0, m.v0, lu0, lv0});
            vertices.push_back({gx1, gy1, m.u1, m.v1, lu1, lv1});
            vertices.push_back({gx0, gy1, m.u0, m.v1, lu0, lv1});
        }
        penX += m.advance * scale;
    };

    for (std::size_t li = 0; li < lines.size(); ++li) {
        float penX = lineStartX(li);
        float baselineY = blockStartY + face->ascender() * scale + lineHeight * static_cast<float>(li);

        const std::string& source = lines[li].overrideText.empty() ? content : lines[li].overrideText;
        std::size_t i = lines[li].overrideText.empty() ? lines[li].byteStart : 0;
        std::size_t end = lines[li].overrideText.empty() ? lines[li].byteEnd : source.size();

        while (i < end) {
            Utf8Decoded d = decodeUtf8At(source, i);
            i += d.length;
            const GlyphMetrics& m = face->glyph(d.codepoint);
            if (!m.valid) continue;
            emitGlyph(m, penX, baselineY);
        }
    }

    flush();
}

void Renderer::renderWidget(Widget& widget) {
    if (!isCapturingOwn()) {
        if (widget.hasGlass() && widget.shapeRadii()) {
            renderGlass(widget, widget.glass());
        } else {
            const BlurStyle backdrop = widget.backgroundBlur();
            if (std::max(backdrop.startRadius, backdrop.endRadius) > 0.01f)
                renderBackgroundBlur(widget, backdrop);
        }

        for (const auto& effect : widget.effects())
            effect->renderBehind(*this, widget);
    }

    if (!isCapturingOwn()) {
        const BlurStyle blur = widget.layerBlur();
        if (std::max(blur.startRadius, blur.endRadius) > 0.01f) {
            renderLayerBlur(widget, blur);
            return;
        }
    }

    renderWidgetContents(widget);
}

void Renderer::renderWidgetContents(Widget& widget) {
    if (!widget.clip()) {
        widget.render(*this);
        return;
    }

    const int texW = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int texH = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    if (texW <= 0 || texH <= 0)
        return;

    ensureClipMask(widget, texW, texH);
    renderClipLayer(widget, texW, texH);
    compositeClipLayer(widget, texW, texH);
}

static void setClipTextureParams() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

struct SavedRenderTarget { GLint fbo; GLint attachedTexture; GLint viewport[4]; glm::mat4 projection; };

template <typename F>
struct ScopeExit {
    F f;
    ~ScopeExit() { f(); }
};
template <typename F> ScopeExit(F) -> ScopeExit<F>;

static SavedRenderTarget beginClipTarget(GLuint clipFBO, GLuint cameraUBO, glm::mat4& projectionRef,
                                          GLuint texture, int texW, int texH, const glm::mat4& worldMatrix,
                                          float padding = 0.0f) {
    SavedRenderTarget saved;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saved.fbo);
    glGetIntegerv(GL_VIEWPORT, saved.viewport);
    saved.projection = projectionRef;

    saved.attachedTexture = 0;
    if (static_cast<GLuint>(saved.fbo) == clipFBO) {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &saved.attachedTexture);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, clipFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        spdlog::warn("clip framebuffer incomplete ({}x{})", texW, texH);
    glViewport(0, 0, texW, texH);

    projectionRef = glm::ortho(-padding, static_cast<float>(texW) - padding,
                                -padding, static_cast<float>(texH) - padding,
                                -1.0f, 1.0f) * glm::inverse(worldMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projectionRef);

    return saved;
}

static void endClipTarget(const SavedRenderTarget& saved, GLuint clipFBO, GLuint cameraUBO, glm::mat4& projectionRef) {
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(saved.fbo));
    if (static_cast<GLuint>(saved.fbo) == clipFBO) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                static_cast<GLuint>(saved.attachedTexture), 0);
    }
    glViewport(saved.viewport[0], saved.viewport[1], saved.viewport[2], saved.viewport[3]);
    projectionRef = saved.projection;
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projectionRef);
}

void Renderer::ensureClipMask(Widget& widget, int texW, int texH) {
    if (widget.m_clipMaskTexture == 0)
        glGenTextures(1, &widget.m_clipMaskTexture);

    if (widget.m_clipMaskWidth != texW || widget.m_clipMaskHeight != texH) {
        glBindTexture(GL_TEXTURE_2D, widget.m_clipMaskTexture);
        setClipTextureParams();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texW, texH, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        widget.m_clipMaskWidth = texW;
        widget.m_clipMaskHeight = texH;
    }

    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection, widget.m_clipMaskTexture, texW, texH, widget.worldMatrix());
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    if (widget.isLayouter())
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    else
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint prevEquation;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &prevEquation);

    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX);
    const bool prevCapturingMask = m_capturingMask;
    m_capturingMask = true;
    ScopeExit restoreCaptureState{[&] {
        m_capturingMask = prevCapturingMask;
        glBlendEquation(static_cast<GLenum>(prevEquation));
    }};

    widget.render(*this);
}

void Renderer::renderClipLayer(Widget& widget, int texW, int texH) {
    if (widget.m_clipColorTexture == 0)
        glGenTextures(1, &widget.m_clipColorTexture);

    if (widget.m_clipColorWidth != texW || widget.m_clipColorHeight != texH) {
        glBindTexture(GL_TEXTURE_2D, widget.m_clipColorTexture);
        setClipTextureParams();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        widget.m_clipColorWidth = texW;
        widget.m_clipColorHeight = texH;
    }

    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection, widget.m_clipColorTexture, texW, texH, widget.worldMatrix());
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ScopeExit restoreBlendFunc{[&] {
        glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                             static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));
    }};

    widget.render(*this);
}

static bool isAxisAlignedUnitScale(const glm::mat4& m) {
    constexpr float eps = 1e-4f;
    return std::abs(m[0][0] - 1.0f) < eps && std::abs(m[0][1]) < eps &&
           std::abs(m[1][0]) < eps && std::abs(m[1][1] - 1.0f) < eps;
}

void Renderer::compositeClipLayer(Widget& widget, int texW, int texH) {
    clipCompositeShader.use();

    glm::mat4 world = widget.worldMatrix();
    if (isAxisAlignedUnitScale(world)) {
        world[3].x = std::round(world[3].x);
        world[3].y = std::round(world[3].y);
    }
    clipCompositeShader.setMat4("world", world);
    clipCompositeShader.setVec2("uRectSize", glm::vec2{static_cast<float>(texW), static_cast<float>(texH)});

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, widget.m_clipColorTexture);
    clipCompositeShader.setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, widget.m_clipMaskTexture);
    clipCompositeShader.setInt("uMask", 1);
    glActiveTexture(GL_TEXTURE0);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(static_cast<float>(texW), static_cast<float>(texH));

    glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                         static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));
}

GLuint Renderer::acquireTexture(int w, int h) {
    constexpr std::size_t kMaxPooledTextures = 24;

    for (PooledTexture& entry : m_texturePool) {
        if (!entry.busy && entry.width == w && entry.height == h) {
            entry.busy = true;
            return entry.id;
        }
    }

    if (m_texturePool.size() >= kMaxPooledTextures) {
        std::erase_if(m_texturePool, [](const PooledTexture& entry) {
            if (entry.busy)
                return false;
            glDeleteTextures(1, &entry.id);
            return true;
        });
    }

    PooledTexture entry;
    entry.width = w;
    entry.height = h;
    entry.busy = true;
    glGenTextures(1, &entry.id);
    glBindTexture(GL_TEXTURE_2D, entry.id);
    setClipTextureParams();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_texturePool.push_back(entry);
    return entry.id;
}

void Renderer::releaseTexture(GLuint texture) {
    for (PooledTexture& entry : m_texturePool) {
        if (entry.id == texture)
            entry.busy = false;
    }
}

void Renderer::captureWidget(Widget& widget, GLuint texture, int texW, int texH, int pad, bool ownOnly) {
    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection, texture, texW, texH,
                                                    widget.worldMatrix(), static_cast<float>(pad));
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    const bool prevCapturingOwn = m_capturingOwn;
    m_capturingOwn = ownOnly;
    ScopeExit restoreState{[&] {
        m_capturingOwn = prevCapturingOwn;
        glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                             static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));
    }};

    if (ownOnly)
        widget.render(*this);
    else
        renderWidgetContents(widget);
}

void Renderer::captureCoverage(Widget& widget, GLuint texture, int texW, int texH) {
    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection, texture, texW, texH,
                                                    widget.worldMatrix());
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint prevEquation;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &prevEquation);
    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX);

    const bool prevCapturingMask = m_capturingMask;
    m_capturingMask = true;
    ScopeExit restoreState{[&] {
        m_capturingMask = prevCapturingMask;
        glBlendEquation(static_cast<GLenum>(prevEquation));
    }};

    widget.render(*this);
}

void Renderer::runPass(Shader& shader, GLuint source, GLuint target, int w, int h,
                       const std::function<void(Shader&)>& setup) {
    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection, target, w, h, glm::mat4{1.0f});
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glDisable(GL_BLEND);
    ScopeExit restoreBlend{[&] {
        if (blendWasEnabled)
            glEnable(GL_BLEND);
    }};

    shader.use();
    shader.setMat4("world", glm::mat4{1.0f});
    shader.setVec2("uRectSize", glm::vec2{static_cast<float>(w), static_cast<float>(h)});

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    shader.setInt("uSource", 0);
    setup(shader);

    drawQuad(static_cast<float>(w), static_cast<float>(h));
}

GLuint Renderer::blurAlpha(GLuint source, int w, int h, float sigma, int taps) {
    const GLuint horizontal = acquireTexture(w, h);
    runPass(effectBlurShader, source, horizontal, w, h, [&](Shader& shader) {
        shader.setVec2("uStep", glm::vec2{1.0f / static_cast<float>(w), 0.0f});
        shader.setFloat("uSigma", sigma);
        shader.setInt("uTaps", taps);
    });

    const GLuint result = acquireTexture(w, h);
    runPass(effectBlurShader, horizontal, result, w, h, [&](Shader& shader) {
        shader.setVec2("uStep", glm::vec2{0.0f, 1.0f / static_cast<float>(h)});
        shader.setFloat("uSigma", sigma);
        shader.setInt("uTaps", taps);
    });
    releaseTexture(horizontal);
    return result;
}

GLuint Renderer::morphAlpha(GLuint source, int w, int h, int radius, bool erode) {
    const GLuint horizontal = acquireTexture(w, h);
    runPass(effectMorphShader, source, horizontal, w, h, [&](Shader& shader) {
        shader.setVec2("uStep", glm::vec2{1.0f / static_cast<float>(w), 0.0f});
        shader.setInt("uRadius", radius);
        shader.setInt("uErode", erode ? 1 : 0);
    });

    const GLuint result = acquireTexture(w, h);
    runPass(effectMorphShader, horizontal, result, w, h, [&](Shader& shader) {
        shader.setVec2("uStep", glm::vec2{0.0f, 1.0f / static_cast<float>(h)});
        shader.setInt("uRadius", radius);
        shader.setInt("uErode", erode ? 1 : 0);
    });
    releaseTexture(horizontal);
    return result;
}

GLuint Renderer::blurLayerTexture(GLuint source, int texW, int texH, int width, int height, int pad, int taps,
                                  const BlurStyle& blur) {
    const float startSigma = std::max(0.0f, blur.startRadius) * 0.5f;
    const float endSigma = std::max(0.0f, blur.endRadius) * 0.5f;
    const float angle = glm::radians(blur.direction);
    const glm::vec2 direction{std::cos(angle), std::sin(angle)};
    auto configure = [&](Shader& shader, const glm::vec2& step) {
        shader.setVec2("uStep", step);
        shader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
        shader.setVec2("uBoxSize", glm::vec2{static_cast<float>(width), static_cast<float>(height)});
        shader.setVec2("uSigmas", glm::vec2{startSigma, endSigma});
        shader.setVec2("uDirection", direction);
        shader.setInt("uMaxTaps", taps);
    };

    const GLuint horizontal = acquireTexture(texW, texH);
    runPass(layerBlurShader, source, horizontal, texW, texH, [&](Shader& shader) {
        configure(shader, glm::vec2{1.0f / static_cast<float>(texW), 0.0f});
    });

    const GLuint result = acquireTexture(texW, texH);
    runPass(layerBlurShader, horizontal, result, texW, texH, [&](Shader& shader) {
        configure(shader, glm::vec2{0.0f, 1.0f / static_cast<float>(texH)});
    });
    releaseTexture(horizontal);
    return result;
}

GLuint Renderer::captureBackdrop(Widget& widget, int texW, int texH, int pad) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const glm::mat4 mvp = projection * widget.worldMatrix();

    const float lo = -static_cast<float>(pad);
    const glm::vec2 corners[4] = {
        {lo, lo}, {lo + static_cast<float>(texW), lo},
        {lo + static_cast<float>(texW), lo + static_cast<float>(texH)}, {lo, lo + static_cast<float>(texH)},
    };
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    for (const glm::vec2& corner : corners) {
        const glm::vec4 clip = mvp * glm::vec4{corner, 0.0f, 1.0f};
        const float px = static_cast<float>(viewport[0]) + (clip.x / clip.w * 0.5f + 0.5f) * static_cast<float>(viewport[2]);
        const float py = static_cast<float>(viewport[1]) + (clip.y / clip.w * 0.5f + 0.5f) * static_cast<float>(viewport[3]);
        minX = std::min(minX, px);
        maxX = std::max(maxX, px);
        minY = std::min(minY, py);
        maxY = std::max(maxY, py);
    }
    const int regionX = std::max(viewport[0], static_cast<int>(std::floor(minX)));
    const int regionY = std::max(viewport[1], static_cast<int>(std::floor(minY)));
    const int regionW = std::min(viewport[0] + viewport[2], static_cast<int>(std::ceil(maxX))) - regionX;
    const int regionH = std::min(viewport[1] + viewport[3], static_cast<int>(std::ceil(maxY))) - regionY;
    if (regionW <= 0 || regionH <= 0)
        return 0;

    GLint targetFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &targetFramebuffer);
    const bool opaque = targetFramebuffer == 0;

    const GLuint backdrop = acquireTexture(regionW, regionH);
    glBindTexture(GL_TEXTURE_2D, backdrop);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, regionX, regionY, regionW, regionH);
    glBindTexture(GL_TEXTURE_2D, 0);

    const GLuint local = acquireTexture(texW, texH);
    runPass(backdropResampleShader, backdrop, local, texW, texH, [&](Shader& shader) {
        shader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
        shader.setMat4("uMvp", mvp);
        shader.setVec4("uViewport", glm::vec4{static_cast<float>(viewport[0]), static_cast<float>(viewport[1]),
                                              static_cast<float>(viewport[2]), static_cast<float>(viewport[3])});
        shader.setVec4("uRegion", glm::vec4{static_cast<float>(regionX), static_cast<float>(regionY),
                                            static_cast<float>(regionW), static_cast<float>(regionH)});
        shader.setInt("uOpaque", opaque ? 1 : 0);
    });
    releaseTexture(backdrop);
    return local;
}

void Renderer::renderBackgroundBlur(Widget& widget, const BlurStyle& blur) {
    const int width = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int height = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    const float opacity = widget.effectiveOpacity();
    if (width <= 0 || height <= 0 || opacity <= 0.0f)
        return;

    const float maxSigma = std::max(std::max(0.0f, blur.startRadius), std::max(0.0f, blur.endRadius)) * 0.5f;
    const int taps = static_cast<int>(std::ceil(maxSigma * 3.0f));
    const int pad = taps + 1;
    const int texW = width + 2 * pad;
    const int texH = height + 2 * pad;

    const GLuint local = captureBackdrop(widget, texW, texH, pad);
    if (local == 0)
        return;

    const GLuint blurred = blurLayerTexture(local, texW, texH, width, height, pad, taps, blur);

    const std::optional<CornerRadii> radii = widget.shapeRadii();
    GLuint mask = 0;
    if (!radii) {
        mask = acquireTexture(width, height);
        captureCoverage(widget, mask, width, height);
    }

    backgroundBlurCompositeShader.use();
    backgroundBlurCompositeShader.setMat4("world", widget.worldMatrix());
    backgroundBlurCompositeShader.setVec2("uRectSize", glm::vec2{static_cast<float>(texW), static_cast<float>(texH)});
    backgroundBlurCompositeShader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
    backgroundBlurCompositeShader.setVec2("uOffset", glm::vec2{0.0f, 0.0f});
    backgroundBlurCompositeShader.setVec2("uBoxSize", glm::vec2{static_cast<float>(width), static_cast<float>(height)});
    backgroundBlurCompositeShader.setVec2("uHalfSize", glm::vec2{widget.displayedWidth(), widget.displayedHeight()} * 0.5f);
    backgroundBlurCompositeShader.setVec4("uRadius", radii ? glm::vec4{radii->topLeft, radii->topRight, radii->bottomRight, radii->bottomLeft} : glm::vec4{0.0f});
    backgroundBlurCompositeShader.setInt("uMaskMode", radii ? 0 : 1);
    backgroundBlurCompositeShader.setFloat("uOpacity", opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurred);
    backgroundBlurCompositeShader.setInt("uBlurred", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mask != 0 ? mask : blurred);
    backgroundBlurCompositeShader.setInt("uMask", 1);
    glActiveTexture(GL_TEXTURE0);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(widget.displayedWidth(), widget.displayedHeight());

    glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                         static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));

    if (mask != 0)
        releaseTexture(mask);
    releaseTexture(blurred);
    releaseTexture(local);
}

void Renderer::renderGlass(Widget& widget, const GlassStyle& glass) {
    const std::optional<CornerRadii> radii = widget.shapeRadii();
    const int width = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int height = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    const float opacity = widget.effectiveOpacity();
    if (!radii || width <= 0 || height <= 0 || opacity <= 0.0f)
        return;

    const float frost = std::max(0.0f, glass.frost);
    const float sigma = frost * 0.5f;
    const bool frosted = sigma > 0.01f;
    const int taps = frosted ? static_cast<int>(std::ceil(sigma * 3.0f)) : 0;
    const int pad = taps + 1;
    const int texW = width + 2 * pad;
    const int texH = height + 2 * pad;

    const GLuint local = captureBackdrop(widget, texW, texH, pad);
    if (local == 0)
        return;

    GLuint source = local;
    GLuint blurred = 0;
    if (frosted) {
        blurred = blurLayerTexture(local, texW, texH, width, height, pad, taps, BlurStyle{frost, frost, 0.0f});
        source = blurred;
    }

    const float angle = glm::radians(glass.lightAngle);
    const glm::vec2 light{std::sin(angle), -std::cos(angle)};
    const float refraction = std::clamp(glass.refraction, 0.0f, 1.0f);

    glassCompositeShader.use();
    glassCompositeShader.setMat4("world", widget.worldMatrix());
    glassCompositeShader.setVec2("uRectSize", glm::vec2{static_cast<float>(texW), static_cast<float>(texH)});
    glassCompositeShader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
    glassCompositeShader.setVec2("uOffset", glm::vec2{0.0f, 0.0f});
    glassCompositeShader.setVec2("uHalfSize", glm::vec2{widget.displayedWidth(), widget.displayedHeight()} * 0.5f);
    glassCompositeShader.setVec4("uRadius", glm::vec4{radii->topLeft, radii->topRight, radii->bottomRight, radii->bottomLeft});
    glassCompositeShader.setFloat("uDepth", std::max(glass.depth, 1.0f));
    glassCompositeShader.setFloat("uIndex", 1.0f + 0.8f * refraction);
    glassCompositeShader.setFloat("uDispersion", std::clamp(glass.dispersion, 0.0f, 1.0f));
    glassCompositeShader.setVec2("uLight", light);
    glassCompositeShader.setFloat("uLightIntensity", std::clamp(glass.lightIntensity, 0.0f, 1.0f));
    glassCompositeShader.setFloat("uSplay", std::clamp(glass.splay, 0.0f, 1.0f));
    glassCompositeShader.setVec4("uTint", glm::vec4{glass.tint.r, glass.tint.g, glass.tint.b, glass.tint.a});
    glassCompositeShader.setFloat("uSaturation", glass.saturation);
    glassCompositeShader.setFloat("uBrightness", glass.brightness);
    glassCompositeShader.setFloat("uOpacity", opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    glassCompositeShader.setInt("uBlurred", 0);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(widget.displayedWidth(), widget.displayedHeight());

    glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                         static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));

    if (blurred != 0)
        releaseTexture(blurred);
    releaseTexture(local);
}

void Renderer::renderLayerBlur(Widget& widget, const BlurStyle& blur) {
    const int width = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int height = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    if (width <= 0 || height <= 0)
        return;

    const float startSigma = std::max(0.0f, blur.startRadius) * 0.5f;
    const float endSigma = std::max(0.0f, blur.endRadius) * 0.5f;
    const int taps = static_cast<int>(std::ceil(std::max(startSigma, endSigma) * 3.0f));
    const int pad = taps + 1;
    const int texW = width + 2 * pad;
    const int texH = height + 2 * pad;

    const GLuint layer = acquireTexture(texW, texH);
    captureWidget(widget, layer, texW, texH, pad, false);

    const GLuint result = blurLayerTexture(layer, texW, texH, width, height, pad, taps, blur);

    layerBlurCompositeShader.use();
    layerBlurCompositeShader.setMat4("world", widget.worldMatrix());
    layerBlurCompositeShader.setVec2("uRectSize", glm::vec2{static_cast<float>(texW), static_cast<float>(texH)});
    layerBlurCompositeShader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
    layerBlurCompositeShader.setVec2("uOffset", glm::vec2{0.0f, 0.0f});

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, result);
    layerBlurCompositeShader.setInt("uLayer", 0);

    GLint prevSrcRGB, prevDstRGB, prevSrcAlpha, prevDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstAlpha);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    const float origin = -static_cast<float>(pad);
    drawQuad(origin, origin, origin + static_cast<float>(texW), origin + static_cast<float>(texH));

    glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB), static_cast<GLenum>(prevDstRGB),
                         static_cast<GLenum>(prevSrcAlpha), static_cast<GLenum>(prevDstAlpha));

    releaseTexture(result);
    releaseTexture(layer);
}

void Renderer::renderAlphaShadow(Widget& widget, const ShadowBase& shadow, bool inner) {
    const int width = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int height = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    const Color color = shadow.color();
    const float opacity = widget.effectiveOpacity();
    if (width <= 0 || height <= 0 || color.a <= 0.0f || opacity <= 0.0f)
        return;

    const float sigma = std::max(0.0f, shadow.radius()) * 0.5f;
    const bool blurred = sigma > 0.01f;
    const int taps = blurred ? static_cast<int>(std::ceil(sigma * 3.0f)) : 0;
    const int spread = static_cast<int>(std::lround(shadow.spread()));
    const int pad = taps + std::abs(spread) + 1;
    const int texW = width + 2 * pad;
    const int texH = height + 2 * pad;

    const GLuint own = acquireTexture(texW, texH);
    captureWidget(widget, own, texW, texH, pad, true);

    GLuint shape = own;
    GLuint grown = 0;
    GLuint softened = 0;
    if (spread != 0) {
        const bool erode = inner ? spread > 0 : spread < 0;
        grown = morphAlpha(shape, texW, texH, std::abs(spread), erode);
        shape = grown;
    }
    if (blurred) {
        softened = blurAlpha(shape, texW, texH, sigma, taps);
        shape = softened;
    }

    Shader& shader = inner ? alphaInnerShadowShader : alphaDropShadowShader;
    shader.use();
    shader.setMat4("world", widget.worldMatrix());
    shader.setVec2("uRectSize", glm::vec2{static_cast<float>(texW), static_cast<float>(texH)});
    shader.setVec2("uPad", glm::vec2{static_cast<float>(pad), static_cast<float>(pad)});
    shader.setVec2("uOffset", glm::vec2{shadow.offsetX(), shadow.offsetY()});
    shader.setVec4("uColor", glm::vec4{color.r, color.g, color.b, color.a});
    shader.setFloat("uOpacity", opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shape);
    shader.setInt("uBlurred", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, own);
    shader.setInt("uOwn", 1);
    glActiveTexture(GL_TEXTURE0);

    if (inner) {
        drawQuad(widget.displayedWidth(), widget.displayedHeight());
    } else {
        const float x0 = shadow.offsetX() - static_cast<float>(pad);
        const float y0 = shadow.offsetY() - static_cast<float>(pad);
        drawQuad(x0, y0, x0 + static_cast<float>(texW), y0 + static_cast<float>(texH));
    }

    if (softened != 0)
        releaseTexture(softened);
    if (grown != 0)
        releaseTexture(grown);
    releaseTexture(own);
}

void Renderer::shutdown() {
    for (const PooledTexture& entry : m_texturePool)
        glDeleteTextures(1, &entry.id);
    m_texturePool.clear();

    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &textVBO);
    glDeleteBuffers(1, &cameraUBO);

    quadShader.release();
    imageShader.release();
    svgShader.release();
    textShader.release();
    clipCompositeShader.release();
    shadowShader.release();
    innerShadowShader.release();
    effectBlurShader.release();
    effectMorphShader.release();
    alphaDropShadowShader.release();
    alphaInnerShadowShader.release();
    layerBlurShader.release();
    layerBlurCompositeShader.release();
    backdropResampleShader.release();
    backgroundBlurCompositeShader.release();
    glassCompositeShader.release();

    quadVAO = quadVBO = 0;
    textVAO = textVBO = 0;
    cameraUBO = 0;
    clipFBO = 0;

    m_isInit = false;
}

}