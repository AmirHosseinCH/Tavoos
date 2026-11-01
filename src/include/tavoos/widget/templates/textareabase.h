#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/text/fontenums.h>
#include <tavoos/widget/templates/texteditbase.h>

#include <utility>

namespace Tavoos {

class TAVOOS_EXPORT TextAreaBase : public TextEditBase {
public:
    TextAreaBase(Object* parent);

    decltype(auto) wrapMode(this auto&& self, PropertyArg<WrapMode> mode) {
        mode.applyTo(self.m_wrapMode);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) tabStopDistance(this auto&& self, PropertyArg<float> distance) {
        distance.applyTo(self.m_tabStopDistance);
        return std::forward<decltype(self)>(self);
    }

    WrapMode wrapMode() const { return m_wrapMode; }
    float tabStopDistance() const { return m_tabStopDistance; }

protected:
    Size computeIntrinsicSize() override;
    void onResolvedSizeChanged() override;
    void onDisplayChanged() override;
    void onDisplayResized() override;

private:
    void applyWrapWidth();

    Property<WrapMode> m_wrapMode{WrapMode::NoWrap};
    Property<float> m_tabStopDistance{80.0f};
};

}
