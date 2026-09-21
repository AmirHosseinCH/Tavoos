#pragma once

#include <tavoos/export.hpp>
#include <tavoos/text/font.h>
#include <tavoos/text/fontenums.h>
#include <tavoos/text/textlayout.h>
#include <tavoos/types.h>
#include <tavoos/widget/widget.h>

#include <string>
#include <vector>

namespace Tavoos {

class FontFace;

class TAVOOS_EXPORT TextWidget : public Widget {
    friend class Renderer;

public:
    TextWidget(Object* parent);

    decltype(auto) text(this auto&& self, PropertyArg<std::string> content) {
        content.applyTo(self.m_text);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) family(this auto&& self, PropertyArg<std::string> name) {
        name.applyTo(self.m_family);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) weight(this auto&& self, PropertyArg<FontWeight> w) {
        w.applyTo(self.m_weight);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, PropertyArg<FontStyle> s) {
        s.applyTo(self.m_style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fontSize(this auto&& self, PropertyArg<float> size) {
        size.applyTo(self.m_fontSize);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, const Font& font) {
        self.m_font.set(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) font(this auto&& self, State<Font>& font) {
        self.m_font.bind(font);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) color(this auto&& self, PropertyArg<Paint> c) {
        c.applyTo(self.m_color);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) textAlignment(this auto&& self, PropertyArg<Alignment> align) {
        align.applyTo(self.m_textAlignment);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) wrapMode(this auto&& self, PropertyArg<WrapMode> mode) {
        mode.applyTo(self.m_wrapMode);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) elideMode(this auto&& self, PropertyArg<ElideMode> mode) {
        mode.applyTo(self.m_elideMode);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) maxLines(this auto&& self, PropertyArg<int> lines) {
        lines.applyTo(self.m_maxLines);
        return std::forward<decltype(self)>(self);
    }

    std::string text() const { return m_text; }
    Font font() const { return {m_family, m_fontSize, m_weight, m_style}; }
    std::string family() const { return m_family; }
    FontWeight weight() const { return m_weight; }
    FontStyle style() const { return m_style; }
    float fontSize() const { return m_fontSize; }
    Paint color() const { return m_color; }
    Alignment textAlignment() const { return m_textAlignment; }
    WrapMode wrapMode() const { return m_wrapMode; }
    ElideMode elideMode() const { return m_elideMode; }
    int maxLines() const { return m_maxLines; }

protected:
    void render(Renderer& r) override;
    Widget::Size computeIntrinsicSize() override;

private:
    FontFace* resolvedFace();

    struct LineCache {
        bool valid = false;
        std::string text;
        FontFace* face = nullptr;
        float scale = 0.0f;
        WrapMode wrapMode = WrapMode::NoWrap;
        float layoutWrapWidth = 0.0f;
        float clampMaxWidth = 0.0f;
        ElideMode elideMode = ElideMode::None;
        int maxLines = 0;
        float heightForMaxLines = 0.0f;
        std::vector<TextLine> lines;
    };

    const std::vector<TextLine>& layoutLines(LineCache& cache, FontFace& face, float scale,
                                              float layoutWrapWidth, float clampMaxWidth,
                                              float heightForMaxLines);

    LineCache m_intrinsicLineCache;
    LineCache m_renderLineCache;

    Property<std::string> m_text;
    Property<std::string> m_family{};
    Property<FontWeight> m_weight{FontWeight::Regular};
    Property<FontStyle> m_style{FontStyle::Normal};
    Property<float> m_fontSize{14.0f};
    Property<Font> m_font;
    Property<Paint> m_color{Color::Black};
    Property<Alignment> m_textAlignment{Alignment::Left | Alignment::Top};
    Property<WrapMode> m_wrapMode{WrapMode::NoWrap};
    Property<ElideMode> m_elideMode{ElideMode::None};
    Property<int> m_maxLines{0};
};

}