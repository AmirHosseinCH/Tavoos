#include <tavoos/widget/column.h>
#include <tavoos/window.h>

namespace Tavoos {

ColumnWidget::ColumnWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_spacing);
}

void ColumnWidget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    std::vector<Size> itemSizes;
    itemSizes.reserve(items.size());
    for (auto* item : items)
        itemSizes.push_back(item->intrinsicSize());

    float fixedHeight = 0.0f;
    int fillCount = 0;
    float maxChildWidth = 0.0f;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const float vMargin = items[i]->marginTop() + items[i]->marginBottom();
        const float hMargin = items[i]->marginLeft() + items[i]->marginRight();
        if (hasFlag(items[i]->fill(), Fill::Height)) {
            ++fillCount;
            fixedHeight += vMargin;
        } else {
            fixedHeight += itemSizes[i].height + vMargin;
        }
        if (!hasFlag(items[i]->fill(), Fill::Width))
            maxChildWidth = std::max(maxChildWidth, itemSizes[i].width + hMargin);
    }
    if (items.size() > 1)
        fixedHeight += m_spacing * static_cast<float>(items.size() - 1);

    float resolvedW, resolvedH, resX, resY;

    if (parentIsLayouter()) {
        resolvedW = resolvedWidth();
        resolvedH = resolvedHeight();
        resX = resolvedX();
        resY = resolvedY();
    } else {
        const ContentArea area = resolveContentArea();

        resolvedH = hasFlag(fill(), Fill::Height) ? area.height - marginTop() - marginBottom()
                    : (height() > 0) ? static_cast<float>(height()) : fixedHeight + paddingTop() + paddingBottom();
        resolvedW = hasFlag(fill(), Fill::Width)  ? area.width - marginLeft() - marginRight()
                    : (width()  > 0) ? static_cast<float>(width())  : maxChildWidth + paddingLeft() + paddingRight();

        const Alignment align = alignment();
        if (hasFlag(align, Alignment::Left))                  resX = area.x + marginLeft();
        else if (hasFlag(align, Alignment::Right))             resX = area.x + area.width - resolvedW - marginRight();
        else if (hasFlag(align, Alignment::CenterHorizontal))  resX = area.x + (area.width - resolvedW) * 0.5f;
        else                                                    resX = area.x + static_cast<float>(x());

        if (hasFlag(align, Alignment::Top))                   resY = area.y + marginTop();
        else if (hasFlag(align, Alignment::Bottom))            resY = area.y + area.height - resolvedH - marginBottom();
        else if (hasFlag(align, Alignment::CenterVertical))    resY = area.y + (area.height - resolvedH) * 0.5f;
        else                                                    resY = area.y + static_cast<float>(y());
    }

    setResolved(resX, resY, resolvedW, resolvedH);
    syncDisplayedGeometry();
    updateWorldMatrix();

    const float innerW = resolvedW - paddingLeft() - paddingRight();
    const float innerH = resolvedH - paddingTop()  - paddingBottom();
    const float leftover = std::max(0.0f, innerH - fixedHeight);
    const float fillShare = (fillCount > 0) ? leftover / static_cast<float>(fillCount) : 0.0f;

    float cursorY = paddingTop();
    for (std::size_t i = 0; i < items.size(); ++i) {
        Widget* const item = items[i];
        const bool fillsWidth  = hasFlag(item->fill(), Fill::Width);
        const bool fillsHeight = hasFlag(item->fill(), Fill::Height);
        const float itemMarginTop = item->marginTop();
        const float itemMarginBottom = item->marginBottom();
        const float itemMarginLeft = item->marginLeft();
        const float itemMarginRight = item->marginRight();

        const float childH = fillsHeight ? fillShare : itemSizes[i].height;
        const float childW = fillsWidth  ? (innerW - itemMarginLeft - itemMarginRight) : itemSizes[i].width;

        const Alignment childAlign = item->alignment();
        float childX;
        if (fillsWidth)
            childX = paddingLeft() + itemMarginLeft;
        else if (hasFlag(childAlign, Alignment::Right))
            childX = paddingLeft() + innerW - itemMarginRight - childW;
        else if (hasFlag(childAlign, Alignment::CenterHorizontal))
            childX = paddingLeft() + itemMarginLeft + (innerW - itemMarginLeft - itemMarginRight - childW) * 0.5f;
        else
            childX = paddingLeft() + itemMarginLeft + static_cast<float>(item->x());

        item->setResolved(childX, cursorY + itemMarginTop, childW, childH);
        cursorY += itemMarginTop + childH + itemMarginBottom + m_spacing;
        item->layout(true);
    }
}

Widget::Size ColumnWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    float totalH = 0.0f, maxW = 0.0f;
    for (auto* item : items) {
        const Size sz = item->intrinsicSize();
        totalH += sz.height + item->marginTop() + item->marginBottom();
        maxW = std::max(maxW, sz.width + item->marginLeft() + item->marginRight());
    }
    if (items.size() > 1)
        totalH += m_spacing * static_cast<float>(items.size() - 1);

    return { (width() > 0) ? static_cast<float>(width()) : maxW + paddingLeft() + paddingRight(),
            (height() > 0) ? static_cast<float>(height()) : totalH + paddingTop() + paddingBottom() };
}

}