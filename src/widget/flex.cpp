#include <tavoos/widget/flex.h>
#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

FlexWidget::FlexWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_direction, m_wrap, m_justifyContent, m_alignItems,
                         m_alignContent, m_rowGap, m_columnGap);
}

FlexWidget::FlexMetrics FlexWidget::computeMetrics(float availableMainSpace) const {
    FlexMetrics metrics;

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    const bool horizontalMain = (m_direction.get() == FlexDirection::Row);
    const float mainGap = horizontalMain ? m_columnGap.get() : m_rowGap.get();
    const float crossGap = horizontalMain ? m_rowGap.get() : m_columnGap.get();
    const bool wrapping = (m_wrap.get() == FlexWrap::Wrap);

    auto mainOf  = [&](const Size& s) { return horizontalMain ? s.width : s.height; };
    auto crossOf = [&](const Size& s) { return horizontalMain ? s.height : s.width; };

    FlexLine current;
    float currentMain = 0.0f;

    auto mainMarginOf = [&](Widget* w) {
        return horizontalMain ? (w->marginLeft() + w->marginRight()) : (w->marginTop() + w->marginBottom());
    };
    auto crossMarginOf = [&](Widget* w) {
        return horizontalMain ? (w->marginTop() + w->marginBottom()) : (w->marginLeft() + w->marginRight());
    };

    for (auto* item : items) {
        const Size sz = item->intrinsicSize();
        const float itemMain = mainOf(sz) + mainMarginOf(item);
        float addGap = current.items.empty() ? 0.0f : mainGap;

        if (wrapping && !current.items.empty() && currentMain + addGap + itemMain > availableMainSpace) {
            metrics.lines.push_back(std::move(current));
            current = FlexLine{};
            currentMain = 0.0f;
            addGap = 0.0f;
        }

        current.items.push_back(item);
        current.itemSizes.push_back(sz);
        currentMain += addGap + itemMain;
        current.crossSize = std::max(current.crossSize, crossOf(sz) + crossMarginOf(item));
    }
    if (!current.items.empty())
        metrics.lines.push_back(std::move(current));

    for (auto& line : metrics.lines) {
        float m = 0.0f;
        for (std::size_t i = 0; i < line.itemSizes.size(); ++i) {
            if (i > 0)
                m += mainGap;
            m += mainOf(line.itemSizes[i]) + mainMarginOf(line.items[i]);
        }
        line.mainSize = m;
    }

    float naturalMain = 0.0f;
    for (auto& line : metrics.lines)
        naturalMain = std::max(naturalMain, line.mainSize);

    float naturalCross = 0.0f;
    for (std::size_t i = 0; i < metrics.lines.size(); ++i) {
        if (i > 0)
            naturalCross += crossGap;
        naturalCross += metrics.lines[i].crossSize;
    }

    metrics.naturalMainSize = naturalMain;
    metrics.naturalCrossSize = naturalCross;

    return metrics;
}

