#include <tavoos/widget/controls/swipeviewwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

SwipeViewWidget::SwipeViewWidget(Object* parent) : SwipeViewBase{parent} {
    m_style.onChange([this](const SwipeViewStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().swipeView);
}

void SwipeViewWidget::applyStyle(const SwipeViewStyle& value) {
    duration(value.duration);
    easing(value.easing);
}

}
