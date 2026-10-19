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
                  "put the effect on the widget that draws, for example inside background(...) or on a "
                  "RectangleWidget child.");
}

}

class TAVOOS_EXPORT EffectlessWidget : public Widget {
public:
    using Widget::Widget;

    template<typename Self>
    decltype(auto) dropShadow(this Self&& self, const ShadowStyle& = {}) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) dropShadow(this Self&& self, State<ShadowStyle>&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) innerShadow(this Self&& self, const ShadowStyle& = {}) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) innerShadow(this Self&& self, State<ShadowStyle>&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) backgroundBlur(this Self&& self, const BlurStyle&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) backgroundBlur(this Self&& self, State<BlurStyle>&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) backgroundBlur(this Self&& self, float) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) glass(this Self&& self, const GlassStyle&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) glass(this Self&& self, State<GlassStyle>&) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    decltype(auto) clearGlass(this Self&& self) {
        detail::rejectEffect<Self>();
        return std::forward<Self>(self);
    }

    template<typename Self>
    void addDropShadow(this Self&&, const ShadowStyle& = {}) {
        detail::rejectEffect<Self>();
    }

    template<typename Self>
    void addDropShadow(this Self&&, State<ShadowStyle>&) {
        detail::rejectEffect<Self>();
    }

    template<typename Self>
    void addInnerShadow(this Self&&, const ShadowStyle& = {}) {
        detail::rejectEffect<Self>();
    }

    template<typename Self>
    void addInnerShadow(this Self&&, State<ShadowStyle>&) {
        detail::rejectEffect<Self>();
    }
};

}
