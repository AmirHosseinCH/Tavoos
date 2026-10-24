#pragma once

#include <tavoos/export.hpp>
#include <tavoos/model/listmodel.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/scrollareabase.h>

#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT ListViewBase : public ScrollAreaBase {
public:
    ListViewBase(Object* parent);
    ~ListViewBase() override;

    template<typename T>
    decltype(auto) delegate(this auto&& self, std::type_identity_t<std::function<void(T&, int)>> body) {
        self.template installDelegate<T>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename Row, typename Item>
    decltype(auto) delegate(this auto&& self, std::type_identity_t<std::function<void(Row&, State<Item>&, State<int>&)>> body) {
        self.template installModelDelegate<Row, Item>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) model(this auto&& self, ListModelBase& model) {
        self.attachModel(model);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) count(this auto&& self, PropertyArg<int> value) {
        value.applyTo(self.m_count);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) spacing(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_spacing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) cacheBuffer(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_cacheBuffer);
        return std::forward<decltype(self)>(self);
    }

    int count() const { return rowCount(); }
    float spacing() const { return m_spacing; }
    float cacheBuffer() const { return m_cacheBuffer; }

    int firstVisibleIndex() const { return m_firstVisible.get(); }
    int lastVisibleIndex() const { return m_lastVisible.get(); }
    State<int>& firstVisibleIndexState() { return m_firstVisible; }
    State<int>& lastVisibleIndexState() { return m_lastVisible; }

    Widget* rowAt(int index) const;
    void reload();
    void scrollToIndex(int index);

protected:
    void render(Renderer& renderer) override;
    void onBuilt() override;
    Size computeContentSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;
    void onOffsetsSynced(bool offsetsChanged) override;

private:
    struct RowState {
        virtual ~RowState() = default;
        virtual void setIndex(int index) = 0;
        virtual void refresh() = 0;
    };

    template<typename Item>
    struct ModelRowState final : RowState {
        ModelRowState(const ListModel<Item>& model, int index) : model{model}, item{model[index]}, index{index} {}
        void setIndex(int value) override { index.setIfChanged(value); }
        void refresh() override { item.set(model[index.get()]); }

        const ListModel<Item>& model;
        State<Item> item;
        State<int> index;
    };

    struct LiveRow {
        Widget* widget{nullptr};
        std::unique_ptr<RowState> state;
    };

    template<typename T>
    void installDelegate(std::function<void(T&, int)> body) {
        detachModel();
        m_delegate = [this, body = std::move(body)](int index) -> LiveRow {
            Widget* const widget = addChild<T>([&body, index](T& row) {
                row.fill(Fill::Width);
                if (body)
                    runInBuilderScope(row, [&row, &body, index] { body(row, index); });
            });
            return {widget, nullptr};
        };
        if (built())
            reload();
    }

    template<typename Row, typename Item>
    void installModelDelegate(std::function<void(Row&, State<Item>&, State<int>&)> body) {
        auto* const model = dynamic_cast<ListModel<Item>*>(m_model);
        if (!model) {
            reportModelMismatch();
            return;
        }
        m_delegate = [this, model, body = std::move(body)](int index) -> LiveRow {
            auto state = std::make_unique<ModelRowState<Item>>(*model, index);
            ModelRowState<Item>* const states = state.get();
            Widget* const widget = addChild<Row>([&body, states](Row& row) {
                row.fill(Fill::Width);
                if (body)
                    runInBuilderScope(row, [&row, &body, states] { body(row, states->item, states->index); });
            });
            return {widget, std::move(state)};
        };
        if (built())
            reload();
    }

    void attachModel(ListModelBase& model);
    void detachModel();
    void reportModelMismatch() const;
    void modelChanged(const ListChange& change);
    void insertRows(int index, int count);
    void removeRows(int index, int count);
    void moveRow(int from, int to);
    void refreshRowStates(int index, int count);
    void reindexRows();
    void retireRow(LiveRow& row);

    static void runInBuilderScope(Widget& row, const std::function<void()>& call);
    int rowCount() const;
    int trackedCount() const;
    float gap() const;
    float heightOf(int index) const;
    int indexAt(float y) const;
    float measure(Widget& row);
    bool measureRows();
    void rebuildOffsets();
    void syncRows();
    bool syncPass();
    void updateVisible();
    void applyCount();
    void refreshRows();
    void clearRows();
    void removeRowsBeyond(int total);
    int rowIndexOf(const Widget* child) const;

    Property<int> m_count{0};
    Property<float> m_spacing{0.0f};
    Property<float> m_cacheBuffer{200.0f};
    State<int> m_firstVisible{-1};
    State<int> m_lastVisible{-1};

    std::function<LiveRow(int)> m_delegate;
    std::deque<LiveRow> m_rows;
    std::vector<std::unique_ptr<RowState>> m_retiredStates;
    ListModelBase* m_model{nullptr};
    std::size_t m_modelSubscription{0};
    int m_first{0};
    int m_pendingIndex{-1};
    int m_target{-1};

    std::vector<float> m_heights;
    std::vector<float> m_tops{0.0f};
    float m_estimate{40.0f};
    bool m_syncing{false};
    bool m_reloadPending{false};
};

}
