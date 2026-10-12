#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/scrollareabase.h>

#include <deque>
#include <functional>
#include <type_traits>
#include <vector>

namespace Tavoos {

class TAVOOS_EXPORT ListViewBase : public ScrollAreaBase {
public:
    ListViewBase(Object* parent);

    template<typename T>
    decltype(auto) delegate(this auto&& self, std::type_identity_t<std::function<void(T&, int)>> body) {
        self.template installDelegate<T>(std::move(body));
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

    int count() const { return m_count; }
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
    template<typename T>
    void installDelegate(std::function<void(T&, int)> body) {
        m_delegate = [this, body = std::move(body)](int index) -> Widget* {
            return addChild<T>([&body, index](T& row) {
                row.fill(Fill::Width);
                if (body)
                    body(row, index);
            });
        };
        if (built())
            reload();
    }

    int rowCount() const;
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

    std::function<Widget*(int)> m_delegate;
    std::deque<Widget*> m_rows;
    int m_first{0};
    int m_pendingIndex{-1};
    int m_target{-1};

    std::vector<float> m_heights;
    std::vector<float> m_tops{0.0f};
    float m_estimate{40.0f};
    bool m_syncing{false};
};

}
