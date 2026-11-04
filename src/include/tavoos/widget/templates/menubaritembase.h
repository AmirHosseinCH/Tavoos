#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/menubarbase.h>
#include <tavoos/widget/templates/menubase.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuBarItemBase : public ButtonBase {
    friend class MenuBarBase;

public:
    MenuBarItemBase(Object* parent);

    MenuBase* menu() const { return m_menu; }

protected:
    virtual void bindMenu(MenuBase&) {}
    void handleClick(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;

private:
    MenuBarBase* bar() const { return ancestor<MenuBarBase>(); }

    MenuBase* m_menu{nullptr};
};

}
