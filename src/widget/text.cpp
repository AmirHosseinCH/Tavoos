#include <tavoos/gfx/renderer.h>
#include <tavoos/text/text.h>
#include <tavoos/widget/text.h>

namespace Tavoos {

TextWidget::TextWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_text, m_family, m_weight, m_style, m_fontSize, m_wrapMode, m_elideMode, m_maxLines);
    bindRepaintTriggers(m_color, m_textAlignment);
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

    if (cache.valid && cache.text == text && cache.face == &face && cache.scale == scale &&
        cache.wrapMode == wrap && cache.layoutWrapWidth == layoutWrapWidth &&
        cache.clampMaxWidth == clampMaxWidth && cache.elideMode == elide &&
        cache.maxLines == maxLines && cache.heightForMaxLines == heightForMaxLines) {
        return cache.lines;
    }

    float lineHeight = face.lineHeight() * scale;
    auto lines = layoutTextLines(text, face, scale, wrap, layoutWrapWidth);
    int limit = effectiveMaxLines(maxLines, heightForMaxLines, lineHeight);
    lines = clampLines(std::move(lines), limit, text, face, scale, elide, clampMaxWidth);

    cache.valid = true;
    cache.text = text;
    cache.face = &face;
    cache.scale = scale;
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

void TextWidget::render(Renderer& r) {
    r.renderText(*this);
    renderChildren(r);
}

}