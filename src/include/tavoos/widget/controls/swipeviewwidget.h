#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/controls/style/swipeviewstyle.h>
#include <tavoos/widget/templates/swipeviewbase.h>

namespace Tavoos {

class TAVOOS_EXPORT SwipeViewWidget : public SwipeViewBase {
public:
    SwipeViewWidget(Object* parent);

    decltype(auto) style(this auto&& self, const SwipeViewStyle& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) style(this auto&& self, State<SwipeViewStyle>& style) {
        self.m_style.set(style);
        return std::forward<decltype(self)>(self);
    }

private:
    void applyStyle(const SwipeViewStyle& style);

    BindableState<SwipeViewStyle> m_style;
};

}