void FlexWidget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    const bool horizontalMain = (m_direction.get() == FlexDirection::Row);
    const float mainGap = horizontalMain ? m_columnGap.get() : m_rowGap.get();
    const float crossGap = horizontalMain ? m_rowGap.get() : m_columnGap.get();

    float resolvedW, resolvedH, resX, resY;

    if (parentIsLayouter()) {
        resolvedW = resolvedWidth();
        resolvedH = resolvedHeight();
        resX = resolvedX();
        resY = resolvedY();
    } else {
        const ContentArea area = resolveContentArea();

        float naturalW = 0.0f, naturalH = 0.0f;
        const bool needNaturalW = !hasFlag(fill(), Fill::Width) && width() <= 0;
        const bool needNaturalH = !hasFlag(fill(), Fill::Height) && height() <= 0;
        if (needNaturalW || needNaturalH) {
            const FlexMetrics naturalMetrics = computeMetrics(kUnbounded);
            naturalW = horizontalMain ? naturalMetrics.naturalMainSize : naturalMetrics.naturalCrossSize;
            naturalH = horizontalMain ? naturalMetrics.naturalCrossSize : naturalMetrics.naturalMainSize;
        }

        resolvedH = hasFlag(fill(), Fill::Height) ? area.height - marginTop() - marginBottom()
                    : (height() > 0) ? static_cast<float>(height()) : naturalH + paddingTop() + paddingBottom();
        resolvedW = hasFlag(fill(), Fill::Width)  ? area.width - marginLeft() - marginRight()
                    : (width()  > 0) ? static_cast<float>(width())  : naturalW + paddingLeft() + paddingRight();

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

    const float availableMain  = horizontalMain ? innerW : innerH;
    const float availableCross = horizontalMain ? innerH : innerW;

    auto mainOf  = [&](const Size& s) { return horizontalMain ? s.width : s.height; };
    auto crossOf = [&](const Size& s) { return horizontalMain ? s.height : s.width; };

    const FlexMetrics metrics = computeMetrics(availableMain);
    const int lineCount = static_cast<int>(metrics.lines.size());

    std::vector<float> lineCrossSize(lineCount);
    for (int i = 0; i < lineCount; ++i)
        lineCrossSize[i] = metrics.lines[i].crossSize;

    if (m_alignContent.get() == FlexAlign::Stretch && lineCount > 0) {
        const float natural = metrics.naturalCrossSize;
        const float leftover = std::max(0.0f, availableCross - natural);
        const float share = leftover / static_cast<float>(lineCount);
        for (auto& cs : lineCrossSize)
            cs += share;
    }

    float totalLineCross = 0.0f;
    for (int i = 0; i < lineCount; ++i) {
        if (i > 0)
            totalLineCross += crossGap;
        totalLineCross += lineCrossSize[i];
    }
    const float crossLeftover = std::max(0.0f, availableCross - totalLineCross);

    float crossOffset = 0.0f, crossBetween = crossGap;
    switch (m_alignContent.get()) {
    case FlexAlign::Center:
        crossOffset = crossLeftover * 0.5f;
        break;
    case FlexAlign::End:
        crossOffset = crossLeftover;
        break;
    case FlexAlign::SpaceBetween:
        crossBetween = crossGap + (lineCount > 1 ? crossLeftover / static_cast<float>(lineCount - 1) : 0.0f);
        break;
    case FlexAlign::SpaceAround: {
        const float extra = lineCount > 0 ? crossLeftover / static_cast<float>(lineCount) : 0.0f;
        crossOffset = extra * 0.5f;
        crossBetween = crossGap + extra;
        break;
    }
    case FlexAlign::SpaceEvenly: {
        const float extra = lineCount > 0 ? crossLeftover / static_cast<float>(lineCount + 1) : 0.0f;
        crossOffset = extra;
        crossBetween = crossGap + extra;
        break;
    }
    default:
        break;
    }

    float crossCursor = crossOffset;
    for (int li = 0; li < lineCount; ++li) {
        const FlexLine& line = metrics.lines[li];
        const float thisLineCross = lineCrossSize[li];
        const std::size_t itemCount = line.items.size();

        int fillCount = 0;
        float fixedMain = 0.0f;
        for (std::size_t i = 0; i < itemCount; ++i) {
            const bool fillsMain = hasFlag(line.items[i]->fill(), horizontalMain ? Fill::Width : Fill::Height);
            const float itemMainMargin = horizontalMain
                ? (line.items[i]->marginLeft() + line.items[i]->marginRight())
                : (line.items[i]->marginTop() + line.items[i]->marginBottom());
            if (fillsMain) {
                ++fillCount;
                fixedMain += itemMainMargin;
            } else {
                fixedMain += mainOf(line.itemSizes[i]) + itemMainMargin;
            }
        }
        if (itemCount > 1)
            fixedMain += mainGap * static_cast<float>(itemCount - 1);

        const float mainLeftover = std::max(0.0f, availableMain - fixedMain);
        const float fillShare = (fillCount > 0) ? mainLeftover / static_cast<float>(fillCount) : 0.0f;
        const float justifyLeftover = (fillCount > 0) ? 0.0f : mainLeftover;

        float mainOffset = 0.0f, mainBetween = mainGap;
        switch (m_justifyContent.get()) {
        case FlexJustify::Center:
            mainOffset = justifyLeftover * 0.5f;
            break;
        case FlexJustify::End:
            mainOffset = justifyLeftover;
            break;
        case FlexJustify::SpaceBetween:
            mainBetween = mainGap + (itemCount > 1 ? justifyLeftover / static_cast<float>(itemCount - 1) : 0.0f);
            break;
        case FlexJustify::SpaceAround: {
            const float extra = itemCount > 0 ? justifyLeftover / static_cast<float>(itemCount) : 0.0f;
            mainOffset = extra * 0.5f;
            mainBetween = mainGap + extra;
            break;
        }
        case FlexJustify::SpaceEvenly: {
            const float extra = itemCount > 0 ? justifyLeftover / static_cast<float>(itemCount + 1) : 0.0f;
            mainOffset = extra;
            mainBetween = mainGap + extra;
            break;
        }
        default:
            break;
        }

        float mainCursor = mainOffset;
        for (std::size_t i = 0; i < itemCount; ++i) {
            Widget* const item = line.items[i];
            const bool fillsMain  = hasFlag(item->fill(), horizontalMain ? Fill::Width  : Fill::Height);
            const bool fillsCross = hasFlag(item->fill(), horizontalMain ? Fill::Height : Fill::Width);
            const float itemMainMarginLeading   = horizontalMain ? item->marginLeft() : item->marginTop();
            const float itemMainMarginTrailing  = horizontalMain ? item->marginRight() : item->marginBottom();
            const float itemCrossMarginLeading  = horizontalMain ? item->marginTop() : item->marginLeft();
            const float itemCrossMarginTrailing = horizontalMain ? item->marginBottom() : item->marginRight();

            const float itemMain  = fillsMain  ? fillShare : mainOf(line.itemSizes[i]);
            const float itemCross = fillsCross ? (thisLineCross - itemCrossMarginLeading - itemCrossMarginTrailing)
                                                : crossOf(line.itemSizes[i]);

            const Alignment itemAlign = item->alignment();
            const Alignment effectiveAlign = (itemAlign == Alignment::None) ? m_alignItems.get() : itemAlign;

            float crossPos;
            if (fillsCross) {
                crossPos = crossCursor + itemCrossMarginLeading;
            } else {
                bool alignEnd, alignCenter;
                if (horizontalMain) {
                    alignEnd = hasFlag(effectiveAlign, Alignment::Bottom);
                    alignCenter = hasFlag(effectiveAlign, Alignment::CenterVertical);
                } else {
                    alignEnd = hasFlag(effectiveAlign, Alignment::Right);
                    alignCenter = hasFlag(effectiveAlign, Alignment::CenterHorizontal);
                }
                if (alignEnd)
                    crossPos = crossCursor + thisLineCross - itemCrossMarginTrailing - itemCross;
                else if (alignCenter)
                    crossPos = crossCursor + itemCrossMarginLeading
                                + (thisLineCross - itemCrossMarginLeading - itemCrossMarginTrailing - itemCross) * 0.5f;
                else
                    crossPos = crossCursor + itemCrossMarginLeading;
            }

            const float mainPos = mainCursor + itemMainMarginLeading;
            const float childX = paddingLeft() + (horizontalMain ? mainPos : crossPos);
            const float childY = paddingTop()  + (horizontalMain ? crossPos : mainPos);
            const float childW = horizontalMain ? itemMain : itemCross;
            const float childH = horizontalMain ? itemCross : itemMain;

            item->setResolved(childX, childY, childW, childH);
            item->layout(true);

            mainCursor += itemMainMarginLeading + itemMain + itemMainMarginTrailing + mainBetween;
        }

        crossCursor += thisLineCross + crossBetween;
    }
}

Widget::Size FlexWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };

    const bool horizontalMain = (m_direction.get() == FlexDirection::Row);
    const FlexMetrics metrics = computeMetrics(kUnbounded);

    const float naturalW = horizontalMain ? metrics.naturalMainSize : metrics.naturalCrossSize;
    const float naturalH = horizontalMain ? metrics.naturalCrossSize : metrics.naturalMainSize;

    return { (width() > 0) ? static_cast<float>(width()) : naturalW + paddingLeft() + paddingRight(),
            (height() > 0) ? static_cast<float>(height()) : naturalH + paddingTop() + paddingBottom() };
}

}
