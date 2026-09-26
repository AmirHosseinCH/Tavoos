#include <tavoos/widget/radiogroup.h>
#include <tavoos/widget/radiowidget.h>

#include <algorithm>

namespace Tavoos {

void RadioGroup::add(RadioWidget* button) {
    if (std::find(m_buttons.begin(), m_buttons.end(), button) != m_buttons.end())
        return;

    m_buttons.push_back(button);
    button->selectedState().onChange([this, button](const bool& isSelected) {
        if (isSelected)
            m_selected = button;
        else if (m_selected == button)
            m_selected = nullptr;
    });
}

void RadioGroup::select(RadioWidget* button) {
    for (RadioWidget* other : m_buttons) {
        if (other != button)
            other->selected(false);
    }
    button->selected(true);
}

}
