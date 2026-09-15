#include <tavoos/gfx/renderer.h>
#include <tavoos/widget/image.h>

#include <spdlog/spdlog.h>
#include <stb_image/stb_image.h>

namespace Tavoos {

ImageWidget::ImageWidget(Object* parent) : Widget{parent} {
    m_source.onChange([this](const auto&) { reload(); });
    bindRepaintTriggers(m_fit);
}

ImageWidget::~ImageWidget() {
    releaseCurrent();
}

void ImageWidget::releaseCurrent() {
    if (!m_loadedPath.empty()) {
        TextureCache::release(m_loadedPath);
        m_loadedPath.clear();
        m_entry = nullptr;
    }
}

void ImageWidget::reload() {
    releaseCurrent();

    const std::string path = m_source.get();
    if (path.empty()) {
        spdlog::warn("failed to load image: the image path is empty");
        m_naturalWidth = m_naturalHeight = 0;
        requestRelayout();
        return;
    }

    m_entry = TextureCache::acquire(path);
    if (m_entry) {
        m_loadedPath = path;
        m_naturalWidth = m_entry->width;
        m_naturalHeight = m_entry->height;
    } else {
        m_naturalWidth = m_naturalHeight = 0;
    }

    requestRelayout();
}

Widget::Size ImageWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };
    if (width() > 0)
        return { static_cast<float>(width()), static_cast<float>(m_naturalHeight) };
    if (height() > 0)
        return { static_cast<float>(m_naturalWidth), static_cast<float>(height()) };
    return { static_cast<float>(m_naturalWidth), static_cast<float>(m_naturalHeight) };
}

void ImageWidget::render(Renderer& r) {
    r.renderImage(*this);
    renderChildren(r);
}

}