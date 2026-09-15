#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

namespace Tavoos {

class TAVOOS_EXPORT RowWidget : public Widget {
public:
    RowWidget(Object*);

    bool isLayouter() const override { return true; }

    decltype(auto) spacing(this auto&& self, PropertyArg<float> spacing) {
        spacing.applyTo(self.m_spacing);
        return std::forward<decltype(self)>(self);
    }

    float spacing() const { return m_spacing; }

protected:
    void render(Renderer& r) override {
        renderChildren(r);
    }
    Size computeIntrinsicSize() override;

private:
    void layout(bool = false) override;

    Property<float> m_spacing{0.0f};
};


}