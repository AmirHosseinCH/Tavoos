#pragma once

#include <tavoos/export.hpp>

#include <vector>

namespace Tavoos {

class ButtonBase;

class TAVOOS_EXPORT ButtonGroup {
public:
    void add(ButtonBase* button);
    void select(ButtonBase* button);
    ButtonBase* checked() const { return m_checked; }

private:
    std::vector<ButtonBase*> m_buttons;
    ButtonBase* m_checked{nullptr};
};

}
