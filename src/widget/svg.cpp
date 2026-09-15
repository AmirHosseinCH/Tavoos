#include <tavoos/gfx/renderer.h>
#include <tavoos/gfx/svgdocumentcache.h>
#include <tavoos/widget/svg.h>

#include <lunasvg.h>

namespace Tavoos {

SVGWidget::SVGWidget(Object* parent) : Widget{parent} {
    m_source.onChange([this](const auto&) { requestRelayout(); });
    bindRepaintTriggers(m_color);
}

SVGWidget::~SVGWidget() {
    releaseCurrent();
}

void SVGWidget::releaseCurrent() {
    if (m_entry) {
        SVGTextureCache::release(m_loadedPath, m_loadedWidth, m_loadedHeight);
        m_entry = nullptr;
    }
}

void SVGWidget::ensureRasterized() {
    const std::string path = m_source.get();
    if (path.empty())
        return;

    const int w = static_cast<int>(resolvedWidth());
    const int h = static_cast<int>(resolvedHeight());
    if (w <= 0 || h <= 0)
        return;

    if (path == m_loadedPath && w == m_loadedWidth && h == m_loadedHeight)
        return;

    SVGTextureCache::Entry* const entry = SVGTextureCache::acquire(path, w, h);
    if (!entry)
        return;

    releaseCurrent();
    m_entry = entry;
    m_loadedPath = path;
    m_loadedWidth = w;
    m_loadedHeight = h;
}

void SVGWidget::ensureNaturalSizeKnown() {
    const std::string path = m_source.get();
    if (path.empty())
        return;

    if (path == m_loadedPath && m_naturalWidth > 0)
        return;

    lunasvg::Document* const document = SVGDocumentCache::get(path);
    if (!document)
        return;

    m_naturalWidth = static_cast<int>(document->width());
    m_naturalHeight = static_cast<int>(document->height());
}

Widget::Size SVGWidget::computeIntrinsicSize() {
    ensureNaturalSizeKnown();

    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };
    if (width() > 0)
        return { static_cast<float>(width()), static_cast<float>(m_naturalHeight) };
    if (height() > 0)
        return { static_cast<float>(m_naturalWidth), static_cast<float>(height()) };
    return { static_cast<float>(m_naturalWidth), static_cast<float>(m_naturalHeight) };
}

void SVGWidget::render(Renderer& r) {
    ensureRasterized();
    r.renderSVG(*this);
    renderChildren(r);
}

}