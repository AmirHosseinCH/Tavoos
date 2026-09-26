#pragma once

#include <tavoos/export.hpp>

#include <vector>

namespace Tavoos {

class RadioWidget;

class TAVOOS_EXPORT RadioGroup {
public:
    void add(RadioWidget* button);
    void select(RadioWidget* button);
    RadioWidget* selected() const { return m_selected; }

private:
    std::vector<RadioWidget*> m_buttons;
    RadioWidget* m_selected{nullptr};
};

}
