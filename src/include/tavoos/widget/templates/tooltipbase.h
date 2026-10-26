#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/templates/popupbase.h>

#include <string>

namespace Tavoos {

class TAVOOS_EXPORT ToolTipBase : public PopupBase, private AnimatableBase {
public:
    ToolTipBase(Object* parent);
    ~ToolTipBase() override;

    decltype(auto) text(this auto&& self, PropertyArg<std::string> text) {
        self.m_text.set(text);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) delay(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_delay);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) timeout(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_timeout);
        return std::forward<decltype(self)>(self);
    }

    std::string text() const { return m_text; }
    float delay() const { return m_delay; }
    float timeout() const { return m_timeout; }

    void show(const std::string& text, float timeout = -1.0f);
    void hide();

protected:
    State<std::string>& textState() { return m_text.state(); }
    void applyOpened(bool opened) override;

private:
    bool tick(float dt) override;
    void startTicking();
    void showNow();

    BindableState<std::string> m_text;
    Property<float> m_delay{0.0f};
    Property<float> m_timeout{-1.0f};

    float m_elapsed{0.0f};
    bool m_pending{false};
    bool m_ticking{false};
};

}
