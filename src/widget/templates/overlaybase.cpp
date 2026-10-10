#include <tavoos/widget/templates/overlaybase.h>

#include <tavoos/window.h>

#include <algorithm>
#include <vector>

namespace Tavoos {

OverlayBase::OverlayBase(Object* parent) : Control{parent} {
    visible(false);
    installScrim<RectangleWidget>([](RectangleWidget& scrim) { scrim.color(Color::Transparent); });
    m_opened.onChange([this](const bool& opened) { applyOpened(opened); });
}

void OverlayBase::open() {
    m_opened.set(true);
}

void OverlayBase::close() {
    m_opened.set(false);
}

void OverlayBase::applyOpened(bool opened) {
    if (opened == m_openedState.get())
        return;
    m_runner.finishNow();
    m_openedState.set(opened);

    m_modalActive = opened && m_modal.get();
    Window* const window = ownerWindow();

    if (opened) {
        visible(true);
        if (m_scrim)
            m_scrim->visible(m_modalActive);
        if (window)
            window->addOverlay(this);
        if (window && m_enter)
            runTransition(m_enter, nullptr);
    } else if (window && m_exit) {
        window->retireOverlay(this);
        runTransition(m_exit, [this] { finishHiding(); });
    } else {
        finishHiding();
        if (window)
            window->removeOverlay(this);
    }

    if (opened && m_onOpen)
        m_onOpen();
    else if (!opened && m_onClose)
        m_onClose();
}

void OverlayBase::runTransition(TransitionFactory factory, std::function<void()> onFinished) {
    Window* const window = ownerWindow();
    std::vector<TransitionRunner::Entry> entries;
    entries.push_back({this, factory()});
    m_runner.start(std::move(entries), static_cast<float>(window->width()), static_cast<float>(window->height()),
                   nullptr, std::move(onFinished));
}

void OverlayBase::finishHiding() {
    visible(false);
    if (m_scrim)
        m_scrim->visible(false);
    if (Window* const window = ownerWindow())
        window->releaseOverlay(this);
}

bool OverlayBase::isContentHit(const Widget* hit) const {
    for (const Widget* w = hit; w && w != this; w = dynamic_cast<const Widget*>(w->parent())) {
        if (w == m_scrim)
            return false;
    }
    return hit != nullptr;
}

bool OverlayBase::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MousePress:
    case EventType::MouseRelease:
    case EventType::MouseClick:
    case EventType::MouseDoubleClick:
    case EventType::MouseMove:
    case EventType::DragStart:
    case EventType::DragMove:
    case EventType::DragEnd:
    case EventType::Wheel:
        return true;
    default:
        return Control::hasHandlerFor(type);
    }
}

Widget::ContentArea OverlayBase::contentAreaFor(const Widget& child) const {
    if (&child == backgroundSlot())
        return { 0.0f, 0.0f, displayedWidth(), displayedHeight() };
    if (&child == m_scrim) {
        Window* const window = ownerWindow();
        return { -resolvedX(), -resolvedY(),
                 window ? static_cast<float>(window->width()) : 0.0f,
                 window ? static_cast<float>(window->height()) : 0.0f };
    }
    return Control::contentAreaFor(child);
}

Widget::Size OverlayBase::computeIntrinsicSize() {
    float extentWidth = 0.0f;
    float extentHeight = 0.0f;
    for (const auto& child : children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || !widget->participatesInLayout())
            continue;

        const Size size = widget->intrinsicSize();
        const float childWidth = hasFlag(widget->fill(), Fill::Width) ? 0.0f : size.width;
        const float childHeight = hasFlag(widget->fill(), Fill::Height) ? 0.0f : size.height;
        extentWidth = std::max(extentWidth, widget->marginLeft() + static_cast<float>(widget->x()) + childWidth + widget->marginRight());
        extentHeight = std::max(extentHeight, widget->marginTop() + static_cast<float>(widget->y()) + childHeight + widget->marginBottom());
    }

    return { (width() > 0) ? static_cast<float>(width()) : extentWidth + paddingLeft() + paddingRight(),
             (height() > 0) ? static_cast<float>(height()) : extentHeight + paddingTop() + paddingBottom() };
}

}
