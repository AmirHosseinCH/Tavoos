#include <tavoos/animation/transitions.h>

#include <tavoos/widget/widget.h>

namespace Tavoos {

void FadeIn::prepare(Widget& item, float, float) {
    m_item = &item;
    m_saved = item.opacity();
    item.opacity(0.0f);
}

void FadeIn::update(float progress) {
    m_item->opacity(m_saved * progress);
}

void FadeIn::finish(bool) {
    m_item->opacity(m_saved);
}

void FadeOut::prepare(Widget& item, float, float) {
    m_item = &item;
    m_saved = item.opacity();
}

void FadeOut::update(float progress) {
    m_item->opacity(m_saved * (1.0f - progress));
}

void FadeOut::finish(bool) {
    m_item->opacity(m_saved);
}

namespace {

void captureMargins(const Widget& item, float& left, float& top, float& right, float& bottom) {
    left = item.marginLeft();
    top = item.marginTop();
    right = item.marginRight();
    bottom = item.marginBottom();
}

void applyOffset(Widget& item, float dx, float dy, float left, float top, float right, float bottom) {
    item.marginLeft(left + dx).marginRight(right - dx).marginTop(top + dy).marginBottom(bottom - dy);
}

void offsetFor(Edge edge, float amount, float width, float height, float& dx, float& dy) {
    dx = 0.0f;
    dy = 0.0f;
    switch (edge) {
    case Edge::Right:  dx = amount * width; break;
    case Edge::Left:   dx = -amount * width; break;
    case Edge::Bottom: dy = amount * height; break;
    case Edge::Top:    dy = -amount * height; break;
    }
}

}

void SlideIn::prepare(Widget& item, float containerWidth, float containerHeight) {
    m_item = &item;
    m_width = containerWidth;
    m_height = containerHeight;
    captureMargins(item, m_left, m_top, m_right, m_bottom);
    update(0.0f);
}

void SlideIn::update(float progress) {
    float dx, dy;
    offsetFor(m_edge, 1.0f - progress, m_width, m_height, dx, dy);
    applyOffset(*m_item, dx, dy, m_left, m_top, m_right, m_bottom);
}

void SlideIn::finish(bool) {
    applyOffset(*m_item, 0.0f, 0.0f, m_left, m_top, m_right, m_bottom);
}

void SlideOut::prepare(Widget& item, float containerWidth, float containerHeight) {
    m_item = &item;
    m_width = containerWidth;
    m_height = containerHeight;
    captureMargins(item, m_left, m_top, m_right, m_bottom);
}

void SlideOut::update(float progress) {
    float dx, dy;
    offsetFor(m_edge, progress, m_width, m_height, dx, dy);
    applyOffset(*m_item, dx, dy, m_left, m_top, m_right, m_bottom);
}

void SlideOut::finish(bool) {
    applyOffset(*m_item, 0.0f, 0.0f, m_left, m_top, m_right, m_bottom);
}

void ScaleIn::prepare(Widget& item, float, float) {
    m_item = &item;
    m_saved = item.scale();
    item.scale(m_saved * m_from);
}

void ScaleIn::update(float progress) {
    m_item->scale(m_saved * (m_from + (1.0f - m_from) * progress));
}

void ScaleIn::finish(bool) {
    m_item->scale(m_saved);
}

void ScaleOut::prepare(Widget& item, float, float) {
    m_item = &item;
    m_saved = item.scale();
}

void ScaleOut::update(float progress) {
    m_item->scale(m_saved * (1.0f + (m_to - 1.0f) * progress));
}

void ScaleOut::finish(bool) {
    m_item->scale(m_saved);
}

void LambdaTransition::prepare(Widget& item, float, float) {
    m_item = &item;
    m_apply(item, 0.0f);
}

void LambdaTransition::update(float progress) {
    m_apply(*m_item, progress);
}

}
