#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/widget/templates/control.h>

#include <vector>

namespace Tavoos {

class MenuBarItemBase;

class TAVOOS_EXPORT MenuBarBase : public Control {
    friend class MenuBarItemBase;

public:
    MenuBarBase(Object* parent);

protected:
    virtual MenuBarItemBase* createItem();
    void onBuilt() override;
    bool hasHandlerFor(EventType type) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerShortcutOverride(KeyEvent& event) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;

private:
    using Control::content;

    std::vector<Widget*> stackedChildren() const;
    std::vector<MenuBarItemBase*> items() const;
    MenuBarItemBase* openItem() const;
    MenuBarItemBase* focusedItem() const;
    MenuBarItemBase* neighbor(MenuBarItemBase* item, int delta) const;
    bool handlesKey(const KeyEvent& event) const;
    void openMenu(MenuBarItemBase* item);
    void hoverItem(MenuBarItemBase* item);
};

}
