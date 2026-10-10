#pragma once

#include <tavoos/animation/transition.h>
#include <tavoos/export.hpp>

#include <functional>

namespace Tavoos {

enum class Edge {
    Left,
    Right,
    Top,
    Bottom,
};

class TAVOOS_EXPORT FadeIn : public Transition {
public:
    explicit FadeIn(float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    Widget* m_item{nullptr};
    float m_saved{1.0f};
};

class TAVOOS_EXPORT FadeOut : public Transition {
public:
    explicit FadeOut(float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    Widget* m_item{nullptr};
    float m_saved{1.0f};
};

class TAVOOS_EXPORT SlideIn : public Transition {
public:
    explicit SlideIn(Edge from, float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)}, m_edge{from} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    Edge m_edge;
    Widget* m_item{nullptr};
    float m_width{0.0f};
    float m_height{0.0f};
    float m_left{0.0f}, m_top{0.0f}, m_right{0.0f}, m_bottom{0.0f};
};

class TAVOOS_EXPORT SlideOut : public Transition {
public:
    explicit SlideOut(Edge to, float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)}, m_edge{to} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    Edge m_edge;
    Widget* m_item{nullptr};
    float m_width{0.0f};
    float m_height{0.0f};
    float m_left{0.0f}, m_top{0.0f}, m_right{0.0f}, m_bottom{0.0f};
};

class TAVOOS_EXPORT ScaleIn : public Transition {
public:
    explicit ScaleIn(float from = 0.9f, float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)}, m_from{from} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    float m_from;
    Widget* m_item{nullptr};
    float m_saved{1.0f};
};

class TAVOOS_EXPORT ScaleOut : public Transition {
public:
    explicit ScaleOut(float to = 0.9f, float duration = 0.25f, EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)}, m_to{to} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;
    void finish(bool completed) override;

private:
    float m_to;
    Widget* m_item{nullptr};
    float m_saved{1.0f};
};

class TAVOOS_EXPORT LambdaTransition : public Transition {
public:
    explicit LambdaTransition(std::function<void(Widget&, float)> apply, float duration = 0.25f,
                              EasingFn easing = Easing::easeInOutQuad)
        : Transition{duration, std::move(easing)}, m_apply{std::move(apply)} {}

    void prepare(Widget& item, float containerWidth, float containerHeight) override;
    void update(float progress) override;

private:
    std::function<void(Widget&, float)> m_apply;
    Widget* m_item{nullptr};
};

}
