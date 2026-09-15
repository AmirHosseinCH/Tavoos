#pragma once

#include <tavoos/animation/animation.h>
#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/object.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Tavoos {

class Renderer;
class Window;

class TAVOOS_EXPORT Widget : public Object {
    friend class Renderer;
    friend class Window;
    friend class Builder;
    friend class ColumnWidget;
    friend class RowWidget;
    friend class GridWidget;
    friend class FlexWidget;

public:
    Widget(Object*);
    ~Widget() override;

    decltype(auto) x(this auto&& self, PropertyArg<int> x) {
        x.applyTo(self.m_x);
        self.m_alignment.set(withoutFlag(self.m_alignment.get(), HorizontalAlignment));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) y(this auto&& self, PropertyArg<int> y) {
        y.applyTo(self.m_y);
        self.m_alignment.set(withoutFlag(self.m_alignment.get(), VerticalAlignment));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) width(this auto&& self, PropertyArg<int> width) {
        width.applyTo(self.m_width);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) height(this auto&& self, PropertyArg<int> height) {
        height.applyTo(self.m_height);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) rotation(this auto&& self, PropertyArg<float> rotation) {
        rotation.applyTo(self.m_rotation);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) scale(this auto&& self, PropertyArg<float> scale) {
        scale.applyTo(self.m_scale);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) alignment(this auto&& self, PropertyArg<Alignment> alignment) {
        alignment.applyTo(self.m_alignment);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fill(this auto&& self, PropertyArg<Fill> fill) {
        fill.applyTo(self.m_fill);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) clip(this auto&& self, PropertyArg<bool> clip) {
        clip.applyTo(self.m_clip);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) gridColumnSpan(this auto&& self, PropertyArg<int> span) {
        span.applyTo(self.m_gridColumnSpan);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) gridRowSpan(this auto&& self, PropertyArg<int> span) {
        span.applyTo(self.m_gridRowSpan);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) gridRow(this auto&& self, PropertyArg<int> row) {
        row.applyTo(self.m_gridRow);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) gridColumn(this auto&& self, PropertyArg<int> column) {
        column.applyTo(self.m_gridColumn);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) padding(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_padding.leftProperty());
        value.applyTo(self.m_padding.topProperty());
        value.applyTo(self.m_padding.rightProperty());
        value.applyTo(self.m_padding.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) paddingLeft(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_padding.leftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) paddingTop(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_padding.topProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) paddingRight(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_padding.rightProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) paddingBottom(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_padding.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) margin(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_margin.leftProperty());
        value.applyTo(self.m_margin.topProperty());
        value.applyTo(self.m_margin.rightProperty());
        value.applyTo(self.m_margin.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) marginLeft(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_margin.leftProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) marginTop(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_margin.topProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) marginRight(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_margin.rightProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) marginBottom(this auto&& self, PropertyArg<float> value) {
        value.applyTo(self.m_margin.bottomProperty());
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) visible(this auto&& self, PropertyArg<bool> visible) {
        visible.applyTo(self.m_visible);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) focusable(this auto&& self, PropertyArg<bool> focusable) {
        focusable.applyTo(self.m_focusable);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) opacity(this auto&& self, PropertyArg<float> opacity) {
        opacity.applyTo(self.m_opacity);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) alignmentAnimation(this auto&& self, PropertyArg<bool> enabled, PropertyArg<float> duration, PropertyArg<EasingFn> easing = Easing::linear) {
        enabled.applyTo(self.m_alignmentAnimationEnabled);
        duration.applyTo(self.m_alignmentAnimationDuration);
        easing.applyTo(self.m_alignmentAnimationEasing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) fillAnimation(this auto&& self, PropertyArg<bool> enabled, PropertyArg<float> duration, PropertyArg<EasingFn> easing = Easing::linear) {
        enabled.applyTo(self.m_fillAnimationEnabled);
        duration.applyTo(self.m_fillAnimationDuration);
        easing.applyTo(self.m_fillAnimationEasing);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onClick(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onClick = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onPress(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onPress = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onDoubleClick(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onDoubleClick = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onRelease(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onRelease = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onMouseMove(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onMouseMove = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onMouseEnter(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onMouseEnter = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onMouseLeave(this auto&& self, std::function<void(MouseEvent&)> callback) {
        self.m_onMouseLeave = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onWheel(this auto&& self, std::function<void(WheelEvent&)> callback) {
        self.m_onWheel = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onKeyPress(this auto&& self, std::function<void(KeyEvent&)> callback) {
        self.m_onKeyPress = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onKeyRelease(this auto&& self, std::function<void(KeyEvent&)> callback) {
        self.m_onKeyRelease = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onTextInput(this auto&& self, std::function<void(KeyEvent&)> callback) {
        self.m_onTextInput = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onFocusIn(this auto&& self, std::function<void(Event&)> callback) {
        self.m_onFocusIn = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onFocusOut(this auto&& self, std::function<void(Event&)> callback) {
        self.m_onFocusOut = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    void focus();

    int x() const { return m_x; }
    int y() const { return m_y; }

    int width() const { return m_width; }
    int height() const { return m_height; }

    float rotation() const { return m_rotation; }
    float scale() const { return m_scale; }

    Alignment alignment() const { return m_alignment; }
    Fill fill() const { return m_fill; }
    bool clip() const { return m_clip; }
    int gridColumnSpan() const { return m_gridColumnSpan; }
    int gridRowSpan() const { return m_gridRowSpan; }
    int gridRow() const { return m_gridRow; }
    int gridColumn() const { return m_gridColumn; }

    float paddingLeft()   const { return m_padding.left(); }
    float paddingTop()    const { return m_padding.top(); }
    float paddingRight()  const { return m_padding.right(); }
    float paddingBottom() const { return m_padding.bottom(); }

    float marginLeft()   const { return m_margin.left(); }
    float marginTop()    const { return m_margin.top(); }
    float marginRight()  const { return m_margin.right(); }
    float marginBottom() const { return m_margin.bottom(); }

    bool visible() const { return m_visible; }
    bool focusable() const { return m_focusable; }
    float opacity() const { return m_opacity; }
    float effectiveOpacity() const { return m_effectiveOpacity; }

    float resolvedX()      const { return m_resolvedX; }
    float resolvedY()      const { return m_resolvedY; }
    float resolvedWidth()  const { return m_resolvedWidth; }
    float resolvedHeight() const { return m_resolvedHeight; }

    float displayedWidth()  const { return m_displayedWidth; }
    float displayedHeight() const { return m_displayedHeight; }

    virtual bool isLayouter() const { return false; }

    struct Size { float width{0.0f}, height{0.0f}; };
    Size intrinsicSize();

    glm::mat4 localMatrix() const;
    const glm::mat4& worldMatrix() const;

    template<typename T>
    T* addChild(std::function<void(T&)> body = {}) {
        static_assert(std::is_base_of_v<Widget, T>, "addChild<T>() requires T to derive from Widget");
        auto child = std::make_unique<T>(this);
        T* raw = child.get();
        if (body)
            body(*raw);
        std::unique_ptr<Object> asObject = std::move(child);
        appendChild(asObject);
        raw->requestRelayout();
        return raw;
    }

    void removeSelf();
    void removeChild(Widget* child);

protected:
    void updateWorldMatrix();

    void triggerClick(MouseEvent& event)       { if (m_onClick) m_onClick(event); }
    void triggerDoubleClick(MouseEvent& event) { if (m_onDoubleClick) m_onDoubleClick(event); }
    void triggerPress(MouseEvent& event)       { if (m_onPress) m_onPress(event); }
    void triggerRelease(MouseEvent& event)     { if (m_onRelease) m_onRelease(event); }
    void triggerMouseMove(MouseEvent& event)   { if (m_onMouseMove) m_onMouseMove(event); }
    void triggerMouseEnter(MouseEvent& event)  { if (m_onMouseEnter) m_onMouseEnter(event); }
    void triggerMouseLeave(MouseEvent& event)  { if (m_onMouseLeave) m_onMouseLeave(event); }
    void triggerWheel(WheelEvent& event)       { if (m_onWheel) m_onWheel(event); }
    void triggerKeyPress(KeyEvent& event)      { if (m_onKeyPress) m_onKeyPress(event); }
    void triggerKeyRelease(KeyEvent& event)    { if (m_onKeyRelease) m_onKeyRelease(event); }
    void triggerTextInput(KeyEvent& event)     { if (m_onTextInput) m_onTextInput(event); }
    void triggerFocusIn(Event& event)          { if (m_onFocusIn) m_onFocusIn(event); }
    void triggerFocusOut(Event& event)         { if (m_onFocusOut) m_onFocusOut(event); }

    virtual void render(Renderer&) = 0;
    void renderChildren(Renderer&);
    virtual Size computeIntrinsicSize();

    struct ContentArea { float x, y, width, height; };
    ContentArea resolveContentArea();
    bool parentIsLayouter() const;
    bool isSizeBoundary() const;

    void requestRelayout();
    void markLayoutDirty();
    bool consumeLayoutDirty();
    void syncDisplayedGeometry();
    void requestRepaint();
    void detachAndDefer(Widget* child);
    template<typename... Props>
    void bindRepaintTriggers(Props&... props) {
        auto repaint = [this](const auto&) { requestRepaint(); };
        (props.onChange(repaint), ...);
    }
    template<typename... Props>
    void bindRelayoutTriggers(Props&... props) {
        auto relayout = [this](const auto&) { requestRelayout(); };
        (props.onChange(relayout), ...);
    }

    Property<int>& widthProperty() { return m_width; }
    Property<int>& heightProperty() { return m_height; }

private:
    virtual void layout(bool force = false);
    void setResolved(float, float, float, float);

    class PairTween : public AnimatableBase {
    public:
        ~PairTween() override { cancel(); }

        bool isAnimating() const noexcept { return m_animating; }
        float targetA() const noexcept { return m_toA; }
        float targetB() const noexcept { return m_toB; }

        void animateTo(Widget* owner, float* a, float* b,
                        float fromA, float fromB, float toA, float toB,
                        float duration, EasingFn easing);
        void cancel();

    private:
        bool tick(float dt) override;

        Widget* m_owner{nullptr};
        float* m_a{nullptr};
        float* m_b{nullptr};
        float m_fromA{0.0f}, m_fromB{0.0f};
        float m_toA{0.0f}, m_toB{0.0f};
        float m_duration{0.0f};
        float m_elapsed{0.0f};
        EasingFn m_easing{Easing::linear};
        bool m_animating{false};
    };

    Property<int> m_x, m_y;
    Property<int> m_width, m_height;
    Property<float> m_rotation{0.0f};
    Property<float> m_scale{1.0f};
    Property<Alignment> m_alignment{Alignment::None};
    Property<Fill> m_fill{Fill::None};
    Property<bool> m_clip{false};
    Property<int> m_gridColumnSpan{1};
    Property<int> m_gridRowSpan{1};
    Property<int> m_gridRow{-1};
    Property<int> m_gridColumn{-1};
    SidedProperty<float> m_padding{0.0f};
    SidedProperty<float> m_margin{0.0f};
    Property<bool> m_visible{true};
    Property<bool> m_focusable{false};
    Property<float> m_opacity{1.0f};

    float m_effectiveOpacity{1.0f};
    float m_resolvedX{0.0f};
    float m_resolvedY{0.0f};
    float m_resolvedWidth{0.0f};
    float m_resolvedHeight{0.0f};

    float m_displayedX{0.0f};
    float m_displayedY{0.0f};
    float m_displayedWidth{0.0f};
    float m_displayedHeight{0.0f};
    bool m_layoutInitialized{false};

    glm::mat4 m_worldMatrix{1.0f};

    Property<bool> m_alignmentAnimationEnabled{false};
    Property<float> m_alignmentAnimationDuration{0.0f};
    Property<EasingFn> m_alignmentAnimationEasing{Easing::linear};

    Property<bool> m_fillAnimationEnabled{false};
    Property<float> m_fillAnimationDuration{0.0f};
    Property<EasingFn> m_fillAnimationEasing{Easing::linear};

    PairTween m_positionTween;
    PairTween m_sizeTween;

    void syncPositionTween();
    void syncSizeTween();

    unsigned int m_clipMaskTexture{0};
    int m_clipMaskWidth{0};
    int m_clipMaskHeight{0};

    unsigned int m_clipColorTexture{0};
    int m_clipColorWidth{0};
    int m_clipColorHeight{0};

    std::function<void(MouseEvent&)> m_onClick;
    std::function<void(MouseEvent&)> m_onDoubleClick;
    std::function<void(MouseEvent&)> m_onPress;
    std::function<void(MouseEvent&)> m_onRelease;
    std::function<void(MouseEvent&)> m_onMouseMove;
    std::function<void(MouseEvent&)> m_onMouseEnter;
    std::function<void(MouseEvent&)> m_onMouseLeave;
    std::function<void(WheelEvent&)> m_onWheel;
    std::function<void(KeyEvent&)> m_onKeyPress;
    std::function<void(KeyEvent&)> m_onKeyRelease;
    std::function<void(KeyEvent&)> m_onTextInput;
    std::function<void(Event&)> m_onFocusIn;
    std::function<void(Event&)> m_onFocusOut;

    bool hitTest(float px, float py) const;
    Widget* hitTestTree(float px, float py) const;
    bool hasHandlerFor(EventType type);
    void updateEffectiveOpacity();

    bool m_layoutDirty{true};
    bool m_intrinsicSizeCacheValid{false};
    Size m_cachedIntrinsicSize{};
    Window* m_ownerWindow{nullptr};
    static bool s_isBuilding;
};

}
