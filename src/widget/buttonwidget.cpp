#include <tavoos/widget/buttonwidget.h>

#include <tavoos/application.h>
#include <tavoos/widget/row.h>
#include <tavoos/widget/svg.h>

namespace Tavoos {

ButtonWidget::ButtonWidget(Object* parent) : ButtonBase{parent} {
    m_color.set(m_idleColor.get());
    m_textColorOut.set(m_textColor.get());
    m_borderColorOut.set(m_borderColor.get());

    m_idleColor.onChange([this](const Paint&) { updateColor(false); });
    m_hoverColor.onChange([this](const Paint&) { updateColor(false); });
    m_disabledColor.onChange([this](const Paint&) { updateColor(false); });
    hoveredState().onChange([this](const bool&) { updateColor(true); });
    enabledState().onChange([this](const bool&) {
        updateColor(true);
        updateTextColor(true);
        updateBorderColor(true);
    });

    m_borderColor.onChange([this](const Paint&) { updateBorderColor(false); });
    m_disabledBorderColor.onChange([this](const Paint&) { updateBorderColor(false); });
    m_variant.onChange([this](const ButtonVariant& variant) {
        m_variantWins = true;
        applyVariant(variant, m_style.get());
    });

    m_textColor.onChange([this](const Paint&) { updateTextColor(false); });
    m_disabledTextColor.onChange([this](const Paint&) { updateTextColor(false); });
    m_text.onChange([this](const std::string&) { requestRelayout(); });
    m_font.onChange([this](const Font&) { requestRelayout(); });

    m_icon.onChange([this](const std::string& source) {
        if (source.empty() == m_hasIcon)
            rebuildContent();
    });
    m_iconPosition.onChange([this](const ButtonIconPosition&) { rebuildContent(); });
    m_display.onChange([this](const ButtonDisplay&) { rebuildContent(); });

    background([this](RectangleWidget& rect) {
        rect.radius(m_radius.state()).color(m_color).borderWidth(m_borderWidth.state()).borderColor(m_borderColorOut);
    });
    rebuildContent();

    m_style.onChange([this](const ButtonStyle& style) { applyStyle(style); });
    style(Application::instance()->theme().button);
}

void ButtonWidget::applyStyle(const ButtonStyle& value) {
    if (m_variantWins) {
        applyVariant(m_variant.get(), value);
    } else {
        idleColor(value.idleColor);
        hoverColor(value.hoverColor);
        disabledColor(value.disabledColor);
        textColor(value.textColor);
        disabledTextColor(value.disabledTextColor);
        borderWidth(value.borderWidth);
    }
    borderColor(value.borderColor);
    disabledBorderColor(value.disabledBorderColor);
    radius(value.radius);
    font(value.font);
    transition(value.transition);
}

void ButtonWidget::rebuildContent() {
    if (m_ownRevision != 0 && m_ownRevision != contentRevision())
        return;

    m_hasIcon = !m_icon.get().empty();
    const bool showIcon = m_hasIcon && m_display != ButtonDisplay::TextOnly;
    const bool showText = m_display != ButtonDisplay::IconOnly;
    const bool iconFirst = m_iconPosition == ButtonIconPosition::Left;
    const float padding = showText ? 16.0f : 8.0f;

    auto addIcon = [this](RowWidget& row) {
        row.addChild<SVGWidget>([this](SVGWidget& svg) {
            svg.source(m_icon.state()).width(m_iconSize.state()).height(m_iconSize.state())
                .color(m_textColorOut).alignment(Alignment::CenterVertical);
        });
    };
    auto addLabel = [this](RowWidget& row) {
        row.addChild<TextWidget>([this](TextWidget& label) {
            label.text(m_text.state()).font(m_font.state()).color(m_textColorOut)
                .alignment(Alignment::CenterVertical);
        });
    };

    content<RowWidget>([&](RowWidget& row) {
        row.spacing(m_iconSpacing.state()).marginLeft(padding).marginRight(padding).marginTop(8).marginBottom(8);
        if (showIcon && iconFirst)
            addIcon(row);
        if (showText)
            addLabel(row);
        if (showIcon && !iconFirst)
            addIcon(row);
    });
    m_ownRevision = contentRevision();
}

void ButtonWidget::render(Renderer& renderer) {
    m_settled = true;
    ButtonBase::render(renderer);
}

void ButtonWidget::updateColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledColor.get() : hovered() ? m_hoverColor.get() : m_idleColor.get();
    m_color.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}

void ButtonWidget::applyVariant(ButtonVariant variant, const ButtonStyle& base) {
    if (variant == ButtonVariant::Filled) {
        m_idleColor.set(base.idleColor);
        m_hoverColor.set(base.hoverColor);
        m_disabledColor.set(base.disabledColor);
        m_textColor.set(base.textColor);
        m_borderWidth.set(0.0f);
    } else {
        Color accent = base.idleColor.solid;
        Color transparent = accent;
        transparent.a = 0.0f;
        Color tint = accent;
        tint.a = 26.0f / 255.0f;

        m_idleColor.set(Paint{transparent});
        m_hoverColor.set(Paint{tint});
        m_disabledColor.set(Paint{transparent});
        m_textColor.set(Paint{accent});
        m_borderWidth.set(variant == ButtonVariant::Outlined ? 1.0f : 0.0f);
    }
    m_disabledTextColor.set(base.disabledTextColor);
}

void ButtonWidget::updateBorderColor(bool animate) {
    const Paint& target = enabled() ? m_borderColor.get() : m_disabledBorderColor.get();
    m_borderColorOut.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}

void ButtonWidget::updateTextColor(bool animate) {
    const Paint& target = enabled() ? m_textColor.get() : m_disabledTextColor.get();
    m_textColorOut.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}

}
