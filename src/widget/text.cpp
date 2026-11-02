#include <tavoos/gfx/renderer.h>
#include <tavoos/text/text.h>
#include <tavoos/widget/text.h>

namespace Tavoos {

TextWidget::TextWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_text, m_family, m_weight, m_style, m_fontSize, m_wrapMode, m_elideMode, m_maxLines,
                         m_tabStopDistance);
    bindRepaintTriggers(m_color, m_textAlignment, m_selectionStart, m_selectionEnd, m_selectionColor, m_selectedTextColor);
    m_font.onChange([this](const Font& font) {
        family(font.family).weight(font.weight).style(font.style).fontSize(font.size);
    });
}

FontFace* TextWidget::resolvedFace() {
    FontFamily* fam = FontManager::instance().family(m_family.get());
    if (!fam)
        return nullptr;
    return fam->resolveFace(m_weight.get(), m_style.get());
}

const std::vector<TextLine>& TextWidget::layoutLines(LineCache& cache, FontFace& face, float scale,
                                                      float layoutWrapWidth, float clampMaxWidth,
                                                      float heightForMaxLines) {
    const std::string& text = m_text.get();
    WrapMode wrap = m_wrapMode.get();
    ElideMode elide = m_elideMode.get();
    int maxLines = m_maxLines.get();
    const float tabStop = m_tabStopDistance.get();

    if (cache.valid && cache.text == text && cache.face == &face && cache.scale == scale &&
        cache.tabStopDistance == tabStop && cache.wrapMode == wrap && cache.layoutWrapWidth == layoutWrapWidth &&
        cache.clampMaxWidth == clampMaxWidth && cache.elideMode == elide &&
        cache.maxLines == maxLines && cache.heightForMaxLines == heightForMaxLines) {
        return cache.lines;
    }

    float lineHeight = face.lineHeight() * scale;
    int limit = effectiveMaxLines(maxLines, heightForMaxLines, lineHeight);
    auto lines = clampLines(brokenLines(face, scale, wrap, layoutWrapWidth), limit, text,
                            GlyphAdvance{face, scale, tabStop}, elide, clampMaxWidth);

    cache.valid = true;
    cache.text = text;
    cache.face = &face;
    cache.scale = scale;
    cache.tabStopDistance = tabStop;
    cache.wrapMode = wrap;
    cache.layoutWrapWidth = layoutWrapWidth;
    cache.clampMaxWidth = clampMaxWidth;
    cache.elideMode = elide;
    cache.maxLines = maxLines;
    cache.heightForMaxLines = heightForMaxLines;
    cache.lines = std::move(lines);
    return cache.lines;
}

Widget::Size TextWidget::computeIntrinsicSize() {
    FontFace* face = resolvedFace();
    if (!face)
        return { 0.0f, 0.0f };

    float scale = m_fontSize.get() / FontFace::ReferencePixelSize;
    float wrapWidth = (width() > 0) ? static_cast<float>(width()) : 0.0f;
    float lineHeight = face->lineHeight() * scale;

    const auto& lines = layoutLines(m_intrinsicLineCache, *face, scale, wrapWidth, wrapWidth,
                                     (height() > 0) ? static_cast<float>(height()) : 0.0f);

    float measuredWidth = 0.0f;
    for (auto& line : lines)
        measuredWidth = std::max(measuredWidth, line.width);

    float measuredHeight = lineHeight * static_cast<float>(lines.size());

    return {
        (width()  > 0) ? static_cast<float>(width())  : measuredWidth,
        (height() > 0) ? static_cast<float>(height()) : measuredHeight
    };
}

const std::vector<TextLine>& TextWidget::brokenLines(FontFace& face, float scale, WrapMode wrap, float wrapWidth) {
    const std::string& text = m_text.get();
    const float tabStop = m_tabStopDistance.get();
    if (wrap == WrapMode::NoWrap)
        wrapWidth = 0.0f;
    if (m_breakCache.valid && m_breakCache.text == text && m_breakCache.face == &face && m_breakCache.scale == scale &&
        m_breakCache.tabStopDistance == tabStop && m_breakCache.wrapMode == wrap && m_breakCache.wrapWidth == wrapWidth)
        return m_breakCache.lines;

    m_breakCache.valid = true;
    m_breakCache.text = text;
    m_breakCache.face = &face;
    m_breakCache.scale = scale;
    m_breakCache.tabStopDistance = tabStop;
    m_breakCache.wrapMode = wrap;
    m_breakCache.wrapWidth = wrapWidth;
    m_breakCache.lines = layoutTextLines(text, GlyphAdvance{face, scale, tabStop}, wrap, wrapWidth);
    return m_breakCache.lines;
}

float TextWidget::lineHeight() {
    FontFace* face = resolvedFace();
    if (!face)
        return 0.0f;
    return face->lineHeight() * m_fontSize.get() / FontFace::ReferencePixelSize;
}

const std::vector<TextLine>& TextWidget::renderLines(FontFace& face, float scale) {
    const float boxWidth = resolvedWidth() - paddingLeft() - paddingRight();
    const float wrapWidth = m_wrapMode.get() != WrapMode::NoWrap ? boxWidth : 0.0f;
    return layoutLines(m_renderLineCache, face, scale, wrapWidth, boxWidth, resolvedHeight());
}

float TextWidget::lineStartX(const TextLine& line) {
    const float boxWidth = resolvedWidth() - paddingLeft() - paddingRight();
    const Alignment align = m_textAlignment.get();
    if (hasFlag(align, Alignment::Right))
        return paddingLeft() + boxWidth - line.width;
    if (hasFlag(align, Alignment::CenterHorizontal))
        return paddingLeft() + (boxWidth - line.width) * 0.5f;
    return paddingLeft();
}

float TextWidget::blockStartY(std::size_t lineCount, float lineHeight) {
    const float boxHeight = resolvedHeight() - paddingTop() - paddingBottom();
    const float blockHeight = lineHeight * static_cast<float>(lineCount);
    const Alignment align = m_textAlignment.get();
    if (hasFlag(align, Alignment::Bottom))
        return paddingTop() + boxHeight - blockHeight;
    if (hasFlag(align, Alignment::CenterVertical))
        return paddingTop() + (boxHeight - blockHeight) * 0.5f;
    return paddingTop();
}

Rect TextWidget::positionToRectangle(std::size_t position) {
    FontFace* face = resolvedFace();
    if (!face)
        return {};

    const float scale = m_fontSize.get() / FontFace::ReferencePixelSize;
    const float lineHeight = face->lineHeight() * scale;
    const auto& lines = renderLines(*face, scale);
    return positionToRect(m_text.get(), lines, position, blockStartY(lines.size(), lineHeight), lineHeight,
                          [&](std::size_t li) { return lineStartX(lines[li]); },
                          GlyphAdvance{*face, scale, m_tabStopDistance.get()});
}

std::size_t TextWidget::positionAt(float x, float y) {
    FontFace* face = resolvedFace();
    if (!face)
        return 0;

    const float scale = m_fontSize.get() / FontFace::ReferencePixelSize;
    const float lineHeight = face->lineHeight() * scale;
    const auto& lines = renderLines(*face, scale);
    return Tavoos::positionAt(m_text.get(), lines, x, y, blockStartY(lines.size(), lineHeight), lineHeight,
                              [&](std::size_t li) { return lineStartX(lines[li]); },
                              GlyphAdvance{*face, scale, m_tabStopDistance.get()});
}

void TextWidget::render(Renderer& r) {
    r.renderText(*this);
    renderInnerEffects(r);
    renderChildren(r);
}

}