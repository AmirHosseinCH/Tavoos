#include <tavoos/widget/templates/listviewbase.h>

#include <tavoos/gfx/renderer.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace Tavoos {

namespace {

constexpr float kInitialEstimate = 40.0f;
constexpr float kMinimumHeight = 1.0f;
constexpr float kHeightTolerance = 0.01f;
constexpr int kMaxPasses = 8;

}

ListViewBase::ListViewBase(Object* parent) : ScrollAreaBase{parent} {
    flickDirection(FlickDirection::Vertical);

    m_count.onChange([this](const int&) {
        if (built())
            applyCount();
    });
    m_spacing.onChange([this](const float&) {
        if (!built())
            return;
        rebuildOffsets();
        refreshRows();
    });
    m_cacheBuffer.onChange([this](const float&) {
        if (built())
            refreshRows();
    });
}

int ListViewBase::rowCount() const {
    return std::max(0, m_count.get());
}

float ListViewBase::gap() const {
    return std::max(0.0f, m_spacing.get());
}

float ListViewBase::heightOf(int index) const {
    const float height = m_heights[static_cast<std::size_t>(index)];
    return height > 0.0f ? height : m_estimate;
}

int ListViewBase::indexAt(float y) const {
    const int total = rowCount();
    if (total == 0 || y <= 0.0f)
        return 0;

    const auto begin = m_tops.begin();
    const auto next = std::upper_bound(begin, begin + total, y);
    return std::clamp(static_cast<int>(next - begin) - 1, 0, total - 1);
}

int ListViewBase::rowIndexOf(const Widget* child) const {
    const auto found = std::ranges::find(m_rows, child);
    return found == m_rows.end() ? -1 : m_first + static_cast<int>(found - m_rows.begin());
}

Widget* ListViewBase::rowAt(int index) const {
    const int offset = index - m_first;
    if (offset < 0 || offset >= static_cast<int>(m_rows.size()))
        return nullptr;
    return m_rows[static_cast<std::size_t>(offset)];
}

void ListViewBase::rebuildOffsets() {
    const int total = rowCount();

    float known = 0.0f;
    int measured = 0;
    for (const float height : m_heights) {
        if (height > 0.0f) {
            known += height;
            ++measured;
        }
    }
    m_estimate = measured > 0 ? known / static_cast<float>(measured) : kInitialEstimate;

    m_tops.assign(static_cast<std::size_t>(total) + 1, 0.0f);
    float y = 0.0f;
    for (int i = 0; i < total; ++i) {
        m_tops[static_cast<std::size_t>(i)] = y;
        y += heightOf(i) + gap();
    }
    m_tops[static_cast<std::size_t>(total)] = y;
}

float ListViewBase::measure(Widget& row) {
    const Size size = row.intrinsicSize();
    return std::max(kMinimumHeight, size.height + row.marginTop() + row.marginBottom());
}

bool ListViewBase::measureRows() {
    bool changed = false;
    for (std::size_t k = 0; k < m_rows.size(); ++k) {
        const std::size_t index = static_cast<std::size_t>(m_first) + k;
        const float height = measure(*m_rows[k]);
        if (std::fabs(height - m_heights[index]) > kHeightTolerance) {
            m_heights[index] = height;
            changed = true;
        }
    }
    return changed;
}

Widget::Size ListViewBase::computeContentSize() {
    if (!built())
        return { 0.0f, 0.0f };

    const int total = rowCount();
    return { 0.0f, total > 0 ? m_tops[static_cast<std::size_t>(total)] - gap() : 0.0f };
}

Widget::ContentArea ListViewBase::contentAreaFor(const Widget& child) const {
    ContentArea area = ScrollAreaBase::contentAreaFor(child);
    const int index = rowIndexOf(&child);
    if (index < 0)
        return area;

    area.y += m_tops[static_cast<std::size_t>(index)];
    area.height = heightOf(index);
    return area;
}

void ListViewBase::onBuilt() {
    m_heights.assign(static_cast<std::size_t>(rowCount()), 0.0f);
    rebuildOffsets();
    if (m_pendingIndex >= 0)
        scrollToIndex(std::exchange(m_pendingIndex, -1));
    ScrollAreaBase::onBuilt();
}

