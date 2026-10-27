#include <tavoos/widget/droparea.h>

#include <algorithm>

namespace Tavoos {

DropAreaWidget::DropAreaWidget(Object* parent) : Widget{parent} {}

bool DropAreaWidget::acceptsKeys(const std::vector<std::string>& keys) const {
    if (m_keys.empty())
        return true;
    return std::ranges::any_of(keys, [this](const std::string& key) { return std::ranges::contains(m_keys, key); });
}

}
