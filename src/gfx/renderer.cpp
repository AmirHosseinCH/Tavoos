#include <tavoos/gfx/renderer.h>
#include <tavoos/gfx/shader_sources.h>
#include <tavoos/text/text.h>
#include <tavoos/widget/image.h>
#include <tavoos/widget/rectangle.h>
#include <tavoos/widget/svg.h>
#include <tavoos/widget/text.h>
#include <tavoos/widget/widget.h>
#include <tavoos/window.h>

#include <algorithm>
#include <array>
#include <cmath>
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

    std::string quadFragmentSource = std::string(fragmentShaderPrelude) + quadFragmentShaderHead + paintSamplingGLSL + quadFragmentShaderTail;
    std::string svgFragmentSource  = std::string(fragmentShaderPrelude) + svgFragmentShaderHead  + paintSamplingGLSL + svgFragmentShaderTail;
    std::string textFragmentSource = std::string(fragmentShaderPrelude) + textFragmentShaderHead + paintSamplingGLSL + textFragmentShaderTail;

    quadShader.compile(quadVertexShader, quadFragmentSource);
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

    for (const auto& child : window.children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get())) {
            if (widget->visible())
                renderWidget(*widget);
        }
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
    const float vertices[] = { 0, 0, w, 0, w, h, 0, 0, w, h, 0, h };
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
                                          GLuint texture, int texW, int texH, const glm::mat4& worldMatrix) {
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

    projectionRef = glm::ortho(0.0f, static_cast<float>(texW),
                                0.0f, static_cast<float>(texH),
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

void Renderer::shutdown() {
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &textVBO);
    glDeleteBuffers(1, &cameraUBO);

    quadShader.release();
    imageShader.release();
    svgShader.release();
    textShader.release();
    clipCompositeShader.release();

    quadVAO = quadVBO = 0;
    textVAO = textVBO = 0;
    cameraUBO = 0;
    clipFBO = 0;

    m_isInit = false;
}

}