#include <tavoos/widget/controls/stackviewwidget.h>

#include <tavoos/application.h>

namespace Tavoos {

StackViewWidget::StackViewWidget(Object* parent) : StackViewBase{parent} {
    m_style.onChange([this](const StackViewStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().stackView);
}

void StackViewWidget::applyStyle(const StackViewStyle& value) {
    pushEnter(value.pushEnter);
    pushExit(value.pushExit);
    popEnter(value.popEnter);
    popExit(value.popExit);
    replaceEnter(value.replaceEnter);
    replaceExit(value.replaceExit);
}

}
