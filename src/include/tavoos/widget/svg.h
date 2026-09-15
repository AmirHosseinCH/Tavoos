#pragma once

#include <tavoos/export.hpp>
#include <tavoos/gfx/svgtexturecache.h>
#include <tavoos/types.h>
#include <tavoos/widget/widget.h>

#include <string>

namespace Tavoos {

class TAVOOS_EXPORT SVGWidget : public Widget {
    friend class Renderer;

public:
    SVGWidget(Object* parent);
    ~SVGWidget() override;

    decltype(auto) source(this auto&& self, PropertyArg<std::string> path) {
        path.applyTo(self.m_source);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) color(this auto&& self, PropertyArg<Paint> color) {
        color.applyTo(self.m_color);
        self.m_hasColor = true;
        return std::forward<decltype(self)>(self);
    }

    std::string source() const { return m_source; }
    Paint color() const { return m_color; }
    bool hasColor() const { return m_hasColor; }
    int naturalWidth() const { return m_naturalWidth; }
    int naturalHeight() const { return m_naturalHeight; }

protected:
    void render(Renderer& r) override;
    Widget::Size computeIntrinsicSize() override;

private:
    void ensureRasterized();
    void ensureNaturalSizeKnown();
    void releaseCurrent();
    unsigned int texture() const { return m_entry ? m_entry->texture : 0; }

    Property<std::string> m_source;
    Property<Paint> m_color{Color::Black};
    bool m_hasColor{false};

    std::string m_loadedPath;
    int m_loadedWidth{0};
    int m_loadedHeight{0};
    SVGTextureCache::Entry* m_entry{nullptr};

    int m_naturalWidth{0};
    int m_naturalHeight{0};
};

}