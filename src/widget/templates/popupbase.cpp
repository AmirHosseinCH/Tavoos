#include <tavoos/widget/templates/popupbase.h>

#include <tavoos/window.h>

#include <algorithm>

namespace Tavoos {

PopupBase::PopupBase(Object* parent) : Control{parent} {
    visible(false);
    m_opened.onChange([this](const bool& opened) { applyOpened(opened); });
}

void PopupBase::open() {
    m_opened.set(true);
}

void PopupBase::close() {
    m_opened.set(false);
}

void PopupBase::applyOpened(bool opened) {
    if (opened == m_openedState.get())
        return;
    m_openedState.set(opened);
    visible(opened);

    if (Window* const window = ownerWindow()) {
        if (opened)
            window->addOverlay(this);
        else
            window->removeOverlay(this);
    }

    if (opened && m_onOpen)
        m_onOpen();
    else if (!opened && m_onClose)
        m_onClose();
}

bool PopupBase::hasHandlerFor(EventType type) {
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

Widget::ContentArea PopupBase::contentAreaFor(const Widget& child) const {
    if (&child == backgroundSlot())
        return { 0.0f, 0.0f, displayedWidth(), displayedHeight() };
    return Control::contentAreaFor(child);
}

Widget::Size PopupBase::computeIntrinsicSize() {
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
