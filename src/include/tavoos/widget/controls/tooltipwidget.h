#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/font.h>
#include <tavoos/types.h>
#include <tavoos/widget/controls/style/tooltipstyle.h>
#include <tavoos/widget/templates/tooltipbase.h>

namespace Tavoos {

class TAVOOS_EXPORT ToolTipWidget : public ToolTipBase {
public:
    ToolTipWidget(Object* parent);

    decltype(auto) backgroundColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_backgroundColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_borderColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) borderWidth(this auto&& self, PropertyArg<float> width) {
        self.m_borderWidth.set(width);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) radius(this auto&& self, PropertyArg<int> radius) {
        self.m_radius.set(radius);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) textColor(this auto&& self, PropertyArg<Paint> color) {
        self.m_textColor.set(color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, const Font& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, State<Font>& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, const ToolTipStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<ToolTipStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    Paint backgroundColor() const { return m_backgroundColor; }
    Paint borderColor() const { return m_borderColor; }
    float borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }
    Paint textColor() const { return m_textColor; }
    Font font() const { return m_font; }
    ToolTipStyle style() const {
        return { m_backgroundColor.get(), m_borderColor.get(), m_borderWidth.get(), paddingLeft(), m_radius.get(),
                 m_textColor.get(), m_font.get(), enter(), exit() };
    }

private:
    void applyStyle(const ToolTipStyle& style);

    BindableState<Paint> m_backgroundColor{Color::rgba(40, 42, 50)};
    BindableState<Paint> m_borderColor{Color::Transparent};
    BindableState<float> m_borderWidth{0.0f};
    BindableState<int> m_radius{6};
    BindableState<Paint> m_textColor{Color::White};
    BindableState<Font> m_font;
    BindableState<ToolTipStyle> m_style;
};

}
