#include <tavoos/widget/controls/menuitemwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/controls/tooltipwidget.h>
#include <tavoos/widget/row.h>
#include <tavoos/widget/svg.h>
#include <tavoos/widget/text.h>

namespace Tavoos {

namespace {

constexpr const char* kCheckSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.5 L9.5 17 L19 6.5" /></svg>)svg";
constexpr int kCheckSize = 14;
constexpr float kPaddingX = 10.0f;
constexpr float kPaddingY = 7.0f;
constexpr float kSpacing = 8.0f;
constexpr float kShortcutGap = 24.0f;

}

MenuItemWidget::MenuItemWidget(Object* parent) : MenuItemBase{parent} {
    m_highlightColor.onChange([this](const Paint&) { updateColors(); });
    m_textColor.onChange([this](const Paint&) { updateColors(); });
    m_disabledTextColor.onChange([this](const Paint&) { updateColors(); });
    m_shortcutColor.onChange([this](const Paint&) { updateColors(); });
    m_checkColor.onChange([this](const Paint&) { updateColors(); });
    highlightedState().onChange([this](const bool&) { updateColors(); });
    enabledState().onChange([this](const bool&) { updateColors(); });
    checkedState().onChange([this](const bool&) { updateColors(); });

    m_text.onChange([this](const std::string&) { requestRelayout(); });
    m_font.onChange([this](const Font&) { requestRelayout(); });
    checkableState().onChange([this](const bool&) { requestRelayout(); });
    shortcutState().onChange([this](const KeySequence& sequence) {
        m_shortcutText.set(sequence.toString());
        m_hasShortcut.setIfChanged(!sequence.isEmpty());
        requestRelayout();
    });

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundOut);
    });

    content<RowWidget>([this](RowWidget& row) {
        row.spacing(kSpacing).fill(Fill::Width).alignment(Alignment::CenterVertical)
            .marginLeft(kPaddingX).marginRight(kPaddingX).marginTop(kPaddingY).marginBottom(kPaddingY);
        row.addChild<SVGWidget>([this](SVGWidget& check) {
            check.source(std::string("data:") + kCheckSvg).width(kCheckSize).height(kCheckSize)
                .alignment(Alignment::CenterVertical).color(m_checkOut).visible(checkableState());
        });
        row.addChild<TextWidget>([this](TextWidget& label) {
            label.text(m_text.state()).font(m_font.state()).color(m_textOut)
                .fill(Fill::Width).alignment(Alignment::CenterVertical);
        });
        row.addChild<TextWidget>([this](TextWidget& keys) {
            keys.text(m_shortcutText).font(m_font.state()).color(m_shortcutOut)
                .marginLeft(kShortcutGap).alignment(Alignment::CenterVertical).visible(m_hasShortcut);
        });
    });

    updateColors();

    m_style.onChange([this](const MenuItemStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().menuItem);
}

void MenuItemWidget::updateColors() {
    m_backgroundOut.set(highlighted() && enabled() ? m_highlightColor.get() : Paint{Color::Transparent});
    m_textOut.set(enabled() ? m_textColor.get() : m_disabledTextColor.get());
    m_shortcutOut.set(enabled() ? m_shortcutColor.get() : m_disabledTextColor.get());
    const Paint& check = enabled() ? m_checkColor.get() : m_disabledTextColor.get();
    m_checkOut.set(checked() ? check : Paint{Color::Transparent});
}

void MenuItemWidget::applyStyle(const MenuItemStyle& value) {
    highlightColor(value.highlightColor);
    textColor(value.textColor);
    disabledTextColor(value.disabledTextColor);
    shortcutColor(value.shortcutColor);
    checkColor(value.checkColor);
    font(value.font);
    radius(value.radius);
}

void MenuItemWidget::createToolTip() {
    toolTip<ToolTipWidget>({});
}

}
