#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/widget/templates/popupbase.h>

#include <vector>

namespace Tavoos {

class MenuItemBase;

class TAVOOS_EXPORT MenuBase : public PopupBase {
    friend class MenuItemBase;

public:
    MenuBase(Object* parent);

protected:
    void applyOpened(bool opened) override;
    bool hasHandlerFor(EventType type) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerShortcutOverride(KeyEvent& event) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;

private:
    std::vector<Widget*> stackedChildren() const;
    std::vector<MenuItemBase*> items(bool navigableOnly) const;
    MenuItemBase* highlightedItem() const;
    void highlight(MenuItemBase* item);
    void step(int delta);
};

}
