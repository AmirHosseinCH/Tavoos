#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT FlexWidget : public Widget {
public:
    FlexWidget(Object*);

    bool isLayouter() const override { return true; }

    decltype(auto) direction(this auto&& self, PropertyArg<FlexDirection> value) {
        value.applyTo(self.m_direction);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) wrap(this auto&& self, PropertyArg<FlexWrap> value) {
        value.applyTo(self.m_wrap);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) justifyContent(this auto&& self, PropertyArg<FlexJustify> value) {
        value.applyTo(self.m_justifyContent);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) alignItems(this auto&& self, PropertyArg<Alignment> value) {
        value.applyTo(self.m_alignItems);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) alignContent(this auto&& self, PropertyArg<FlexAlign> value) {
        value.applyTo(self.m_alignContent);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) rowGap(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_rowGap);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) columnGap(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_columnGap);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) gap(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_rowGap);
        value.applyTo(self.m_columnGap);
        return std::forward<decltype(self)>(self);
    }

    FlexDirection direction() const { return m_direction; }
    FlexWrap wrap() const { return m_wrap; }
    FlexJustify justifyContent() const { return m_justifyContent; }
    Alignment alignItems() const { return m_alignItems; }
    FlexAlign alignContent() const { return m_alignContent; }
    float rowGap() const { return m_rowGap; }
    float columnGap() const { return m_columnGap; }

protected:
    void render(Renderer& r) override {
        renderChildren(r);
    }
    Size computeIntrinsicSize() override;

private:
    void layout(bool = false) override;

    struct FlexLine {
        std::vector<Widget*> items;
        std::vector<Size> itemSizes;
        float mainSize{0.0f};
        float crossSize{0.0f};
    };

    struct FlexMetrics {
        std::vector<FlexLine> lines;
        float naturalMainSize{0.0f};
        float naturalCrossSize{0.0f};
    };

    static constexpr float kUnbounded = 1e9f;

    FlexMetrics computeMetrics(float availableMainSpace) const;

    Property<FlexDirection> m_direction{FlexDirection::Row};
    Property<FlexWrap> m_wrap{FlexWrap::NoWrap};
    Property<FlexJustify> m_justifyContent{FlexJustify::Start};
    Property<Alignment> m_alignItems{Alignment::None};
    Property<FlexAlign> m_alignContent{FlexAlign::Stretch};
    Property<float> m_rowGap{0.0f};
    Property<float> m_columnGap{0.0f};
};

}
