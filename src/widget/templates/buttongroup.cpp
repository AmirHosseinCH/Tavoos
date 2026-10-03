#include <tavoos/widget/templates/buttonbase.h>
#include <tavoos/widget/templates/buttongroup.h>

#include <algorithm>

namespace Tavoos {

ButtonGroup::~ButtonGroup() {
    for (const Member& member : m_members) {
        member.button->checkedState().removeOnChange(member.callbackId);
        member.button->m_group = nullptr;
    }
}

void ButtonGroup::add(ButtonBase* button) {
    const auto existing = std::find_if(m_members.begin(), m_members.end(),
                                       [button](const Member& member) { return member.button == button; });
    if (existing != m_members.end())
        return;

    const std::size_t id = button->checkedState().onChange([this, button](const bool& isChecked) {
        if (isChecked)
            m_checked = button;
        else if (m_checked == button)
            m_checked = nullptr;
    });
    m_members.push_back({button, id});

    if (button->checked()) {
        for (const Member& member : m_members) {
            if (member.button != button)
                member.button->checked(false);
        }
        m_checked = button;
    }
}

void ButtonGroup::remove(ButtonBase* button) {
    const auto member = std::find_if(m_members.begin(), m_members.end(),
                                     [button](const Member& m) { return m.button == button; });
    if (member == m_members.end())
        return;

    button->checkedState().removeOnChange(member->callbackId);
    button->m_group = nullptr;
    if (m_checked == button)
        m_checked = nullptr;
    m_members.erase(member);
}

void ButtonGroup::select(ButtonBase* button) {
    for (const Member& member : m_members) {
        if (member.button != button)
            member.button->checked(false);
    }
    button->checked(true);
}

}
