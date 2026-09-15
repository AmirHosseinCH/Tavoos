#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/widget.h>

#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT GridWidget : public Widget {
public:
    GridWidget(Object*);

    bool isLayouter() const override { return true; }

    decltype(auto) columns(this auto&& self, PropertyArg<int> count) {
        count.applyTo(self.m_columns);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) columnSpacing(this auto&& self, PropertyArg<float> spacing) {
        spacing.applyTo(self.m_columnSpacing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) rowSpacing(this auto&& self, PropertyArg<float> spacing) {
        spacing.applyTo(self.m_rowSpacing);
        return std::forward<decltype(self)>(self);
    }

    int columns() const { return m_columns; }
    float columnSpacing() const { return m_columnSpacing; }
    float rowSpacing() const { return m_rowSpacing; }

protected:
    void render(Renderer& r) override {
        renderChildren(r);
    }
    Size computeIntrinsicSize() override;

private:
    void layout(bool = false) override;

    struct Placement {
        Widget* widget;
        int row, col, rowSpan, colSpan;
    };

    struct GridMetrics {
        std::vector<Placement> placements;
        std::vector<float> columnWidths;
        std::vector<float> rowHeights;
    };

    GridMetrics computeMetrics() const;

    Property<int> m_columns{1};
    Property<float> m_columnSpacing{0.0f};
    Property<float> m_rowSpacing{0.0f};
};

}
