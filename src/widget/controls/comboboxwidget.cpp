#include <tavoos/widget/controls/comboboxwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/controls/listviewwidget.h>
#include <tavoos/widget/controls/popupwidget.h>
#include <tavoos/widget/svg.h>

#include <string>

namespace Tavoos {

namespace {

constexpr const char* kChevronSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.6" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9 L12 15 L18 9" /></svg>)svg";
constexpr int kIndicatorSize = 12;
constexpr float kPadding = 10.0f;
constexpr float kIndicatorGap = 6.0f;
constexpr float kPopupOffset = 4.0f;

}

ComboBoxWidget::ComboBoxWidget(Object* parent) : ComboBoxBase{parent} {
    width(180).height(36);

    m_backgroundColorOut.set(m_backgroundColor.get());
    m_textColorOut.set(m_textColor.get());

    m_backgroundColor.onChange([this](const Paint&) { updateColor(false); });
    m_hoverColor.onChange([this](const Paint&) { updateColor(false); });
    m_pressedColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    hoveredState().onChange([this](const bool&) { updateColor(true); });
    pressedState().onChange([this](const bool&) { updateColor(true); });
    enabledState().onChange([this](const bool&) {
        updateColor(true);
        updateTextColor(true);
    });

    m_textColor.onChange([this](const Paint& color) {
        m_rowTextColor.set(color);
        updateTextColor(false);
    });
    m_placeholderColor.onChange([this](const Paint&) { updateTextColor(false); });
    m_disabledTextColor.onChange([this](const Paint&) { updateTextColor(false); });
    currentIndexState().onChange([this](const int&) { updateTextColor(false); });

    background([this](RectangleWidget& box) {
        box.radius(m_radius.state()).color(m_backgroundColorOut)
            .borderWidth(m_borderWidth.state()).borderColor(m_borderColor.state());
    });

    content<TextWidget>([this](TextWidget& label) {
        label.text(displayTextState()).color(m_textColorOut).font(m_font.state())
            .fill(Fill::Width).elideMode(ElideMode::Right).alignment(Alignment::CenterVertical)
            .marginLeft(kPadding).marginRight(kPadding + kIndicatorSize + kIndicatorGap);
    });

    indicator<SVGWidget>([this](SVGWidget& chevron) {
        chevron.source(std::string("data:") + kChevronSvg).width(kIndicatorSize).height(kIndicatorSize)
            .marginRight(kPadding).color(m_indicatorColor.state());
    });

    popup<PopupWidget>([](PopupWidget& popup) { popup.offset(kPopupOffset); });

    list<ListViewWidget>([this](ListViewWidget& list) {
        list.highlight<RectangleWidget>([this](RectangleWidget& item) {
            item.radius(m_radius.state()).color(m_highlightColor.state());
        });
    });

    updateTextColor(false);

    m_style.onChange([this](const ComboBoxStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().comboBox);
}

void ComboBoxWidget::render(Renderer& renderer) {
    m_settled = true;
    ComboBoxBase::render(renderer);
}

void ComboBoxWidget::styleRowText(TextWidget& label) {
    label.color(m_rowTextColor).font(m_font.state());
}

void ComboBoxWidget::updateColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledColor.get()
                         : pressed() ? m_pressedColor.get()
                         : hovered() ? m_hoverColor.get()
                         : m_backgroundColor.get();
    m_backgroundColorOut.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}

void ComboBoxWidget::updateTextColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledTextColor.get()
                         : currentIndex() < 0 ? m_placeholderColor.get()
                         : m_textColor.get();
    m_textColorOut.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}

void ComboBoxWidget::applyStyle(const ComboBoxStyle& value) {
    backgroundColor(value.backgroundColor);
    hoverColor(value.hoverColor);
    pressedColor(value.pressedColor);
    disabledColor(value.disabledColor);
    borderColor(value.borderColor);
    borderWidth(value.borderWidth);
    radius(value.radius);
    textColor(value.textColor);
    placeholderColor(value.placeholderColor);
    disabledTextColor(value.disabledTextColor);
    font(value.font);
    indicatorColor(value.indicatorColor);
    highlightColor(value.highlightColor);
    transition(value.transition);
}

}
