#pragma once

#include <tavoos/export.hpp>
#include <tavoos/model/listmodel.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/column.h>
#include <tavoos/widget/rectangle.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/listviewbase.h>
#include <tavoos/widget/templates/popupbase.h>
#include <tavoos/widget/text.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>

namespace Tavoos {

class TAVOOS_EXPORT ComboBoxBase : public ButtonBase {
public:
    ComboBoxBase(Object* parent);
    ~ComboBoxBase() override;

    template<typename W = PopupBase>
        requires std::derived_from<W, PopupBase>
    decltype(auto) popup(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installPopup<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = ListViewBase>
        requires std::derived_from<W, ListViewBase>
    decltype(auto) list(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installList<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) indicator(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installIndicator<W>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) model(this auto&& self, ListModelBase& model) {
        self.attachModel(model);
        return std::forward<decltype(self)>(self);
    }

    template<typename Item>
    decltype(auto) text(this auto&& self, std::type_identity_t<std::function<std::string(const Item&)>> fn) {
        self.template installText<Item>(std::move(fn));
        return std::forward<decltype(self)>(self);
    }

    template<typename Row, typename Item>
    decltype(auto) delegate(this auto&& self, std::type_identity_t<std::function<void(Row&, State<Item>&, State<int>&)>> body) {
        self.template installDelegate<Row, Item>(std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) currentIndex(this auto&& self, PropertyArg<int> index) {
        index.applyTo(self.m_currentIndex);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) placeholder(this auto&& self, PropertyArg<std::string> text) {
        text.applyTo(self.m_placeholder);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) popupMaxHeight(this auto&& self, PropertyArg<float> height) {
        height.applyTo(self.m_popupMaxHeight);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onActivated(this auto&& self, std::function<void(int)> callback) {
        self.m_onActivated = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    int currentIndex() const { return m_currentIndexState.get(); }
    std::string currentText() const;
    std::string placeholder() const { return m_placeholder; }
    float popupMaxHeight() const { return m_popupMaxHeight; }
    const std::string& displayText() const { return m_displayText.get(); }
    bool popupOpened() const;

    State<int>& currentIndexState() { return m_currentIndexState; }
    State<std::string>& displayTextState() { return m_displayText; }
    State<bool>& popupOpenedState() { return m_popupOpened; }

    PopupBase* popupItem() const { return m_popup; }
    ListViewBase* listItem() const { return m_list; }
    Widget* indicatorItem() const { return m_indicator; }

    void openPopup();
    void closePopup();

protected:
    void onBuilt() override;
    void handleClick(MouseEvent& event) override;
    void triggerKeyPress(KeyEvent& event) override;

private:
    template<typename W>
    void installPopup(std::function<void(W&)> body) {
        if (m_popup) {
            removeChild(m_popup);
            m_popup = nullptr;
            m_list = nullptr;
        }
        m_popup = addChild<W>([this, &body](W& slot) {
            slot.placement(Placement::Bottom).closePolicy(ClosePolicy::ClickOutsideParent | ClosePolicy::Escape);
            if (body)
                runInBuilderScope(slot, [&slot, &body] { body(slot); });
            slot.onClose([this] { popupClosed(); });
        });
        wirePopup();
    }

    template<typename W>
    void installList(std::function<void(W&)> body) {
        if (!m_popup) {
            reportMissingPopup();
            return;
        }
        if (m_list) {
            m_popup->removeChild(m_list);
            m_list = nullptr;
        }
        m_list = m_popup->addChild<W>([&body](W& slot) {
            slot.fill(Fill::Width).keyNavigation(true).focusable(true);
            if (body)
                runInBuilderScope(slot, [&slot, &body] { body(slot); });
        });
        wireList();
    }

    template<typename W>
    void installIndicator(std::function<void(W&)> body) {
        if (m_indicator) {
            removeChild(m_indicator);
            m_indicator = nullptr;
        }
        m_indicator = addChild<W>([&body](W& slot) {
            slot.alignment(Alignment::Right | Alignment::CenterVertical);
            if (body)
                runInBuilderScope(slot, [&slot, &body] { body(slot); });
        });
    }

    template<typename Item>
    void installText(std::function<std::string(const Item&)> fn) {
        auto* const model = dynamic_cast<ListModel<Item>*>(m_model);
        if (!model) {
            reportModelMismatch();
            return;
        }
        m_textOf = [model, fn](int index) { return fn((*model)[index]); };
        if (!m_customRows) {
            m_installRows = [this, fn] {
                m_list->delegate<ColumnWidget, Item>([this, fn](ColumnWidget& row, State<Item>& item, State<int>& index) {
                    wireRow(row, index);
                    row.addChild<TextWidget>([&item, &fn](TextWidget& label) {
                        label.text(fn(item.get())).marginLeft(10).marginRight(10).marginTop(8).marginBottom(8);
                        item.onChange([&label, fn](const Item& value) { label.text(fn(value)); });
                    });
                });
            };
            if (m_list)
                m_installRows();
        }
        refreshDisplay();
    }

    template<typename Row, typename Item>
    void installDelegate(std::function<void(Row&, State<Item>&, State<int>&)> body) {
        if (!dynamic_cast<ListModel<Item>*>(m_model)) {
            reportModelMismatch();
            return;
        }
        m_customRows = true;
        m_installRows = [this, body] {
            m_list->delegate<ColumnWidget, Item>([this, body](ColumnWidget& row, State<Item>& item, State<int>& index) {
                wireRow(row, index);
                row.addChild<Row>([&body, &item, &index](Row& content) {
                    content.fill(Fill::Width);
                    if (body)
                        runInBuilderScope(content, [&] { body(content, item, index); });
                });
            });
        };
        if (m_list)
            m_installRows();
    }

    static void runInBuilderScope(Widget& slot, const std::function<void()>& call);
    void wirePopup();
    void wireList();
    void wireRow(ColumnWidget& row, State<int>& index);
    void attachModel(ListModelBase& model);
    void detachModel();
    void modelChanged(const ListChange& change);
    void reportModelMismatch() const;
    void reportMissingPopup() const;
    int validIndex(int index) const;
    void applyCurrentIndex(int index);
    void setChosen(int index);
    void syncFromList();
    void refreshDisplay();
    void updateListHeight();
    void pick(int index);
    void popupClosed();
    void stepChosen(int delta);

    ListModelBase* m_model{nullptr};
    std::size_t m_modelSubscription{0};
    std::function<std::string(int)> m_textOf;
    std::function<void()> m_installRows;
    std::function<void(int)> m_onActivated;
    bool m_customRows{false};

    PopupBase* m_popup{nullptr};
    ListViewBase* m_list{nullptr};
    Widget* m_indicator{nullptr};

    Property<int> m_currentIndex{-1};
    State<int> m_currentIndexState{-1};
    Property<std::string> m_placeholder{""};
    Property<float> m_popupMaxHeight{300.0f};
    State<std::string> m_displayText{""};
    State<bool> m_popupOpened{false};

    int m_remembered{-1};
    bool m_picking{false};
    bool m_applying{false};
};

}
