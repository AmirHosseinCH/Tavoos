#include <tavoos/widget/row.h>
#include <tavoos/window.h>

namespace Tavoos {

RowWidget::RowWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_spacing);
}

void RowWidget::layout(bool force) {
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

    float fixedWidth = 0.0f;
    int fillCount = 0;
    float maxChildHeight = 0.0f;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const float hMargin = items[i]->marginLeft() + items[i]->marginRight();
        const float vMargin = items[i]->marginTop() + items[i]->marginBottom();
        if (hasFlag(items[i]->fill(), Fill::Width)) {
            ++fillCount;
            fixedWidth += hMargin;
        } else {
            fixedWidth += itemSizes[i].width + hMargin;
        }
        if (!hasFlag(items[i]->fill(), Fill::Height))
            maxChildHeight = std::max(maxChildHeight, itemSizes[i].height + vMargin);
    }
    if (items.size() > 1)
        fixedWidth += m_spacing * static_cast<float>(items.size() - 1);

    float resolvedW, resolvedH, resX, resY;

    if (parentIsLayouter()) {
        resolvedW = resolvedWidth();
        resolvedH = resolvedHeight();
        resX = resolvedX();
        resY = resolvedY();
    } else {
        const ContentArea area = resolveContentArea();

        resolvedW = hasFlag(fill(), Fill::Width)  ? area.width - marginLeft() - marginRight()
                    : (width()  > 0) ? static_cast<float>(width())  : fixedWidth + paddingLeft() + paddingRight();
        resolvedH = hasFlag(fill(), Fill::Height) ? area.height - marginTop()  - marginBottom()
                    : (height() > 0) ? static_cast<float>(height()) : maxChildHeight + paddingTop() + paddingBottom();

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
    const float leftover = std::max(0.0f, innerW - fixedWidth);
    const float fillShare = (fillCount > 0) ? leftover / static_cast<float>(fillCount) : 0.0f;

    float cursorX = paddingLeft();
    for (std::size_t i = 0; i < items.size(); ++i) {
        Widget* const item = items[i];
        const bool fillsWidth  = hasFlag(item->fill(), Fill::Width);
        const bool fillsHeight = hasFlag(item->fill(), Fill::Height);
        const float itemMarginLeft = item->marginLeft();
        const float itemMarginRight = item->marginRight();
        const float itemMarginTop = item->marginTop();
        const float itemMarginBottom = item->marginBottom();

        const float childW = fillsWidth  ? fillShare : itemSizes[i].width;
        const float childH = fillsHeight ? (innerH - itemMarginTop - itemMarginBottom) : itemSizes[i].height;

        const Alignment childAlign = item->alignment();
        float childY;
        if (fillsHeight)
            childY = paddingTop() + itemMarginTop;
        else if (hasFlag(childAlign, Alignment::Bottom))
            childY = paddingTop() + innerH - itemMarginBottom - childH;
        else if (hasFlag(childAlign, Alignment::CenterVertical))
            childY = paddingTop() + itemMarginTop + (innerH - itemMarginTop - itemMarginBottom - childH) * 0.5f;
        else
            childY = paddingTop() + itemMarginTop + static_cast<float>(item->y());

        item->setResolved(cursorX + itemMarginLeft, childY, childW, childH);
        cursorX += itemMarginLeft + childW + itemMarginRight + m_spacing;
        item->layout(true);
    }
}

Widget::Size RowWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    float totalW = 0.0f, maxH = 0.0f;
    for (auto* item : items) {
        const Size sz = item->intrinsicSize();
        totalW += sz.width + item->marginLeft() + item->marginRight();
        maxH = std::max(maxH, sz.height + item->marginTop() + item->marginBottom());
    }
    if (items.size() > 1)
        totalW += m_spacing * static_cast<float>(items.size() - 1);

    return { (width() > 0) ? static_cast<float>(width()) : totalW + paddingLeft() + paddingRight(),
            (height() > 0) ? static_cast<float>(height()) : maxH + paddingTop() + paddingBottom() };
}

}