#include <tavoos/widget/grid.h>
#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

GridWidget::GridWidget(Object* parent) : Widget{parent} {
    bindRelayoutTriggers(m_columns, m_columnSpacing, m_rowSpacing);
}

GridWidget::GridMetrics GridWidget::computeMetrics() const {
    GridMetrics metrics;

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    const int cols = std::max(1, m_columns.get());

    std::vector<std::vector<bool>> occupied;
    auto isFree = [&](int row, int col) {
        if (row >= static_cast<int>(occupied.size()))
            return true;
        return !occupied[row][col];
    };
    auto fits = [&](int row, int col, int rowSpan, int colSpan) {
        if (col + colSpan > cols)
            return false;
        for (int r = row; r < row + rowSpan; ++r)
            for (int c = col; c < col + colSpan; ++c)
                if (!isFree(r, c))
                    return false;
        return true;
    };
    auto occupy = [&](int row, int col, int rowSpan, int colSpan) {
        while (static_cast<int>(occupied.size()) < row + rowSpan)
            occupied.emplace_back(cols, false);
        for (int r = row; r < row + rowSpan; ++r)
            for (int c = col; c < col + colSpan; ++c)
                occupied[r][c] = true;
    };

    metrics.placements.reserve(items.size());

    for (auto* item : items) {
        if (item->gridRow() < 0 || item->gridColumn() < 0)
            continue;

        const int colSpan = std::clamp(item->gridColumnSpan(), 1, cols);
        const int rowSpan = std::max(1, item->gridRowSpan());
        const int col = std::clamp(item->gridColumn(), 0, cols - colSpan);
        const int row = item->gridRow();

        occupy(row, col, rowSpan, colSpan);
        metrics.placements.push_back({item, row, col, rowSpan, colSpan});
    }

    int cursorRow = 0, cursorCol = 0;

    for (auto* item : items) {
        const bool rowSet = item->gridRow() >= 0;
        const bool colSet = item->gridColumn() >= 0;
        if (rowSet && colSet)
            continue;

        const int colSpan = std::clamp(item->gridColumnSpan(), 1, cols);
        const int rowSpan = std::max(1, item->gridRowSpan());
        int row, col;

        if (colSet) {
            col = std::clamp(item->gridColumn(), 0, cols - colSpan);
            row = 0;
            while (!fits(row, col, rowSpan, colSpan))
                ++row;
        } else if (rowSet) {
            row = item->gridRow();
            col = 0;
            while (true) {
                while (col + colSpan <= cols && !fits(row, col, rowSpan, colSpan))
                    ++col;
                if (col + colSpan <= cols)
                    break;
                col = 0;
                ++row;
            }
        } else {
            row = cursorRow;
            col = cursorCol;
            while (true) {
                if (col + colSpan > cols) {
                    col = 0;
                    ++row;
                    continue;
                }
                if (fits(row, col, rowSpan, colSpan))
                    break;
                ++col;
            }
            cursorRow = row;
            cursorCol = col + colSpan;
            if (cursorCol >= cols) {
                cursorCol = 0;
                ++cursorRow;
            }
        }

        occupy(row, col, rowSpan, colSpan);
        metrics.placements.push_back({item, row, col, rowSpan, colSpan});
    }

    const int rowCount = static_cast<int>(occupied.size());
    metrics.columnWidths.assign(cols, 0.0f);
    metrics.rowHeights.assign(rowCount, 0.0f);

    std::vector<Size> itemSizes;
    itemSizes.reserve(metrics.placements.size());
    for (auto& p : metrics.placements)
        itemSizes.push_back(p.widget->intrinsicSize());

    for (std::size_t i = 0; i < metrics.placements.size(); ++i) {
        const auto& p = metrics.placements[i];
        if (p.colSpan == 1)
            metrics.columnWidths[p.col] = std::max(metrics.columnWidths[p.col],
                itemSizes[i].width + p.widget->marginLeft() + p.widget->marginRight());
        if (p.rowSpan == 1)
            metrics.rowHeights[p.row] = std::max(metrics.rowHeights[p.row],
                itemSizes[i].height + p.widget->marginTop() + p.widget->marginBottom());
    }

    for (std::size_t i = 0; i < metrics.placements.size(); ++i) {
        const auto& p = metrics.placements[i];
        if (p.colSpan > 1) {
            float span = 0.0f;
            for (int c = p.col; c < p.col + p.colSpan; ++c)
                span += metrics.columnWidths[c];
            span += m_columnSpacing * static_cast<float>(p.colSpan - 1);
            const float deficit = itemSizes[i].width + p.widget->marginLeft() + p.widget->marginRight() - span;
            if (deficit > 0.0f) {
                const float extra = deficit / static_cast<float>(p.colSpan);
                for (int c = p.col; c < p.col + p.colSpan; ++c)
                    metrics.columnWidths[c] += extra;
            }
        }
        if (p.rowSpan > 1) {
            float span = 0.0f;
            for (int r = p.row; r < p.row + p.rowSpan; ++r)
                span += metrics.rowHeights[r];
            span += m_rowSpacing * static_cast<float>(p.rowSpan - 1);
            const float deficit = itemSizes[i].height + p.widget->marginTop() + p.widget->marginBottom() - span;
            if (deficit > 0.0f) {
                const float extra = deficit / static_cast<float>(p.rowSpan);
                for (int r = p.row; r < p.row + p.rowSpan; ++r)
                    metrics.rowHeights[r] += extra;
            }
        }
    }

    return metrics;
}

