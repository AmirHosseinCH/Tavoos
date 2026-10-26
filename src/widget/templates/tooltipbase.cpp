#include <tavoos/widget/templates/tooltipbase.h>

namespace Tavoos {

ToolTipBase::ToolTipBase(Object* parent) : PopupBase{parent} {
    placement(Placement::Top).offset(6.0f).closePolicy(ClosePolicy::None);
    setCenteredOnAnchor(true);
}

ToolTipBase::~ToolTipBase() {
    if (m_ticking)
        AnimationManager::instance().unregisterAnimation(this);
}

void ToolTipBase::show(const std::string& text, float timeout) {
    m_text.set(text);
    if (timeout >= 0.0f)
        m_timeout.set(timeout);
    applyOpened(true);
}

void ToolTipBase::hide() {
    m_pending = false;
    PopupBase::applyOpened(false);
}

void ToolTipBase::applyOpened(bool opened) {
    m_elapsed = 0.0f;
    if (!opened) {
        hide();
        return;
    }
    if (m_delay.get() > 0.0f && !openedState().get()) {
        m_pending = true;
        startTicking();
        return;
    }
    showNow();
}

void ToolTipBase::showNow() {
    m_pending = false;
    m_elapsed = 0.0f;
    PopupBase::applyOpened(true);
    if (m_timeout.get() > 0.0f)
        startTicking();
}

void ToolTipBase::startTicking() {
    if (m_ticking)
        return;
    m_ticking = true;
    AnimationManager::instance().registerAnimation(this);
}

bool ToolTipBase::tick(float dt) {
    m_elapsed += dt;
    if (m_pending && m_elapsed >= m_delay.get())
        showNow();
    else if (!m_pending && openedState().get() && m_timeout.get() > 0.0f && m_elapsed >= m_timeout.get())
        hide();

    const bool waiting = m_pending || (openedState().get() && m_timeout.get() > 0.0f);
    if (!waiting)
        m_ticking = false;
    return waiting;
}

}
