#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/controls/style/stackviewstyle.h>
#include <tavoos/widget/templates/stackviewbase.h>

namespace Tavoos {

class TAVOOS_EXPORT StackViewWidget : public StackViewBase {
public:
    StackViewWidget(Object* parent);

    decltype(auto) style(this auto&& self, const StackViewStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<StackViewStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

private:
    void applyStyle(const StackViewStyle& style);

    BindableState<StackViewStyle> m_style;
};

}
