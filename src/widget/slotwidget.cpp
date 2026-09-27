#include <tavoos/widget/slotwidget.h>

namespace Tavoos {

SlotWidget::SlotWidget(Object* parent) : Widget{parent} {
}

void SlotWidget::render(Renderer& renderer) {
    renderChildren(renderer);
}

Widget::Size SlotWidget::computeIntrinsicSize() {
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    if (m_content) {
        const Size size = m_content->intrinsicSize();
        contentWidth = size.width + m_content->marginLeft() + m_content->marginRight();
        contentHeight = size.height + m_content->marginTop() + m_content->marginBottom();
    }
    return { (width() > 0) ? static_cast<float>(width()) : contentWidth,
             (height() > 0) ? static_cast<float>(height()) : contentHeight };
}

void SlotWidget::onSlotReplaced() {
}

void SlotWidget::replaceSlot(Widget*& slot) {
    removeChild(slot);
    slot = nullptr;
    onSlotReplaced();
}

}
