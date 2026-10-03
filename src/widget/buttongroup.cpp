#include <tavoos/widget/buttonbase.h>
#include <tavoos/widget/buttongroup.h>

#include <algorithm>

namespace Tavoos {

void ButtonGroup::add(ButtonBase* button) {
    if (std::find(m_buttons.begin(), m_buttons.end(), button) != m_buttons.end())
        return;

    m_buttons.push_back(button);
    button->checkedState().onChange([this, button](const bool& isChecked) {
        if (isChecked)
            m_checked = button;
        else if (m_checked == button)
            m_checked = nullptr;
    });
}

void ButtonGroup::select(ButtonBase* button) {
    for (ButtonBase* other : m_buttons) {
        if (other != button)
            other->checked(false);
    }
    button->checked(true);
}

}
