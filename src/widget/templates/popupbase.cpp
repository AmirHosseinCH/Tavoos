#include <tavoos/widget/templates/popupbase.h>

#include <tavoos/window.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

namespace {

struct Rect { float left, top, right, bottom; };

Placement opposite(Placement side) {
    switch (side) {
    case Placement::Bottom: return Placement::Top;
    case Placement::Top:    return Placement::Bottom;
    case Placement::Left:   return Placement::Right;
    case Placement::Right:  return Placement::Left;
    default:                return side;
    }
}

}

PopupBase::PopupBase(Object* parent) : OverlayBase{parent} {
    m_requestedX.onChange([this](const int&) { m_hasRequestedX = true; });
    m_requestedY.onChange([this](const int&) { m_hasRequestedY = true; });
    m_placement.onChange([this](const Placement&) { m_hasRequestedX = m_hasRequestedY = false; });
}

void PopupBase::place() {
    Window* const window = ownerWindow();
    if (!window)
        return;

    const Size size = intrinsicSize();
    const float windowW = static_cast<float>(window->width());
    const float windowH = static_cast<float>(window->height());

    auto* const anchor = dynamic_cast<Widget*>(parent());
    const bool toWindow = m_target.get() == PlacementTarget::Window || !anchor;

    Rect ref{0.0f, 0.0f, windowW, windowH};
    if (!toWindow) {
        const Point a = anchor->mapToWindow({0.0f, 0.0f});
        const Point b = anchor->mapToWindow({anchor->displayedWidth(), anchor->displayedHeight()});
        ref = {std::min(a.x, b.x), std::min(a.y, b.y), std::max(a.x, b.x), std::max(a.y, b.y)};
    }

    float x = (ref.left + ref.right - size.width) * 0.5f;
    float y = (ref.top + ref.bottom - size.height) * 0.5f;
    Placement side = m_placement.get();

    if (toWindow) {
        switch (side) {
        case Placement::Bottom: y = ref.bottom - m_offset.bottom() - size.height; break;
        case Placement::Top:    y = ref.top + m_offset.top(); break;
        case Placement::Left:   x = ref.left + m_offset.left(); break;
        case Placement::Right:  x = ref.right - m_offset.right() - size.width; break;
        case Placement::Center: break;
        }
    } else if (side != Placement::Center) {
        auto space = [&](Placement s) {
            switch (s) {
            case Placement::Bottom: return windowH - ref.bottom - m_offset.bottom();
            case Placement::Top:    return ref.top - m_offset.top();
            case Placement::Left:   return ref.left - m_offset.left();
            default:                return windowW - ref.right - m_offset.right();
            }
        };
        const bool vertical = side == Placement::Bottom || side == Placement::Top;
        const float needed = vertical ? size.height : size.width;
        if (space(side) < needed && space(opposite(side)) > space(side))
            side = opposite(side);

        switch (side) {
        case Placement::Bottom: x = ref.left; y = ref.bottom + m_offset.bottom(); break;
        case Placement::Top:    x = ref.left; y = ref.top - m_offset.top() - size.height; break;
        case Placement::Left:   x = ref.left - m_offset.left() - size.width; y = ref.top; break;
        default:                x = ref.right + m_offset.right(); y = ref.top; break;
        }
    }

    x = m_hasRequestedX ? ref.left + static_cast<float>(m_requestedX.get())
                        : std::clamp(x, 0.0f, std::max(0.0f, windowW - size.width));
    y = m_hasRequestedY ? ref.top + static_cast<float>(m_requestedY.get())
                        : std::clamp(y, 0.0f, std::max(0.0f, windowH - size.height));

    const int placedX = static_cast<int>(std::lround(x));
    const int placedY = static_cast<int>(std::lround(y));
    if (Widget::x() != placedX)
        Widget::x(placedX);
    if (Widget::y() != placedY)
        Widget::y(placedY);
}

}
