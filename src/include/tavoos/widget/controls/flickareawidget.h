#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/flickareastyle.h>
#include <tavoos/widget/templates/flickareabase.h>

namespace Tavoos {

class TAVOOS_EXPORT FlickAreaWidget : public FlickAreaBase {
public:
    FlickAreaWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_backgroundColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const FlickAreaStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<FlickAreaStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    int radius() const { return m_radius; }
    FlickAreaStyle style() const { return { m_backgroundColor.get(), m_radius.get() }; }

private:
    void applyStyle(const FlickAreaStyle& style);

    BindableState<Paint> m_backgroundColor{Color::Transparent};
    BindableState<int> m_radius{0};
    BindableState<FlickAreaStyle> m_style;
};

}
