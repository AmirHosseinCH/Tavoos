#pragma once

#include <tavoos/export.hpp>
#include <tavoos/widget/widget.h>

#include <utility>

namespace Tavoos {

namespace detail {

template<typename...>
inline constexpr bool kEffectsRejected = false;

template<typename... Args>
void rejectEffect() {
    static_assert(kEffectsRejected<Args...>,
                  "This widget is made of other widgets, so effects are added to those parts instead: "
                  "put the shadow on the widget that draws, for example inside background(...) or on a "
                  "RectangleWidget child.");
}

}

class TAVOOS_EXPORT EffectlessWidget : public Widget {
public:
    using Widget::Widget;

    template<typename... Args>
    decltype(auto) dropShadow(this auto&& self, Args&&...) {
        detail::rejectEffect<Args...>();
        return std::forward<decltype(self)>(self);
    }

    template<typename... Args>
    decltype(auto) innerShadow(this auto&& self, Args&&...) {
        detail::rejectEffect<Args...>();
        return std::forward<decltype(self)>(self);
    }

    template<typename... Args>
    void addDropShadow(Args&&...) {
        detail::rejectEffect<Args...>();
    }

    template<typename... Args>
    void addInnerShadow(Args&&...) {
        detail::rejectEffect<Args...>();
    }
};

}