void GridWidget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    const GridMetrics metrics = computeMetrics();

    const int cols = static_cast<int>(metrics.columnWidths.size());
    const int rows = static_cast<int>(metrics.rowHeights.size());

    float naturalW = 0.0f;
    for (float w : metrics.columnWidths) naturalW += w;
    if (cols > 1)
        naturalW += m_columnSpacing * static_cast<float>(cols - 1);

    float naturalH = 0.0f;
    for (float h : metrics.rowHeights) naturalH += h;
    if (rows > 1)
        naturalH += m_rowSpacing * static_cast<float>(rows - 1);

    float resolvedW, resolvedH, resX, resY;

    if (parentIsLayouter()) {
        resolvedW = resolvedWidth();
        resolvedH = resolvedHeight();
        resX = resolvedX();
        resY = resolvedY();
    } else {
        const ContentArea area = resolveContentArea();

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

    const float extraW = std::max(0.0f, innerW - naturalW);
    const float extraH = std::max(0.0f, innerH - naturalH);

    std::vector<float> columnWidths = metrics.columnWidths;
    std::vector<float> rowHeights = metrics.rowHeights;

    if (cols > 0 && extraW > 0.0f) {
        const float share = extraW / static_cast<float>(cols);
        for (float& w : columnWidths) w += share;
    }
    if (rows > 0 && extraH > 0.0f) {
        const float share = extraH / static_cast<float>(rows);
        for (float& h : rowHeights) h += share;
    }

    std::vector<float> colX(cols + 1, 0.0f);
    for (int c = 0; c < cols; ++c)
        colX[c + 1] = colX[c] + columnWidths[c] + m_columnSpacing;

    std::vector<float> rowY(rows + 1, 0.0f);
    for (int r = 0; r < rows; ++r)
        rowY[r + 1] = rowY[r] + rowHeights[r] + m_rowSpacing;

    for (const auto& p : metrics.placements) {
        Widget* const item = p.widget;
        const float itemMarginLeft = item->marginLeft();
        const float itemMarginRight = item->marginRight();
        const float itemMarginTop = item->marginTop();
        const float itemMarginBottom = item->marginBottom();

        const float cellX = paddingLeft() + colX[p.col];
        const float cellY = paddingTop() + rowY[p.row];

        float cellW = 0.0f;
        for (int c = p.col; c < p.col + p.colSpan; ++c)
            cellW += columnWidths[c];
        if (p.colSpan > 1)
            cellW += m_columnSpacing * static_cast<float>(p.colSpan - 1);

        float cellH = 0.0f;
        for (int r = p.row; r < p.row + p.rowSpan; ++r)
            cellH += rowHeights[r];
        if (p.rowSpan > 1)
            cellH += m_rowSpacing * static_cast<float>(p.rowSpan - 1);

        const Size itemSize = item->intrinsicSize();
        const bool fillsWidth  = hasFlag(item->fill(), Fill::Width);
        const bool fillsHeight = hasFlag(item->fill(), Fill::Height);

        const float childW = fillsWidth  ? (cellW - itemMarginLeft - itemMarginRight) : itemSize.width;
        const float childH = fillsHeight ? (cellH - itemMarginTop - itemMarginBottom) : itemSize.height;

        const Alignment childAlign = item->alignment();
        float childX;
        if (fillsWidth)
            childX = cellX + itemMarginLeft;
        else if (hasFlag(childAlign, Alignment::Right))
            childX = cellX + cellW - itemMarginRight - childW;
        else if (hasFlag(childAlign, Alignment::CenterHorizontal))
            childX = cellX + itemMarginLeft + (cellW - itemMarginLeft - itemMarginRight - childW) * 0.5f;
        else
            childX = cellX + itemMarginLeft + static_cast<float>(item->x());

        float childY;
        if (fillsHeight)
            childY = cellY + itemMarginTop;
        else if (hasFlag(childAlign, Alignment::Bottom))
            childY = cellY + cellH - itemMarginBottom - childH;
        else if (hasFlag(childAlign, Alignment::CenterVertical))
            childY = cellY + itemMarginTop + (cellH - itemMarginTop - itemMarginBottom - childH) * 0.5f;
        else
            childY = cellY + itemMarginTop + static_cast<float>(item->y());

        item->setResolved(childX, childY, childW, childH);
        item->layout(true);
    }
}

Widget::Size GridWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };

    const GridMetrics metrics = computeMetrics();

    const int cols = static_cast<int>(metrics.columnWidths.size());
    const int rows = static_cast<int>(metrics.rowHeights.size());

    float totalW = 0.0f;
    for (float w : metrics.columnWidths) totalW += w;
    if (cols > 1)
        totalW += m_columnSpacing * static_cast<float>(cols - 1);

    float totalH = 0.0f;
    for (float h : metrics.rowHeights) totalH += h;
    if (rows > 1)
        totalH += m_rowSpacing * static_cast<float>(rows - 1);

    return { (width() > 0) ? static_cast<float>(width()) : totalW + paddingLeft() + paddingRight(),
            (height() > 0) ? static_cast<float>(height()) : totalH + paddingTop() + paddingBottom() };
}

}