void ListViewBase::render(Renderer& renderer) {
    if (!renderer.isCapturingMask())
        syncRows();
    ScrollAreaBase::render(renderer);
}

void ListViewBase::onOffsetsSynced(bool offsetsChanged) {
    syncRows();
    ScrollAreaBase::onOffsetsSynced(offsetsChanged);
}

void ListViewBase::syncRows() {
    if (m_syncing)
        return;

    if (rowCount() == 0 || !m_delegate) {
        m_target = -1;
        clearRows();
        updateVisible();
        return;
    }

    if (viewportHeight() <= 0.0f) {
        updateVisible();
        return;
    }

    m_syncing = true;
    int pass = 0;
    while (pass++ < kMaxPasses && syncPass()) {
    }
    m_syncing = false;
    m_target = -1;
    updateVisible();
}

bool ListViewBase::syncPass() {
    const float top = contentY();
    const float bottom = top + viewportHeight();
    const float buffer = std::max(0.0f, m_cacheBuffer.get());
    const int target = std::min(m_target, rowCount() - 1);
    const int anchor = target >= 0 ? target : indexAt(top);
    const float anchorOffset = target >= 0 ? 0.0f : top - m_tops[static_cast<std::size_t>(anchor)];
    const int from = indexAt(top - buffer);
    const int to = indexAt(bottom + buffer);

    while (!m_rows.empty() && m_first < from) {
        removeChild(m_rows.front());
        m_rows.pop_front();
        ++m_first;
    }
    while (!m_rows.empty() && m_first + static_cast<int>(m_rows.size()) - 1 > to) {
        removeChild(m_rows.back());
        m_rows.pop_back();
    }
    if (m_rows.empty())
        m_first = from;

    while (m_first > from) {
        m_rows.push_front(m_delegate(m_first - 1));
        --m_first;
    }
    while (m_first + static_cast<int>(m_rows.size()) - 1 < to)
        m_rows.push_back(m_delegate(m_first + static_cast<int>(m_rows.size())));

    if (!measureRows())
        return false;

    rebuildOffsets();
    markLayoutDirty();
    scrollTo(contentX(), m_tops[static_cast<std::size_t>(anchor)] + anchorOffset);
    return true;
}

void ListViewBase::updateVisible() {
    const int total = rowCount();
    if (total == 0 || viewportHeight() <= 0.0f) {
        m_firstVisible.setIfChanged(-1);
        m_lastVisible.setIfChanged(-1);
        return;
    }

    const float top = contentY();
    const float bottom = top + viewportHeight();
    int first = indexAt(top);
    if (top >= m_tops[static_cast<std::size_t>(first)] + heightOf(first))
        first = std::min(first + 1, total - 1);
    const int last = std::max(first, indexAt(bottom - kHeightTolerance));
    m_firstVisible.setIfChanged(first);
    m_lastVisible.setIfChanged(last);
}

void ListViewBase::applyCount() {
    const int total = rowCount();
    m_heights.resize(static_cast<std::size_t>(total), 0.0f);
    removeRowsBeyond(total);
    rebuildOffsets();
    refreshRows();
}

void ListViewBase::removeRowsBeyond(int total) {
    while (!m_rows.empty() && m_first + static_cast<int>(m_rows.size()) - 1 >= total) {
        removeChild(m_rows.back());
        m_rows.pop_back();
    }
    if (m_rows.empty())
        m_first = 0;
}

void ListViewBase::refreshRows() {
    scrollTo(contentX(), contentY());
}

void ListViewBase::clearRows() {
    for (Widget* const row : m_rows)
        removeChild(row);
    m_rows.clear();
    m_first = 0;
}

void ListViewBase::scrollToIndex(int index) {
    if (!built()) {
        m_pendingIndex = std::max(0, index);
        return;
    }

    const int total = rowCount();
    if (total == 0)
        return;

    m_target = std::clamp(index, 0, total - 1);
    m_syncing = true;
    scrollTo(contentX(), m_tops[static_cast<std::size_t>(m_target)]);
    m_syncing = false;
    syncRows();
}

void ListViewBase::reload() {
    clearRows();
    m_heights.assign(static_cast<std::size_t>(rowCount()), 0.0f);
    rebuildOffsets();
    refreshRows();
}

}
