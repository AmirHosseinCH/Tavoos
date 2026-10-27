#pragma once

#include <tavoos/animation/animation.h>
#include <tavoos/component/componentbase.h>
#include <tavoos/effects/blurstyle.h>
#include <tavoos/effects/glassstyle.h>
#include <tavoos/effects/graphicaleffect.h>
#include <tavoos/effects/shadowstyle.h>
#include <tavoos/events/events.h>
#include <tavoos/export.hpp>
#include <tavoos/object.h>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Tavoos {

class Renderer;
class Window;
class DropShadow;
class InnerShadow;

class TAVOOS_EXPORT Widget : public Object {
    friend class Renderer;
    friend class Window;
    friend class GraphicalEffect;
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

    decltype(auto) widthFraction(this auto&& self, PropertyArg<float> fraction) {
        fraction.applyTo(self.m_widthFraction);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) heightFraction(this auto&& self, PropertyArg<float> fraction) {
        fraction.applyTo(self.m_heightFraction);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) xFraction(this auto&& self, PropertyArg<float> fraction) {
        fraction.applyTo(self.m_xFraction);
        self.m_alignment.set(withoutFlag(self.m_alignment.get(), HorizontalAlignment));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) yFraction(this auto&& self, PropertyArg<float> fraction) {
        fraction.applyTo(self.m_yFraction);
        self.m_alignment.set(withoutFlag(self.m_alignment.get(), VerticalAlignment));
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
    decltype(auto) scaleX(this auto&& self, PropertyArg<float> scale) {
        scale.applyTo(self.m_scaleX);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) scaleY(this auto&& self, PropertyArg<float> scale) {
        scale.applyTo(self.m_scaleY);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) rotationX(this auto&& self, PropertyArg<float> rotation) {
        rotation.applyTo(self.m_rotationX);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) rotationY(this auto&& self, PropertyArg<float> rotation) {
        rotation.applyTo(self.m_rotationY);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) perspective(this auto&& self, PropertyArg<float> distance) {
        distance.applyTo(self.m_perspective);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) transformOrigin(this auto&& self, TransformOrigin origin) {
        PropertyArg<Point>{transformOriginFraction(origin)}.applyTo(self.m_transformOrigin);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) transformOrigin(this auto&& self, PropertyArg<Point> origin) {
        origin.applyTo(self.m_transformOrigin);
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

    decltype(auto) layerBlur(this auto&& self, const BlurStyle& style) {
        self.m_layerBlur.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) layerBlur(this auto&& self, State<BlurStyle>& style) {
        PropertyArg<BlurStyle>{style}.applyTo(self.m_layerBlur);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) layerBlur(this auto&& self, float radius) {
        self.m_layerBlur.set(BlurStyle{radius, radius, 0.0f});
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) backgroundBlur(this auto&& self, const BlurStyle& style) {
        self.m_backgroundBlur.set(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) backgroundBlur(this auto&& self, State<BlurStyle>& style) {
        PropertyArg<BlurStyle>{style}.applyTo(self.m_backgroundBlur);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) backgroundBlur(this auto&& self, float radius) {
        self.m_backgroundBlur.set(BlurStyle{radius, radius, 0.0f});
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) glass(this auto&& self, const GlassStyle& style) {
        self.m_glass.set(style);
        self.m_glassEnabled.set(true);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) glass(this auto&& self, State<GlassStyle>& style) {
        PropertyArg<GlassStyle>{style}.applyTo(self.m_glass);
        self.m_glassEnabled.set(true);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) clearGlass(this auto&& self) {
        self.m_glassEnabled.set(false);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) z(this auto&& self, PropertyArg<int> z) {
        z.applyTo(self.m_z);
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

    decltype(auto) cursor(this auto&& self, PropertyArg<CursorShape> shape) {
        shape.applyTo(self.m_cursor);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) draggable(this auto&& self, PropertyArg<bool> draggable) {
        draggable.applyTo(self.m_draggable);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragThreshold(this auto&& self, PropertyArg<float> threshold) {
        threshold.applyTo(self.m_dragThreshold);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragXAxis(this auto&& self, const DragAxis& axis) {
        self.m_dragXAxis.set(axis);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragXAxis(this auto&& self, State<DragAxis>& axis) {
        self.m_dragXAxis.bind(axis);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragYAxis(this auto&& self, const DragAxis& axis) {
        self.m_dragYAxis.set(axis);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragYAxis(this auto&& self, State<DragAxis>& axis) {
        self.m_dragYAxis.bind(axis);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragKeys(this auto&& self, std::vector<std::string> keys) {
        self.m_dragKeys = std::move(keys);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragData(this auto&& self, DragData data) {
        self.m_dragData = std::move(data);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dragHotSpot(this auto&& self, PropertyArg<Point> hotSpot) {
        hotSpot.applyTo(self.m_dragHotSpot);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onDragStart(this auto&& self, std::function<void(DragEvent&)> callback) {
        self.m_onDragStart = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onDragMove(this auto&& self, std::function<void(DragEvent&)> callback) {
        self.m_onDragMove = std::move(callback);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) onDragEnd(this auto&& self, std::function<void(DragEvent&)> callback) {
        self.m_onDragEnd = std::move(callback);
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

    decltype(auto) onShortcutOverride(this auto&& self, std::function<void(KeyEvent&)> callback) {
        self.m_onShortcutOverride = std::move(callback);
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
    float widthFraction() const { return m_widthFraction; }
    float heightFraction() const { return m_heightFraction; }
    float xFraction() const { return m_xFraction; }
    float yFraction() const { return m_yFraction; }

    float rotation() const { return m_rotation; }
    float scale() const { return m_scale; }
    float scaleX() const { return m_scaleX; }
    float scaleY() const { return m_scaleY; }
    float rotationX() const { return m_rotationX; }
    float rotationY() const { return m_rotationY; }
    float perspective() const { return m_perspective; }
    Point transformOrigin() const { return m_transformOrigin.get(); }

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
    Window* ownerWindow() const noexcept { return m_ownerWindow; }
    bool built() const noexcept { return m_built; }
    bool focusable() const { return m_focusable; }
    bool focused() const { return m_focused; }
    State<bool>& focusedState() { return m_focused; }
    CursorShape cursor() const { return m_cursor; }
    bool draggable() const { return m_draggable; }
    float dragThreshold() const { return m_dragThreshold; }
    DragAxis dragXAxis() const { return m_dragXAxis; }
    DragAxis dragYAxis() const { return m_dragYAxis; }
    const std::vector<std::string>& dragKeys() const { return m_dragKeys; }
    const DragData& dragData() const { return m_dragData; }
    Point dragHotSpot() const { return m_dragHotSpot; }
    float opacity() const { return m_opacity; }
    BlurStyle layerBlur() const { return m_layerBlur; }
    BlurStyle backgroundBlur() const { return m_backgroundBlur; }
    GlassStyle glass() const { return m_glass; }
    bool hasGlass() const { return m_glassEnabled; }
    float effectiveOpacity() const { return m_effectiveOpacity; }
    int z() const { return m_z; }

    float resolvedX()      const { return m_resolvedX; }
    float resolvedY()      const { return m_resolvedY; }
    float resolvedWidth()  const { return m_resolvedWidth; }
    float resolvedHeight() const { return m_resolvedHeight; }

    float displayedWidth()  const { return m_displayedWidth; }
    float displayedHeight() const { return m_displayedHeight; }

    virtual bool isLayouter() const { return false; }
    virtual bool isOverlay() const { return false; }
    virtual std::optional<CornerRadii> shapeRadii() const { return std::nullopt; }

    bool participatesInLayout() const { return visible() && !isOverlay(); }

    struct Size { float width{0.0f}, height{0.0f}; };
    Size intrinsicSize();

    glm::mat4 localMatrix() const;
    const glm::mat4& worldMatrix() const;

    Point mapToParent(const Point& point) const;
    Point mapFromParent(const Point& point) const;
    Point mapToWindow(const Point& point) const;
    Point mapFromWindow(const Point& point) const;

    template<typename T>
    T* addChild(std::function<void(T&)> body = {}) {
        static_assert(std::is_base_of_v<Object, T>, "addChild<T>() requires T to derive from Object");
        auto child = std::make_unique<T>(this);
        T* raw = child.get();
        if constexpr (std::is_base_of_v<ComponentBase, T>)
            static_cast<ComponentBase&>(*raw).runBuild();
        if (body)
            body(*raw);
        std::unique_ptr<Object> asObject = std::move(child);
        appendChild(asObject);
        if constexpr (std::is_base_of_v<Widget, T>) {
            raw->finishBuild();
            raw->requestRelayout();
        }
        return raw;
    }

    DropShadow& addDropShadow(const ShadowStyle& style = {});
    DropShadow& addDropShadow(State<ShadowStyle>& style);
    InnerShadow& addInnerShadow(const ShadowStyle& style = {});
    InnerShadow& addInnerShadow(State<ShadowStyle>& style);

    decltype(auto) dropShadow(this auto&& self, const ShadowStyle& style = {}) {
        self.addDropShadow(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) dropShadow(this auto&& self, State<ShadowStyle>& style) {
        self.addDropShadow(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerShadow(this auto&& self, const ShadowStyle& style = {}) {
        self.addInnerShadow(style);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) innerShadow(this auto&& self, State<ShadowStyle>& style) {
        self.addInnerShadow(style);
        return std::forward<decltype(self)>(self);
    }

    void removeEffect(GraphicalEffect& effect);
    void clearEffects();
    const std::vector<std::unique_ptr<GraphicalEffect>>& effects() const noexcept { return m_effects; }

    void removeSelf();
    void removeChild(Object* child);

protected:
    void updateWorldMatrix();

    virtual void triggerClick(MouseEvent& event)       { if (m_onClick) m_onClick(event); }
    virtual void triggerDoubleClick(MouseEvent& event) { if (m_onDoubleClick) m_onDoubleClick(event); }
    virtual void triggerPress(MouseEvent& event)       { if (m_onPress) m_onPress(event); }
    virtual void triggerRelease(MouseEvent& event)     { if (m_onRelease) m_onRelease(event); }
    virtual void triggerMouseMove(MouseEvent& event)   { if (m_onMouseMove) m_onMouseMove(event); }
    virtual void triggerMouseEnter(MouseEvent& event)  { if (m_onMouseEnter) m_onMouseEnter(event); }
    virtual void triggerMouseLeave(MouseEvent& event)  { if (m_onMouseLeave) m_onMouseLeave(event); }
    virtual void triggerDragStart(DragEvent& event) { if (m_onDragStart) m_onDragStart(event); }

    virtual void triggerDragMove(DragEvent& event) {
        if (m_draggable) {
            const DragAxis& xAxis = m_dragXAxis.get();
            const DragAxis& yAxis = m_dragYAxis.get();
            if (xAxis.enabled) {
                const int newX = dragBase(true) + static_cast<int>(std::lround(event.dx()));
                x(std::clamp(newX, xAxis.min, xAxis.max));
            }
            if (yAxis.enabled) {
                const int newY = dragBase(false) + static_cast<int>(std::lround(event.dy()));
                y(std::clamp(newY, yAxis.min, yAxis.max));
            }
        }
        if (m_onDragMove)
            m_onDragMove(event);
    }

    virtual void triggerDragEnd(DragEvent& event) { if (m_onDragEnd) m_onDragEnd(event); }
    virtual void triggerWheel(WheelEvent& event)       { if (m_onWheel) m_onWheel(event); }
    virtual void triggerKeyPress(KeyEvent& event)      { if (m_onKeyPress) m_onKeyPress(event); }
    virtual void triggerKeyRelease(KeyEvent& event)    { if (m_onKeyRelease) m_onKeyRelease(event); }
    virtual void triggerShortcutOverride(KeyEvent& event) { if (m_onShortcutOverride) m_onShortcutOverride(event); }
    virtual void triggerTextInput(KeyEvent& event)     { if (m_onTextInput) m_onTextInput(event); }
    virtual void triggerFocusIn(Event& event) {
        m_focused.setIfChanged(true);
        if (m_onFocusIn)
            m_onFocusIn(event);
    }
    virtual void triggerFocusOut(Event& event) {
        m_focused.setIfChanged(false);
        if (m_onFocusOut)
            m_onFocusOut(event);
    }

    virtual bool hasHandlerFor(EventType type);

    virtual void render(Renderer&) = 0;
    void renderChildren(Renderer&);
    virtual Size computeIntrinsicSize();
    virtual void onResolvedSizeChanged() {}
    virtual void onBuilt() {}

    struct ContentArea { float x, y, width, height; };
    ContentArea resolveContentArea();
    int dragBase(bool horizontal);
    virtual ContentArea contentAreaFor(const Widget& child) const;
    bool parentIsLayouter() const;
    bool isSizeBoundary() const;

    void requestRelayout();
    void markLayoutDirty();
    bool consumeLayoutDirty();
    void syncDisplayedGeometry();
    void notifyResolvedSizeIfChanged();
    void requestRepaint();
    void renderInnerEffects(Renderer& renderer);
    void detachAndDefer(Object* child);
    template<typename... Props>
    void bindRepaintTriggers(Props&... props) {
        auto repaint = [this](const auto&) { requestRepaint(); };
        (props.onChange(repaint), ...);
    }

    static constexpr Point transformOriginFraction(TransformOrigin origin) noexcept {
        switch (origin) {
        case TransformOrigin::TopLeft:     return {0.0f, 0.0f};
        case TransformOrigin::Top:         return {0.5f, 0.0f};
        case TransformOrigin::TopRight:    return {1.0f, 0.0f};
        case TransformOrigin::Left:        return {0.0f, 0.5f};
        case TransformOrigin::Center:      return {0.5f, 0.5f};
        case TransformOrigin::Right:       return {1.0f, 0.5f};
        case TransformOrigin::BottomLeft:  return {0.0f, 1.0f};
        case TransformOrigin::Bottom:      return {0.5f, 1.0f};
        case TransformOrigin::BottomRight: return {1.0f, 1.0f};
        }
        return {0.5f, 0.5f};
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
    void finishBuild() {
        m_built = true;
        onBuilt();
    }

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
    Property<float> m_widthFraction{-1.0f};
    Property<float> m_heightFraction{-1.0f};
    Property<float> m_xFraction{-1.0f};
    Property<float> m_yFraction{-1.0f};
    Property<float> m_rotation{0.0f};
    Property<float> m_scale{1.0f};
    Property<float> m_scaleX{1.0f};
    Property<float> m_scaleY{1.0f};
    Property<float> m_rotationX{0.0f};
    Property<float> m_rotationY{0.0f};
    Property<float> m_perspective{1000.0f};
    Property<Point> m_transformOrigin{Point{0.5f, 0.5f}};
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
    State<bool> m_focused{false};
    Property<CursorShape> m_cursor{CursorShape::Inherit};
    Property<bool> m_draggable{false};
    Property<float> m_dragThreshold{0.0f};
    Property<DragAxis> m_dragXAxis{DragAxis{}};
    Property<DragAxis> m_dragYAxis{DragAxis{}};
    std::vector<std::string> m_dragKeys;
    DragData m_dragData;
    Property<Point> m_dragHotSpot{Point{0.5f, 0.5f}};
    Property<float> m_opacity{1.0f};
    Property<BlurStyle> m_layerBlur{BlurStyle{}};
    Property<BlurStyle> m_backgroundBlur{BlurStyle{}};
    Property<GlassStyle> m_glass{GlassStyle{}};
    Property<bool> m_glassEnabled{false};
    Property<int> m_z{0};

    float m_effectiveOpacity{1.0f};
    float m_resolvedX{0.0f};
    float m_resolvedY{0.0f};
    float m_resolvedWidth{0.0f};
    float m_resolvedHeight{0.0f};
    float m_notifiedWidth{-1.0f};
    float m_notifiedHeight{-1.0f};

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
    std::function<void(DragEvent&)> m_onDragStart;
    std::function<void(DragEvent&)> m_onDragMove;
    std::function<void(DragEvent&)> m_onDragEnd;
    std::function<void(WheelEvent&)> m_onWheel;
    std::function<void(KeyEvent&)> m_onKeyPress;
    std::function<void(KeyEvent&)> m_onKeyRelease;
    std::function<void(KeyEvent&)> m_onShortcutOverride;
    std::function<void(KeyEvent&)> m_onTextInput;
    std::function<void(Event&)> m_onFocusIn;
    std::function<void(Event&)> m_onFocusOut;

    bool hitTest(float px, float py) const;
    Widget* hitTestTree(float px, float py, const Widget* skip = nullptr) const;
    void updateEffectiveOpacity();

    template<typename E>
    E& attachEffect();

    static std::vector<Widget*> zOrderedChildren(const Object& parent);

    bool m_layoutDirty{true};
    bool m_intrinsicSizeCacheValid{false};
    Size m_cachedIntrinsicSize{};
    Window* m_ownerWindow{nullptr};
    bool m_built{false};
    std::vector<std::unique_ptr<GraphicalEffect>> m_effects;
    static bool s_isBuilding;
};

}
