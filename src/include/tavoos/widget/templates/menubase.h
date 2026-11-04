#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/popupbase.h>

#include <vector>

namespace Tavoos {

class MenuItemBase;

class TAVOOS_EXPORT MenuBase : public PopupBase, private AnimatableBase {
    friend class MenuItemBase;

public:
    MenuBase(Object* parent);
    ~MenuBase() override;

    decltype(auto) subMenuDelay(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_subMenuDelay);
        return std::forward<decltype(self)>(self);
    }

    float subMenuDelay() const { return m_subMenuDelay; }

    void popup();
    void popup(const Point& point);

protected:
    void applyOpened(bool opened) override;
    bool hasHandlerFor(EventType type) override;
    void triggerKeyPress(KeyEvent& event) override;
    void triggerShortcutOverride(KeyEvent& event) override;
    Size computeIntrinsicSize() override;
    ContentArea contentAreaFor(const Widget& child) const override;

private:
    bool tick(float dt) override;
    bool handlesKey(const KeyEvent& event) const;
    std::vector<Widget*> stackedChildren() const;
    std::vector<MenuItemBase*> items(bool navigableOnly) const;
    MenuItemBase* highlightedItem() const;
    void highlight(MenuItemBase* item, bool byHover = false);
    void focusHighlighted();
    void step(int delta);
    void closeSubMenus(const MenuItemBase* except);
    void startSubMenuTimer(MenuItemBase* item);
    void stopSubMenuTimer();

    Property<float> m_subMenuDelay{0.2f};
    MenuItemBase* m_pendingItem{nullptr};
    float m_pendingElapsed{0.0f};
    bool m_ticking{false};
};

}
