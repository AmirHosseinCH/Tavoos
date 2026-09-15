#pragma once

#include <tavoos/export.hpp>
#include <tavoos/gfx/texturecache.h>
#include <tavoos/types.h>
#include <tavoos/widget/widget.h>

#include <string>

namespace Tavoos {

enum class ImageFillMode {
    Stretch,
    PreserveAspectFit,
    PreserveAspectCrop,
    Tile,
    TileVertically,
    TileHorizontally,
};

class TAVOOS_EXPORT ImageWidget : public Widget {
    friend class Renderer;

public:
    ImageWidget(Object* parent);
    ~ImageWidget() override;

    decltype(auto) source(this auto&& self, PropertyArg<std::string> path) {
        path.applyTo(self.m_source);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fillMode(this auto&& self, PropertyArg<ImageFillMode> fit) {
        fit.applyTo(self.m_fit);
        return std::forward<decltype(self)>(self);
    }

    std::string source() const { return m_source; }
    ImageFillMode fillMode() const { return m_fit; }
    int naturalWidth() const { return m_naturalWidth; }
    int naturalHeight() const { return m_naturalHeight; }

protected:
    void render(Renderer& r) override;
    Widget::Size computeIntrinsicSize() override;

private:
    void reload();
    void releaseCurrent();
    unsigned int texture() const { return m_entry ? m_entry->texture : 0; }

    Property<std::string> m_source;
    Property<ImageFillMode> m_fit{ImageFillMode::Stretch};

    std::string m_loadedPath;
    TextureCache::Entry* m_entry{nullptr};

    int m_naturalWidth{0};
    int m_naturalHeight{0};
};

}