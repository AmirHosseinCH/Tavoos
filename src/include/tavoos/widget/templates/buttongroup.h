#pragma once

#include <tavoos/export.hpp>

#include <cstddef>
#include <vector>

namespace Tavoos {

class ButtonBase;

class TAVOOS_EXPORT ButtonGroup {
public:
    ButtonGroup() = default;
    ButtonGroup(const ButtonGroup&) = delete;
    ButtonGroup& operator=(const ButtonGroup&) = delete;
    ~ButtonGroup();

    void add(ButtonBase* button);
    void remove(ButtonBase* button);
    void select(ButtonBase* button);
    ButtonBase* checked() const { return m_checked; }

private:
    struct Member {
        ButtonBase* button;
        std::size_t callbackId;
    };

    std::vector<Member> m_members;
    ButtonBase* m_checked{nullptr};
};

}
