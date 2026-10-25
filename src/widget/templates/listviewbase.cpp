#include <tavoos/widget/templates/listviewbase.h>

#include <tavoos/builder.h>
#include <tavoos/events/key.h>
#include <tavoos/gfx/renderer.h>

#include <spdlog/spdlog.h>

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
        if (!built() || m_model)
            return;
        applyCount();
        if (currentIndex() >= rowCount())
            setCurrentIndexQuietly(-1);
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
    m_currentIndex.onChange([this](const int& index) { applyCurrentIndex(index); });
    m_highlightFollowsCurrent.onChange([this](const bool&) { updateHighlight(false); });
    m_highlightTop.onChange([this](const float&) { markLayoutDirty(); });
    m_highlightHeight.onChange([this](const float&) { markLayoutDirty(); });
}

ListViewBase::~ListViewBase() {
    detachModel();
}

void ListViewBase::runInBuilderScope(Widget& row, const std::function<void()>& call) {
    Builder::Scope scope{&row};
    call();
}

int ListViewBase::rowCount() const {
    return m_model ? m_model->size() : std::max(0, m_count.get());
}

int ListViewBase::trackedCount() const {
    return static_cast<int>(m_heights.size());
}

float ListViewBase::gap() const {
    return std::max(0.0f, m_spacing.get());
}

float ListViewBase::heightOf(int index) const {
    const float height = m_heights[static_cast<std::size_t>(index)];
    return height > 0.0f ? height : m_estimate;
}

int ListViewBase::indexAt(float y) const {
    const int total = trackedCount();
    if (total == 0 || y <= 0.0f)
        return 0;

    const auto begin = m_tops.begin();
    const auto next = std::upper_bound(begin, begin + total, y);
    return std::clamp(static_cast<int>(next - begin) - 1, 0, total - 1);
}

int ListViewBase::rowIndexOf(const Widget* child) const {
    const auto found = std::ranges::find(m_rows, child, &LiveRow::widget);
    return found == m_rows.end() ? -1 : m_first + static_cast<int>(found - m_rows.begin());
}

Widget* ListViewBase::rowAt(int index) const {
    const int offset = index - m_first;
    if (offset < 0 || offset >= static_cast<int>(m_rows.size()))
        return nullptr;
    return m_rows[static_cast<std::size_t>(offset)].widget;
}

void ListViewBase::rebuildOffsets() {
    const int total = trackedCount();

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
    m_rowsHeight.setIfChanged(total > 0 ? y - gap() : 0.0f);
}

float ListViewBase::measure(Widget& row) {
    const Size size = row.intrinsicSize();
    return std::max(kMinimumHeight, size.height + row.marginTop() + row.marginBottom());
}

bool ListViewBase::measureRows() {
    bool changed = false;
    for (std::size_t k = 0; k < m_rows.size(); ++k) {
        const std::size_t index = static_cast<std::size_t>(m_first) + k;
        const float height = measure(*m_rows[k].widget);
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

    const int total = trackedCount();
    return { 0.0f, total > 0 ? m_tops[static_cast<std::size_t>(total)] - gap() : 0.0f };
}

Widget::ContentArea ListViewBase::contentAreaFor(const Widget& child) const {
    ContentArea area = ScrollAreaBase::contentAreaFor(child);
    if (&child == m_highlight && m_highlightFollowsCurrent.get()) {
        area.y += m_highlightTop.get();
        area.height = m_highlightHeight.get();
        return area;
    }

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
    const int requested = m_currentIndexState.get();
    const int valid = requested >= 0 && requested < rowCount() ? requested : -1;
    if (valid != requested)
        setCurrentIndexQuietly(valid);
    if (m_pendingIndex >= 0)
        scrollToIndex(std::exchange(m_pendingIndex, -1));
    ScrollAreaBase::onBuilt();
}

void ListViewBase::render(Renderer& renderer) {
    m_retiredStates.clear();
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
    updateHighlight(false);
    if (std::exchange(m_reloadPending, false))
        reload();
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
        retireRow(m_rows.front());
        m_rows.pop_front();
        ++m_first;
    }
    while (!m_rows.empty() && m_first + static_cast<int>(m_rows.size()) - 1 > to) {
        retireRow(m_rows.back());
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
        retireRow(m_rows.back());
        m_rows.pop_back();
    }
    if (m_rows.empty())
        m_first = 0;
}

void ListViewBase::refreshRows() {
    scrollTo(contentX(), contentY());
}

void ListViewBase::clearRows() {
    for (LiveRow& row : m_rows)
        retireRow(row);
    m_rows.clear();
    m_first = 0;
}

