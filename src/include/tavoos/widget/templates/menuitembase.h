#pragma once

#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/menubase.h>

namespace Tavoos {

class TAVOOS_EXPORT MenuItemBase : public ButtonBase {
    friend class MenuBase;

public:
    MenuItemBase(Object* parent);

    bool highlighted() const { return m_highlighted.get(); }
    State<bool>& highlightedState() { return m_highlighted; }
    MenuBase* menu() const { return ancestor<MenuBase>(); }

protected:
    void handleClick(MouseEvent& event) override;
    void triggerMouseEnter(MouseEvent& event) override;
    void triggerFocusIn(Event& event) override;

private:
    State<bool> m_highlighted{false};
};

}
