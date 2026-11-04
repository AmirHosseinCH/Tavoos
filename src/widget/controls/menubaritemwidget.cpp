#include <tavoos/widget/controls/menubaritemwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/controls/tooltipwidget.h>
#include <tavoos/widget/templates/menubarbase.h>
#include <tavoos/widget/templates/menubase.h>

namespace Tavoos {

namespace {

constexpr float kPaddingX = 10.0f;
constexpr float kPaddingY = 5.0f;

}

MenuBarItemWidget::MenuBarItemWidget(Object* parent) : MenuBarItemBase{parent} {
    m_highlightColor.onChange([this](const Paint&) { updateColors(); });
    m_hoverColor.onChange([this](const Paint&) { updateColors(); });
    m_textColor.onChange([this](const Paint&) { updateColors(); });
    m_disabledTextColor.onChange([this](const Paint&) { updateColors(); });
    hoveredState().onChange([this](const bool&) { updateColors(); });
    enabledState().onChange([this](const bool&) { updateColors(); });
    m_font.onChange([this](const Font&) { requestRelayout(); });

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundOut);
    });

    content<TextWidget>([this](TextWidget& label) {
        m_label = &label;
        label.font(m_font.state()).color(m_textOut).alignment(Alignment::CenterVertical)
            .marginLeft(kPaddingX).marginRight(kPaddingX).marginTop(kPaddingY).marginBottom(kPaddingY);
    });

    updateColors();

    m_style.onChange([this](const MenuBarItemStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().menuBarItem);
}

void MenuBarItemWidget::bindMenu(MenuBase& menu) {
    m_label->text(menu.titleState());
    menu.openedState().onChange([this](const bool&) { updateColors(); });
    if (const auto* const owner = ancestor<MenuBarBase>())
        menu.offsetBottom(owner->paddingBottom());
    requestRelayout();
    updateColors();
}

void MenuBarItemWidget::updateColors() {
    const bool open = menu() && menu()->openedState().get();
    m_backgroundOut.set(!enabled() ? Paint{Color::Transparent}
                        : open ? m_highlightColor.get()
                        : hovered() ? m_hoverColor.get()
                        : Paint{Color::Transparent});
    m_textOut.set(enabled() ? m_textColor.get() : m_disabledTextColor.get());
}

void MenuBarItemWidget::applyStyle(const MenuBarItemStyle& value) {
    highlightColor(value.highlightColor);
    hoverColor(value.hoverColor);
    textColor(value.textColor);
    disabledTextColor(value.disabledTextColor);
    font(value.font);
    radius(value.radius);
}

void MenuBarItemWidget::createToolTip() {
    toolTip<ToolTipWidget>({});
}

}