void ListViewBase::retireRow(LiveRow& row) {
    removeChild(row.widget);
    row.widget = nullptr;
    if (row.state)
        m_retiredStates.push_back(std::move(row.state));
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

void ListViewBase::attachModel(ListModelBase& model) {
    detachModel();
    m_model = &model;
    m_modelSubscription = model.onChange([this](const ListChange& change) { modelChanged(change); });
    m_delegate = nullptr;
    if (built())
        setCurrentIndexQuietly(-1);
    if (built())
        reload();
}

void ListViewBase::reportModelMismatch() const {
    if (m_model)
        spdlog::error("ListView: the delegate's item type does not match the model");
    else
        spdlog::error("ListView: call model() before delegate()");
}

void ListViewBase::detachModel() {
    if (m_model)
        m_model->removeOnChange(m_modelSubscription);
    m_model = nullptr;
    m_modelSubscription = 0;
}

void ListViewBase::modelChanged(const ListChange& change) {
    using Kind = ListChange::Kind;

    if (change.kind == Kind::Destroyed) {
        m_model = nullptr;
        m_modelSubscription = 0;
        m_delegate = nullptr;
        setCurrentIndexQuietly(-1);
        if (built())
            reload();
        return;
    }
    if (change.kind == Kind::Reset) {
        setCurrentIndexQuietly(-1);
        if (built())
            reload();
        return;
    }

    const bool structural = change.kind == Kind::Inserted || change.kind == Kind::Removed || change.kind == Kind::Moved;
    const int current = structural ? Tavoos::indexAfterChange(currentIndex(), change) : currentIndex();
    if (!built() || m_syncing) {
        setCurrentIndexQuietly(current);
        if (m_syncing)
            m_reloadPending = true;
        return;
    }
    if (change.kind == Kind::Changed) {
        refreshRowStates(change.index, change.count);
        return;
    }

    const float top = contentY();
    const bool atTop = top <= 0.0f;
    int anchor = indexAt(top);
    float anchorOffset = trackedCount() > 0 ? top - m_tops[static_cast<std::size_t>(anchor)] : 0.0f;
    const auto heights = m_heights.begin();

    switch (change.kind) {
    case Kind::Inserted:
        m_heights.insert(heights + change.index, static_cast<std::size_t>(change.count), 0.0f);
        insertRows(change.index, change.count);
        if (anchor >= change.index)
            anchor += change.count;
        break;
    case Kind::Removed:
        m_heights.erase(heights + change.index, heights + change.index + change.count);
        removeRows(change.index, change.count);
        if (anchor >= change.index + change.count) {
            anchor -= change.count;
        } else if (anchor >= change.index) {
            anchor = change.index;
            anchorOffset = 0.0f;
        }
        break;
    case Kind::Moved: {
        const float moved = m_heights[static_cast<std::size_t>(change.index)];
        m_heights.erase(heights + change.index);
        m_heights.insert(m_heights.begin() + change.to, moved);
        moveRow(change.index, change.to);
        if (anchor == change.index)
            anchor = change.to;
        else if (change.index < anchor && anchor <= change.to)
            --anchor;
        else if (change.to <= anchor && anchor < change.index)
            ++anchor;
        break;
    }
    default:
        break;
    }

    rebuildOffsets();
    markLayoutDirty();
    setCurrentIndexQuietly(current);
    const int total = trackedCount();
    const float y = atTop || total == 0 ? 0.0f : m_tops[static_cast<std::size_t>(std::clamp(anchor, 0, total - 1))] + anchorOffset;
    m_syncing = true;
    scrollTo(contentX(), y);
    m_syncing = false;
    syncRows();
}

void ListViewBase::insertRows(int index, int count) {
    if (m_rows.empty())
        return;

    const int end = m_first + static_cast<int>(m_rows.size());
    if (index <= m_first) {
        m_first += count;
    } else if (index < end) {
        const auto at = m_rows.begin() + (index - m_first);
        if (count > static_cast<int>(m_rows.size())) {
            for (auto it = at; it != m_rows.end(); ++it)
                retireRow(*it);
            m_rows.erase(at, m_rows.end());
        } else {
            std::size_t position = static_cast<std::size_t>(index - m_first);
            for (int k = 0; k < count; ++k)
                m_rows.insert(m_rows.begin() + static_cast<std::ptrdiff_t>(position++), m_delegate(index + k));
        }
    }
    reindexRows();
}

void ListViewBase::removeRows(int index, int count) {
    if (m_rows.empty())
        return;

    std::deque<LiveRow> kept;
    int first = -1;
    for (std::size_t k = 0; k < m_rows.size(); ++k) {
        const int row = m_first + static_cast<int>(k);
        if (row >= index && row < index + count) {
            retireRow(m_rows[k]);
            continue;
        }
        if (first < 0)
            first = row < index ? row : row - count;
        kept.push_back(std::move(m_rows[k]));
    }
    m_rows = std::move(kept);
    m_first = std::max(first, 0);
    reindexRows();
}

void ListViewBase::moveRow(int from, int to) {
    if (m_rows.empty())
        return;

    LiveRow moving;
    const int end = m_first + static_cast<int>(m_rows.size());
    if (from >= m_first && from < end) {
        const auto at = m_rows.begin() + (from - m_first);
        moving = std::move(*at);
        m_rows.erase(at);
    } else if (from < m_first) {
        --m_first;
    }

    const int last = m_first + static_cast<int>(m_rows.size());
    if (to < m_first) {
        ++m_first;
    } else if (to < last || (to == last && moving.widget)) {
        const auto at = m_rows.begin() + (to - m_first);
        if (moving.widget) {
            m_rows.insert(at, std::move(moving));
            moving = {};
        } else {
            m_rows.insert(at, m_delegate(to));
        }
    }
    if (moving.widget)
        retireRow(moving);
    reindexRows();
}

void ListViewBase::refreshRowStates(int index, int count) {
    for (std::size_t k = 0; k < m_rows.size(); ++k) {
        const int row = m_first + static_cast<int>(k);
        if (row >= index && row < index + count && m_rows[k].state)
            m_rows[k].state->refresh();
    }
}

void ListViewBase::reindexRows() {
    for (std::size_t k = 0; k < m_rows.size(); ++k) {
        if (m_rows[k].state)
            m_rows[k].state->setIndex(m_first + static_cast<int>(k));
    }
}

void ListViewBase::setCurrentIndexQuietly(int index) {
    m_quietIndexChange = true;
    m_currentIndex.set(index);
    m_quietIndexChange = false;
}

void ListViewBase::applyCurrentIndex(int index) {
    if (!built()) {
        m_currentIndexState.setIfChanged(index);
        return;
    }

    const int valid = index >= 0 && index < rowCount() ? index : -1;
    if (valid != index) {
        m_currentIndex.set(valid);
        return;
    }
    if (m_currentIndexState.get() == valid)
        return;
    m_currentIndexState.set(valid);
    updateHighlight(!m_quietIndexChange);
}

Widget* ListViewBase::currentItem() const {
    const int index = currentIndex();
    return index >= 0 ? rowAt(index) : nullptr;
}

void ListViewBase::updateHighlight(bool animate) {
    if (!m_highlight || !m_highlightFollowsCurrent.get() || !built())
        return;

    const int index = currentIndex();
    if (index < 0 || index >= trackedCount()) {
        if (m_highlight->visible())
            m_highlight->visible(false);
        return;
    }

    const float top = m_tops[static_cast<std::size_t>(index)];
    const float height = heightOf(index);
    const bool shown = m_highlight->visible();
    if (!shown)
        m_highlight->visible(true);

    if (animate && shown) {
        const float duration = std::max(0.0f, m_highlightMoveDuration.get());
        m_highlightTop.animateTo(top, duration, Easing::easeOutQuad);
        m_highlightHeight.animateTo(height, duration, Easing::easeOutQuad);
        return;
    }
    if (shown && (m_highlightTop.isAnimating() || m_highlightHeight.isAnimating()))
        return;
    if (m_highlightTop.get() != top)
        m_highlightTop.animateTo(top, 0.0f);
    if (m_highlightHeight.get() != height)
        m_highlightHeight.animateTo(height, 0.0f);
}

void ListViewBase::ensureVisible(int index) {
    if (!built() || index < 0 || index >= trackedCount())
        return;

    const float top = m_tops[static_cast<std::size_t>(index)];
    const float bottom = top + heightOf(index);
    const float viewTop = contentY();
    const float viewHeight = viewportHeight();
    if (top < viewTop)
        scrollTo(contentX(), top);
    else if (bottom > viewTop + viewHeight)
        scrollTo(contentX(), std::min(top, bottom - viewHeight));
}

bool ListViewBase::hasHandlerFor(EventType type) {
    if (type == EventType::KeyPress && keyNavigation())
        return true;
    return ScrollAreaBase::hasHandlerFor(type);
}

void ListViewBase::triggerKeyPress(KeyEvent& event) {
    const int total = rowCount();
    const int current = currentIndex();
    int next = current;
    switch (event.keyCode()) {
    case static_cast<int>(Key::Up):   next = current < 0 ? 0 : current - 1; break;
    case static_cast<int>(Key::Down): next = current < 0 ? 0 : current + 1; break;
    case static_cast<int>(Key::Home): next = 0; break;
    case static_cast<int>(Key::End):  next = total - 1; break;
    default:
        ScrollAreaBase::triggerKeyPress(event);
        return;
    }

    if (total > 0)
        next = std::clamp(next, 0, total - 1);
    if (total == 0 || next == current) {
        event.ignore();
    } else {
        m_currentIndex.set(next);
        ensureVisible(next);
    }
    Widget::triggerKeyPress(event);
}

}
