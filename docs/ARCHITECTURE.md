# Tavoos Architecture

This document explains how Tavoos is built internally, with the actual source next to the
explanation, for anyone reading or contributing to Tavoos's own code. If you're *using*
Tavoos as a library, see [GUIDE.md](GUIDE.md) instead.

The codebase has no inline comments by design (a deliberate, deferred choice) - this
document is where the "why" behind non-obvious code lives instead.

## Table of contents

1. [Design philosophy](#design-philosophy)
2. [The object model](#the-object-model)
3. [Building the tree](#building-the-tree)
4. [Reactive properties](#reactive-properties)
   - [`Property<T>`](#propertyt)
   - [`State<T>`](#statet)
   - [`PropertyArg<T>`](#propertyargt)
   - [`SidedProperty<T>` / `CornerProperty<T>`](#sidedpropertyt--cornerpropertyt)
5. [Layout system](#layout-system)
   - [Dirty-flag propagation](#dirty-flag-propagation)
   - [`Widget::layout()` - the non-layouter default](#widgetlayout---the-non-layouter-default)
   - [Column & Row](#column--row)
   - [Grid](#grid)
   - [Flex](#flex)
6. [Rendering pipeline](#rendering-pipeline)
   - [Per-frame sequence](#per-frame-sequence)
   - [Clipping](#clipping)
7. [Animation system](#animation-system)
   - [`AnimatableBase` / `AnimationManager`](#animatablebase--animationmanager)
   - [`AnimatedState<T>`](#animatedstatet)
   - [`Widget::PairTween`](#widgetpairtween)
8. [Event system](#event-system)
   - [Bubbling](#bubbling)
   - [Hit-testing](#hit-testing)
   - [Press / release / click / double-click](#press--release--click--double-click)
   - [Focus chain](#focus-chain)
9. [Image & SVG rendering and caching](#image--svg-rendering-and-caching)
10. [Text rendering](#text-rendering)
    - [`FontFace` / `FontManager` / `GlyphAtlas`](#fontface--fontmanager--glyphatlas)
    - [Wrap, elide, and line-clamp](#wrap-elide-and-line-clamp)
11. [Resource embedding](#resource-embedding)
12. [Application & Window lifecycle](#application--window-lifecycle)
13. [Adding a new widget type](#adding-a-new-widget-type)

## Design philosophy

Tavoos is a **retained-mode** widget tree (unlike immediate-mode GUI libraries): you build a
tree of `Widget` objects once via a declarative builder, and the framework keeps it alive,
re-laying-out and re-rendering only the parts that changed. Three ideas run through the
whole codebase:

1. **Declarative construction** - a widget tree is built as nested lambdas, using C++23
   "deducing this" so every setter chains fluently regardless of which class in the
   hierarchy declares it.
2. **Reactive properties** - a widget's fields are `Property<T>`, which can either hold a
   plain value or be bound to a shared `State<T>`. Changing a `State<T>` automatically
   propagates to every bound property and triggers relayout/repaint.
3. **Render-on-dirty** - nothing lays out or redraws every frame unconditionally. Property
   changes mark narrow dirty flags; `Application::run()`'s loop only calls into
   layout/render for a window when something in it is actually dirty.

## The object model

```
Object                    (src/include/tavoos/object.h)
 ├─ owns children: std::vector<std::unique_ptr<Object>>
 ├─ Window                (src/include/tavoos/window.h)      -- tree root, not a Widget
 └─ Widget                (src/include/tavoos/widget/widget.h)
     ├─ RectangleWidget, ImageWidget, SVGWidget, TextWidget
     └─ ColumnWidget, RowWidget, GridWidget, FlexWidget       -- layout containers
```

`Object` (`object.h`) is the minimal parent/child ownership primitive:

```cpp
class TAVOOS_EXPORT Object {
    friend class Widget;
    friend class Builder;

public:
    Object(Object* = nullptr);
    virtual ~Object() {};

    Object* parent() const { return m_parent; }
    const std::vector<std::unique_ptr<Object>>& children() const { return m_childrens; }

protected:
    void clearChildren() { m_childrens.clear(); }

private:
    virtual void appendChild(std::unique_ptr<Object>& child);
    virtual std::unique_ptr<Object> detachChild(Object* child);

    Object* m_parent{nullptr};
    std::vector<std::unique_ptr<Object>> m_childrens;
};
```

It intentionally knows nothing about geometry, rendering, or events - that's all added by
`Widget`. `Window` derives from `Object` directly (not `Widget`) because a window isn't laid
out or rendered by a parent - it's the root the rest of the tree hangs off.

`appendChild`/`detachChild` are `private`, with `friend class Widget;`/`friend class
Builder;`.

## Building the tree

There are two distinct ways to add a widget to the tree, and they are **not**
interchangeable.

**1. `Builder`'s `TB::` functions** (`src/include/tavoos/builder.h`):

```cpp
class TAVOOS_EXPORT Builder {
    friend class Window;

public:
    static void Rectangle(std::function<void(RectangleWidget&)> body) { create<RectangleWidget>(std::move(body)); }
    // ... Row, Column, Grid, Flex, Image, SVG, Text follow the same pattern

private:
    static Object* currentItem;

    template<typename T>
    static void create(std::function<void(T&)> body) {
        const auto parentObject = currentItem;
        std::unique_ptr<Object> widget = std::make_unique<T>(parentObject);
        currentItem = widget.get();
        body(static_cast<T&>(*widget));
        parentObject->appendChild(widget);
        currentItem = parentObject;
    }
};
```

`create<T>()` constructs `T` with whatever `currentItem` currently is as its parent, points
`currentItem` at the new widget *before* running `body` (so nested `TB::` calls inside your
lambda pick up the right parent), then appends the finished widget to the original parent and
restores `currentItem`. This is only valid while `currentItem` is non-null, which is only
true during the synchronous `Window::build()` call - `Window::setup()`
(`src/window.cpp`) does:

```cpp
Builder::currentItem = this;   // this = the Window
Widget::s_isBuilding = true;
build();                        // your override runs here, TB:: calls all see a valid parent
Widget::s_isBuilding = false;
Builder::currentItem = nullptr; // dereferencing this later crashes
```

Calling `TB::Rectangle(...)` later (e.g. from an `onClick` handler) dereferences a null
`parentObject` and crashes.

**2. `Widget::addChild<T>()`** (`widget.h`):

```cpp
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
```

This has no dependency on `Builder::currentItem` at all - it's the only safe way to grow the
tree dynamically, at runtime, after `build()` has already returned. If a dynamically-added
widget itself needs children, nest more `addChild<T>()` calls, not `TB::` ones - they'll
silently do nothing useful (`currentItem` is `nullptr`, so `TB::create<T>` dereferences it).

### Removing widgets: two-phase deferred destruction

`Widget::removeSelf()`/`removeChild()` don't destroy anything immediately:

```cpp
void Widget::detachAndDefer(Widget* child) {
    Object* const parentObj = child->parent();
    if (!parentObj)
        return;

    std::unique_ptr<Object> detached = parentObj->detachChild(child);
    if (!detached)
        return;

    if (child->m_ownerWindow) {
        child->m_ownerWindow->clearReferencesTo(child);
        child->m_ownerWindow->markDirty();
        child->m_ownerWindow->deferDestruction(std::move(detached));
    }

    if (auto* const parentWidget = dynamic_cast<Widget*>(parentObj); parentWidget && parentWidget->isLayouter())
        parentWidget->requestRelayout();
}

void Widget::removeSelf() { detachAndDefer(this); }
```

```cpp
// window.h
void deferDestruction(std::unique_ptr<Object> widget);
// window.cpp
void Window::deferDestruction(std::unique_ptr<Object> widget) {
    m_pendingDestruction.push_back(std::move(widget));
}
void Window::flushPendingDestruction() {
    m_pendingDestruction.clear();   // actual destructors run here
}
```

`flushPendingDestruction()` is called once, at the very start of
[`Renderer::renderForWindow`](#per-frame-sequence) - i.e. on the *next* frame after removal,
never mid-dispatch. This two-phase detach-then-destroy exists so a widget can safely remove
*itself* (or an ancestor of itself) from inside its own event handler: the object stays alive
in memory until the current event dispatch (and the current frame) is fully done, so nothing
downstream ends up holding a pointer into freed memory mid-call.

`Window::clearReferencesTo` is the other half of making this safe - it walks the removed
subtree's *potential observers* (not the subtree itself) and nulls any of `Window`'s own
cached pointers that point into it:

```cpp
void Window::clearReferencesTo(Widget* subtreeRoot) {
    auto isOrDescendantOf = [subtreeRoot](Widget* w) {
        while (w) {
            if (w == subtreeRoot)
                return true;
            w = dynamic_cast<Widget*>(w->parent());
        }
        return false;
    };

    if (isOrDescendantOf(m_hoveredWidget))   m_hoveredWidget = nullptr;
    if (isOrDescendantOf(m_pressedWidget))   m_pressedWidget = nullptr;
    if (isOrDescendantOf(m_lastClickWidget)) m_lastClickWidget = nullptr;
    if (isOrDescendantOf(m_focusedWidget))   setFocusedWidget(nullptr);
}
```

Note the general shape this implies for any code that caches a `Widget*` in `Window`: since
an event handler can remove the very widget an event is being dispatched to (or an ancestor
of it), any write of a `Widget*` into `Window`'s own state *after* dispatching to it should
first check that the widget wasn't just invalidated - `clearReferencesTo`'s nulling is the
signal to check against, not new tracking.

## Reactive properties

`src/include/tavoos/reactive/`.

### `Property<T>`

What every widget field actually is (`property.h`):

```cpp
template <typename T>
class Property {
    friend class State<T>;

public:
    Property(const T& value) : m_value{value} {}
    Property(State<T>& state) {
        state.registerObserver(this);
        m_isBind = true;
        m_state = &state;
    }
    ~Property() { unbind(); }

    void bind(State<T>& state) {
        unbind();
        state.registerObserver(this);
        m_isBind = true;
        m_state = &state;
        notifyChange();
    }

    void unbind(bool saveValue = false) {
        if (m_isBind) {
            if (saveValue)
                m_value = m_state->get();
            m_isBind = false;
            m_state->unregisterObserver(this);
            m_state = nullptr;
        }
    }

    void onChange(std::function<void(const T&)> callback) {
        m_onChangeCallbacks.push_back(std::move(callback));
    }

    void set(const T& value) {
        m_value = value;
        unbind();
        notifyChange();
    }

    const T& get() const noexcept { return m_isBind ? m_state->get() : m_value; }
    operator const T&() const noexcept { return get(); }

private:
    T m_value{};
    State<T>* m_state{nullptr};
    bool m_isBind{false};
    std::vector<std::function<void(const T&)>> m_onChangeCallbacks;

    void notifyChange() {
        for (const auto& callback : m_onChangeCallbacks)
            callback(m_isBind ? m_state->get() : m_value);
    }
};
```

Either owns a plain `T` value, or is bound (`m_isBind`) to a `State<T>*`. `get()`/the
implicit `operator const T&()` resolve through the bound state if bound, else the local
value. Calling `set()` on an already-bound property unbinds it first - a plain value always
wins over a stale binding. `notifyChange()` is `private`, friended to `State<T>` only - it's
not meant to be called by anyone except the `State` this property is bound to.

### `State<T>`

A shared value with a list of observing `Property<T>*`s (`state.h`):

```cpp
template <typename T>
class State {
    friend class Property<T>;

public:
    State(const T& value) : m_value{value} {}
    ~State() {
        auto observers = std::move(m_observers);
        for (const auto observer : observers)
            observer->unbind(true);   // saveValue=true: they keep the last value they had
    }

    void notifyObservers() {
        const auto observersSnapshot = m_observers;
        for (const auto observer : observersSnapshot) {
            if (std::find(m_observers.begin(), m_observers.end(), observer) != m_observers.end())
                observer->notifyChange();
        }
    }

    void set(const T& value) {
        m_value = value;
        notifyObservers();
    }

    const T& get() const noexcept { return m_value; }

private:
    T m_value{};
    std::vector<Property<T>*> m_observers;

    void registerObserver(Property<T>* observer)   { m_observers.push_back(observer); }
    void unregisterObserver(Property<T>* observer) { std::erase(m_observers, observer); }
};
```

`notifyObservers()` snapshots the observer list *before* iterating, then checks each
snapshotted pointer is still present in the *live* list before calling it. This matters
because an observer's own `onChange` callback can legally unbind itself (or another
observer) mid-notify - `Property::unbind()`, called from such a callback, removes the
property from `m_observers` via `unregisterObserver`, which would otherwise be mutating the
list while it's being iterated.

### `PropertyArg<T>`

The parameter type every fluent setter actually takes (`propertyarg.h`):

```cpp
template <typename T>
class PropertyArg {
public:
    PropertyArg(const T& value) : m_value{value} {}
    PropertyArg(T&& value) : m_value{std::move(value)} {}
    PropertyArg(State<T>& state) : m_state{&state}, m_isBind{true} {}
    template <typename U>
        requires std::convertible_to<U, T> && (!std::derived_from<std::remove_cvref_t<U>, State<T>>)
    PropertyArg(U&& value) : m_value(std::forward<U>(value)) {}

    void applyTo(Property<T>& property) {
        if (m_isBind)
            property.bind(*m_state);
        else
            property.set(m_value);
    }

private:
    T m_value{};
    State<T>* m_state{nullptr};
    bool m_isBind{false};
};
```

This is *how* every widget setter (`.width(200)` or `.width(someState)`) can accept either a
plain value or a live binding with the same call syntax. Note the fourth constructor's
constraint: it converts from anything convertible to `T` *except* something derived from
`State<T>` - without that exclusion, a `State<T>&` argument would be ambiguous between the
exact `PropertyArg(State<T>&)` overload and the generic converting one. This is also why a
setter typed `PropertyArg<Paint>` won't accept a `State<Color>&` even though a plain `Color`
converts to `Paint` implicitly - the `State<T>` constructor requires an *exact* `T` match, it
doesn't go through `Paint`'s converting constructor the way a plain value would.

`Property`/`State` friend each other in both directions (`property.h`:
`friend class State<T>;`, `state.h`: `friend class Property<T>;`) because C++ friendship
isn't transitive or bidirectional - each class needs its own `friend` declaration to let the
*other* reach into its private members.

### `SidedProperty<T>` / `CornerProperty<T>`

Four independent `Property<T>`s each (`sidedproperty.h`/`cornerproperty.h`), used for
padding/margin (left/top/right/bottom) and per-corner radius (top-left/top-right/
bottom-right/bottom-left). A uniform setter like `Widget::padding(v)` just calls `.applyTo()`
on all four independently:

```cpp
decltype(auto) padding(this auto&& self, PropertyArg<float> value) {
    value.applyTo(self.m_padding.leftProperty());
    value.applyTo(self.m_padding.topProperty());
    value.applyTo(self.m_padding.rightProperty());
    value.applyTo(self.m_padding.bottomProperty());
    return std::forward<decltype(self)>(self);
}
```

`Widget`'s constructor wires most of its geometry-affecting properties into a relayout
trigger, and purely visual ones into a repaint trigger, via two small generic helpers:

```cpp
template<typename... Props>
void bindRelayoutTriggers(Props&... props) {
    auto relayout = [this](const auto&) { requestRelayout(); };
    (props.onChange(relayout), ...);
}
template<typename... Props>
void bindRepaintTriggers(Props&... props) {
    auto repaint = [this](const auto&) { requestRepaint(); };
    (props.onChange(repaint), ...);
}
```

```cpp
// Widget::Widget(Object* parent)
bindRelayoutTriggers(m_x, m_y, m_width, m_height, m_alignment, m_fill,
                     m_padding.leftProperty(), m_padding.topProperty(), /* ... */
                     m_rotation, m_scale, m_gridColumnSpan, m_gridRowSpan, m_gridRow, m_gridColumn);
m_opacity.onChange([this](const auto&) { updateEffectiveOpacity(); });
bindRepaintTriggers(m_opacity, m_visible, m_clip);
```

## Layout system

### Dirty-flag propagation

Two things drive when layout actually runs. First, `Widget::m_layoutDirty`:

```cpp
void Widget::markLayoutDirty() { m_layoutDirty = true; requestRepaint(); }
bool Widget::consumeLayoutDirty() {
    if (m_layoutDirty) { m_layoutDirty = false; return true; }
    return false;
}
```

Every `layout()` override starts the same way:

```cpp
void ColumnWidget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);   // not dirty myself, but a descendant might be
        return;
    }
    /* ... actually recompute this widget's own geometry and place children ... */
}
```

So a `layout()` call is cheap to make on every widget every frame - if nothing's dirty, it's
just a tree walk checking each descendant's own flag, no math.

Second, `Widget::requestRelayout()` decides *how far up the tree* a property change needs to
propagate:

```cpp
void Widget::requestRelayout() {
    if (s_isBuilding)   // no-op during Window::build() - nothing needs walking mid-construction
        return;

    markLayoutDirty();
    m_intrinsicSizeCacheValid = false;

    auto* const parentWidget = dynamic_cast<Widget*>(parent());
    if (!parentWidget || !parentWidget->isLayouter())
        return;   // parent isn't a container - it doesn't need to re-place me

    if (parentWidget->isSizeBoundary())
        parentWidget->markLayoutDirty();     // parent's own size is unaffected - only re-place children
    else
        parentWidget->requestRelayout();     // parent's size may depend on me - keep walking up
}
```

```cpp
bool Widget::isSizeBoundary() const {
    const bool widthFixed  = hasFlag(m_fill.get(), Fill::Width)  || m_width.get()  > 0;
    const bool heightFixed = hasFlag(m_fill.get(), Fill::Height) || m_height.get() > 0;
    return widthFixed && heightFixed;
}
```

A "size boundary" is a widget whose own resolved size doesn't depend on its children (it's
either `fill()`-driven from its parent, or has an explicit `width()`/`height()`) - so a
child's size change can't ripple through it, and the walk stops there. If the container's own
size is intrinsic (sized to its content), the change genuinely could grow/shrink the
container itself, so the walk has to continue upward.

`resolveContentArea()` gives a widget the box it has to lay out within:

```cpp
Widget::ContentArea Widget::resolveContentArea() {
    ContentArea area{0, 0, 0, 0};
    if (auto* const parentWidget = dynamic_cast<Widget*>(parent())) {
        area.x = parentWidget->paddingLeft();
        area.y = parentWidget->paddingTop();
        area.width  = parentWidget->displayedWidth()  - parentWidget->paddingLeft() - parentWidget->paddingRight();
        area.height = parentWidget->displayedHeight() - parentWidget->paddingTop()  - parentWidget->paddingBottom();
    } else if (auto* const window = dynamic_cast<Window*>(parent())) {
        area.width  = static_cast<float>(window->width());
        area.height = static_cast<float>(window->height());
    }
    return area;
}
```

Either the parent widget's padded interior, or (if the `Object` parent is a `Window`
directly) the full window size.

### `Widget::layout()` - the non-layouter default

Every concrete widget except the four containers uses this base implementation as-is
(`widget.cpp`):

```cpp
void Widget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    if (parentIsLayouter()) {
        // A container already computed my resolved geometry directly via setResolved() -
        // I just need to propagate the world matrix and recurse into my own children.
        syncDisplayedGeometry();
        updateWorldMatrix();
        for (auto& child : children())
            if (auto* widget = dynamic_cast<Widget*>(child.get()))
                widget->layout(true);
        return;
    }

    const ContentArea area = resolveContentArea();
    const Size natural = intrinsicSize();

    const float effX = area.x + marginLeft();
    const float effY = area.y + marginTop();
    const float effWidth  = area.width  - marginLeft() - marginRight();
    const float effHeight = area.height - marginTop()  - marginBottom();

    m_resolvedWidth = hasFlag(m_fill.get(), Fill::Width)
        ? effWidth : (m_width.get() > 0) ? static_cast<float>(m_width.get()) : natural.width;
    m_resolvedHeight = hasFlag(m_fill.get(), Fill::Height)
        ? effHeight : (m_height.get() > 0) ? static_cast<float>(m_height.get()) : natural.height;

    const Alignment align = m_alignment.get();
    if (hasFlag(align, Alignment::Left))                  m_resolvedX = effX;
    else if (hasFlag(align, Alignment::Right))             m_resolvedX = effX + effWidth - m_resolvedWidth;
    else if (hasFlag(align, Alignment::CenterHorizontal))  m_resolvedX = effX + (effWidth - m_resolvedWidth) * 0.5f;
    else                                                    m_resolvedX = effX + static_cast<float>(m_x.get());
    // ... same four-way branch for Y / Top / Bottom / CenterVertical / m_y.get()

    syncDisplayedGeometry();
    updateWorldMatrix();

    for (auto& child : children())
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout(true);
}
```

Two branches worth noticing: `parentIsLayouter()` short-circuits entirely - if a container
parent already called `item->setResolved(...)` on this widget (all four containers do this
for every child they place), there's nothing left to compute; this widget just needs to push
its world matrix down and recurse. The `else` branch is the "top-level or non-container
parent" case: resolve size from `fill()`/explicit size/intrinsic size, then position from
`alignment()` flags or explicit `x()`/`y()`.

`intrinsicSize()` caches `computeIntrinsicSize()`:

```cpp
Widget::Size Widget::intrinsicSize() {
    if (!m_intrinsicSizeCacheValid) {
        m_cachedIntrinsicSize = computeIntrinsicSize();
        m_intrinsicSizeCacheValid = true;
    }
    return m_cachedIntrinsicSize;
}
Widget::Size Widget::computeIntrinsicSize() {   // base default: no natural size beyond explicit
    return { static_cast<float>(width()), static_cast<float>(height()) };
}
```

`requestRelayout()` invalidates the cache (`m_intrinsicSizeCacheValid = false`) alongside
marking layout dirty, so a property change that could affect a widget's natural size (its own
`width()`/`height()`, or for a container, a child's size) always gets re-measured.

### Column & Row

Column and Row (`src/widget/column.cpp`, `row.cpp`) are near-mirror images of each other -
Column stacks on the vertical axis, Row on the horizontal - so the full walkthrough below is
for Column; Row's code is included for completeness with every axis swapped.

**`ColumnWidget::layout()`**, in full:

```cpp
void ColumnWidget::layout(bool force) {
    const bool wasDirty = consumeLayoutDirty();
    if (!wasDirty && !force) {
        for (auto& child : children())
            if (auto* w = dynamic_cast<Widget*>(child.get()))
                w->layout(false);
        return;
    }

    std::vector<Widget*> items;
    for (auto& child : children())
        if (auto* w = dynamic_cast<Widget*>(child.get()))
            items.push_back(w);

    std::vector<Size> itemSizes;
    for (auto* item : items)
        itemSizes.push_back(item->intrinsicSize());

    // Pass 1: how much fixed (non-fill) height do all items need, and what's the widest
    // non-fill-width item (for sizing this Column itself, if it isn't fill/explicit)?
    float fixedHeight = 0.0f;
    int fillCount = 0;
    float maxChildWidth = 0.0f;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const float vMargin = items[i]->marginTop() + items[i]->marginBottom();
        const float hMargin = items[i]->marginLeft() + items[i]->marginRight();
        if (hasFlag(items[i]->fill(), Fill::Height)) {
            ++fillCount;
            fixedHeight += vMargin;               // fill items still contribute their margin
        } else {
            fixedHeight += itemSizes[i].height + vMargin;
        }
        if (!hasFlag(items[i]->fill(), Fill::Width))
            maxChildWidth = std::max(maxChildWidth, itemSizes[i].width + hMargin);
    }
    if (items.size() > 1)
        fixedHeight += m_spacing * static_cast<float>(items.size() - 1);

    // Resolve this Column's own geometry (fill / explicit size / hug-content), then position
    // it within its own parent's area via alignment or x()/y() - identical four-way branch
    // to Widget::layout()'s else-branch above, just computed here instead of inherited,
    // because Column needs fixedHeight/maxChildWidth (its own children's sizes) as the
    // "natural size" fallback instead of a generic computeIntrinsicSize() call.
    float resolvedW, resolvedH, resX, resY;
    if (parentIsLayouter()) {
        resolvedW = resolvedWidth();  resolvedH = resolvedHeight();
        resX = resolvedX();           resY = resolvedY();
    } else {
        const ContentArea area = resolveContentArea();
        resolvedH = hasFlag(fill(), Fill::Height) ? area.height - marginTop() - marginBottom()
                    : (height() > 0) ? static_cast<float>(height()) : fixedHeight + paddingTop() + paddingBottom();
        resolvedW = hasFlag(fill(), Fill::Width)  ? area.width - marginLeft() - marginRight()
                    : (width()  > 0) ? static_cast<float>(width())  : maxChildWidth + paddingLeft() + paddingRight();
        // ... alignment-based resX/resY, same four-way branch as Widget::layout()
    }

    setResolved(resX, resY, resolvedW, resolvedH);
    syncDisplayedGeometry();
    updateWorldMatrix();

    // Pass 2: divide leftover vertical space among fill items, then place everything with a
    // cursor walking down.
    const float innerW = resolvedW - paddingLeft() - paddingRight();
    const float innerH = resolvedH - paddingTop()  - paddingBottom();
    const float leftover = std::max(0.0f, innerH - fixedHeight);
    const float fillShare = (fillCount > 0) ? leftover / static_cast<float>(fillCount) : 0.0f;

    float cursorY = paddingTop();
    for (std::size_t i = 0; i < items.size(); ++i) {
        Widget* const item = items[i];
        const bool fillsWidth  = hasFlag(item->fill(), Fill::Width);
        const bool fillsHeight = hasFlag(item->fill(), Fill::Height);
        const float itemMarginTop = item->marginTop(), itemMarginBottom = item->marginBottom();
        const float itemMarginLeft = item->marginLeft(), itemMarginRight = item->marginRight();

        const float childH = fillsHeight ? fillShare : itemSizes[i].height;
        const float childW = fillsWidth  ? (innerW - itemMarginLeft - itemMarginRight) : itemSizes[i].width;

        // Cross-axis (horizontal) position: fill stretches full width; otherwise the item's
        // OWN alignment() flags position it left/right/center within innerW.
        const Alignment childAlign = item->alignment();
        float childX;
        if (fillsWidth)                                            childX = paddingLeft() + itemMarginLeft;
        else if (hasFlag(childAlign, Alignment::Right))             childX = paddingLeft() + innerW - itemMarginRight - childW;
        else if (hasFlag(childAlign, Alignment::CenterHorizontal))  childX = paddingLeft() + itemMarginLeft + (innerW - itemMarginLeft - itemMarginRight - childW) * 0.5f;
        else                                                         childX = paddingLeft() + itemMarginLeft + static_cast<float>(item->x());

        item->setResolved(childX, cursorY + itemMarginTop, childW, childH);
        cursorY += itemMarginTop + childH + itemMarginBottom + m_spacing;
        item->layout(true);   // recurse - force=true since we just changed this item's geometry
    }
}
```

**Worked example.** A `ColumnWidget` with `fill(Fill::Both)` inside a 400px-tall parent,
`spacing(10)`, three children: A (`height(50)`), B (`fill(Fill::Height)`), C (`height(30)`).

- Pass 1: `fixedHeight = 50 + 30 + spacing*(3-1) = 80 + 20 = 100`. `fillCount = 1` (B).
- `resolvedH = 400` (from `Fill::Height`, no `fixedHeight` fallback needed).
- `leftover = 400 - 100 = 300`; `fillShare = 300 / 1 = 300`.
- Placement: A at `cursorY=0`, height 50, `cursorY` advances to `50+10=60`. B at `cursorY=60`,
  height `fillShare=300`, `cursorY` advances to `60+300+10=370`. C at `cursorY=370`, height 30.
  Total consumed: `370+30=400` - exactly fills the column, B absorbed all the leftover space.

**`computeIntrinsicSize()`** - used when this Column itself is a plain (non-fill,
non-explicit-size) child of something else, so *its* natural size needs to be known before
that something else can place it:

```cpp
Widget::Size ColumnWidget::computeIntrinsicSize() {
    if (width() > 0 && height() > 0)
        return { static_cast<float>(width()), static_cast<float>(height()) };

    float totalH = 0.0f, maxW = 0.0f;
    for (auto* item : /* children as Widget* */) {
        const Size sz = item->intrinsicSize();
        totalH += sz.height + item->marginTop() + item->marginBottom();
        maxW = std::max(maxW, sz.width + item->marginLeft() + item->marginRight());
    }
    if (/* more than one item */)
        totalH += m_spacing * static_cast<float>(/* count */ - 1);

    return { (width() > 0) ? static_cast<float>(width()) : maxW + paddingLeft() + paddingRight(),
            (height() > 0) ? static_cast<float>(height()) : totalH + paddingTop() + paddingBottom() };
}
```

Sums children's heights (+ spacing), takes the max of their widths - the "hug content"
natural size. Note this ignores `Fill`-flagged children's *actual* fill behavior (a fill
child still just contributes its `intrinsicSize()` here, same as `fixedHeight`'s accounting
in `layout()` above) - intrinsic size is about what the column would need with no imposed
constraint, not what it does once one is imposed.

**Row** is the exact same two algorithms with width/height, x/y, and Left-Right/Top-Bottom
swapped:

```cpp
void RowWidget::layout(bool force) {
    // ... identical structure to Column, with:
    //   fixedWidth  instead of fixedHeight   (main axis = horizontal)
    //   maxChildHeight instead of maxChildWidth  (cross axis = vertical)
    //   cursorX walking right instead of cursorY walking down
    //   childY positioned via Alignment::Bottom/CenterVertical instead of Right/CenterHorizontal
}
```

### Grid

`src/widget/grid.cpp`. Grid's algorithm splits into two phases: `computeMetrics()` figures
out placement and cell sizes without touching final pixel positions; `layout()` uses that to
resolve this Grid's own geometry and then place every item.

**Placement** (`computeMetrics()`, placement half):

```cpp
GridWidget::GridMetrics GridWidget::computeMetrics() const {
    GridMetrics metrics;
    const int cols = std::max(1, m_columns.get());

    std::vector<std::vector<bool>> occupied;   // occupied[row][col]
    auto isFree = [&](int row, int col) {
        return row >= static_cast<int>(occupied.size()) || !occupied[row][col];
    };
    auto fits = [&](int row, int col, int rowSpan, int colSpan) {
        if (col + colSpan > cols) return false;
        for (int r = row; r < row + rowSpan; ++r)
            for (int c = col; c < col + colSpan; ++c)
                if (!isFree(r, c)) return false;
        return true;
    };
    auto occupy = [&](int row, int col, int rowSpan, int colSpan) {
        while (static_cast<int>(occupied.size()) < row + rowSpan)
            occupied.emplace_back(cols, false);
        for (int r = row; r < row + rowSpan; ++r)
            for (int c = col; c < col + colSpan; ++c)
                occupied[r][c] = true;
    };

    // Phase A: place every item with BOTH gridRow() and gridColumn() explicitly set, exactly
    // where asked (clamped to fit within `cols`), no collision checking against each other -
    // an explicit placement always wins its cell.
    for (auto* item : items) {
        if (item->gridRow() < 0 || item->gridColumn() < 0)
            continue;
        const int colSpan = std::clamp(item->gridColumnSpan(), 1, cols);
        const int rowSpan = std::max(1, item->gridRowSpan());
        const int col = std::clamp(item->gridColumn(), 0, cols - colSpan);
        const int row = item->gridRow();
        occupy(row, col, rowSpan, colSpan);
        metrics.placements.push_back({item, row, col, rowSpan, colSpan});
    }

    // Phase B: everything else - fully auto, row-only, or column-only.
    int cursorRow = 0, cursorCol = 0;
    for (auto* item : items) {
        const bool rowSet = item->gridRow() >= 0, colSet = item->gridColumn() >= 0;
        if (rowSet && colSet) continue;   // handled in Phase A

        const int colSpan = std::clamp(item->gridColumnSpan(), 1, cols);
        const int rowSpan = std::max(1, item->gridRowSpan());
        int row, col;

        if (colSet) {
            // Column pinned, row auto: scan downward for the first row this span fits in.
            col = std::clamp(item->gridColumn(), 0, cols - colSpan);
            row = 0;
            while (!fits(row, col, rowSpan, colSpan)) ++row;
        } else if (rowSet) {
            // Row pinned, column auto: scan across that row; if nothing fits, advance to the
            // next row and scan again.
            row = item->gridRow();
            col = 0;
            while (true) {
                while (col + colSpan <= cols && !fits(row, col, rowSpan, colSpan)) ++col;
                if (col + colSpan <= cols) break;
                col = 0; ++row;
            }
        } else {
            // Fully auto: row-major scan starting from wherever the last auto item left off.
            row = cursorRow; col = cursorCol;
            while (true) {
                if (col + colSpan > cols) { col = 0; ++row; continue; }
                if (fits(row, col, rowSpan, colSpan)) break;
                ++col;
            }
            cursorRow = row;
            cursorCol = col + colSpan;
            if (cursorCol >= cols) { cursorCol = 0; ++cursorRow; }
        }

        occupy(row, col, rowSpan, colSpan);
        metrics.placements.push_back({item, row, col, rowSpan, colSpan});
    }
    /* ... sizing half follows below ... */
}
```

**Worked example: auto-flow with a spanning item.** `columns(3)`, five children in document
order: A (`gridColumnSpan(2)`, otherwise auto), B, C, D, E (all fully auto).

- A: fully auto, `cursorRow=0, cursorCol=0`. `colSpan=2` fits at `(0,0)`. Occupies `(0,0)` and
  `(0,1)`. `cursorCol = 0+2 = 2`, `cursorRow` stays `0`.
- B: fully auto, starts at `(0,2)`. Fits (span 1). Occupies `(0,2)`. `cursorCol = 3 >= cols`,
  so wraps: `cursorCol=0, cursorRow=1`.
- C: starts at `(1,0)`. Fits. Occupies `(1,0)`. `cursorCol=1`.
- D: starts at `(1,1)`. Fits. Occupies `(1,1)`. `cursorCol=2`.
- E: starts at `(1,2)`. Fits. Occupies `(1,2)`. `cursorCol` wraps to `(2,0)` for the next item.

Result: row 0 is `[A, A, B]`, row 1 is `[C, D, E]` - a 2-column-span item correctly pushes the
next auto item to the remaining single cell in its row, then wraps.

**Sizing** (second half of `computeMetrics()`): column widths/row heights start as the
largest *single-span* item claiming that row/column; multi-span items then grow every
column/row they touch equally, *only if* their own size doesn't already fit in the span's
current combined size:

```cpp
// natural per-column/row size from single-span items
for (auto& p : metrics.placements) {
    if (p.colSpan == 1) metrics.columnWidths[p.col] = std::max(metrics.columnWidths[p.col], itemW + margins);
    if (p.rowSpan == 1) metrics.rowHeights[p.row]   = std::max(metrics.rowHeights[p.row],   itemH + margins);
}
// grow spanned columns/rows if a spanning item doesn't fit in their combined natural size
for (auto& p : metrics.placements) {
    if (p.colSpan > 1) {
        float span = /* sum of columnWidths[p.col .. p.col+colSpan) + spacing */;
        const float deficit = (itemW + margins) - span;
        if (deficit > 0.0f) {
            const float extra = deficit / static_cast<float>(p.colSpan);
            for (int c = p.col; c < p.col + p.colSpan; ++c) metrics.columnWidths[c] += extra;
        }
    }
    // same for rowSpan
}
```

**`layout()`**: resolves this Grid's own geometry (fill/explicit/natural-sum, same pattern as
Column/Row), then distributes any *extra* space beyond the natural column/row sizes evenly
across all columns/rows (so a Grid with `fill(Fill::Both)` in a larger area doesn't leave a
big empty margin - every column/row grows to share the extra), computes absolute `colX`/`rowY`
offset tables via a running sum, then places each item within its (possibly multi-cell) cell
bounds using the exact same fill-stretch-or-alignment logic as Column/Row's cross-axis
placement, just on both axes independently since a grid cell has two free dimensions instead
of one.

### Flex

`src/widget/flex.cpp`. Supports direction, wrap, justify, and align - not grow/shrink/basis/
order. Two axis-generic lambdas (`mainOf`/`crossOf`, and their margin equivalents) let the
same code handle both `FlexDirection::Row` and `::Column` without duplicating the algorithm.

**Line-breaking** (`computeMetrics(availableMainSpace)`):

```cpp
FlexLine current;
float currentMain = 0.0f;
for (auto* item : items) {
    const Size sz = item->intrinsicSize();
    const float itemMain = mainOf(sz) + mainMarginOf(item);
    float addGap = current.items.empty() ? 0.0f : mainGap;

    if (wrapping && !current.items.empty() && currentMain + addGap + itemMain > availableMainSpace) {
        metrics.lines.push_back(std::move(current));   // this item doesn't fit - start a new line
        current = FlexLine{};
        currentMain = 0.0f;
        addGap = 0.0f;
    }

    current.items.push_back(item);
    current.itemSizes.push_back(sz);
    currentMain += addGap + itemMain;
    current.crossSize = std::max(current.crossSize, crossOf(sz) + crossMarginOf(item));
}
if (!current.items.empty())
    metrics.lines.push_back(std::move(current));
```

Greedy packing: an item joins the current line unless it would overflow `availableMainSpace`
*and* wrapping is enabled *and* the line already has at least one item (a line is never
empty-then-overflow - the first item on a line always fits, even if it alone exceeds the
available space). `kUnbounded` (`1e9f`) is passed as `availableMainSpace` when *measuring*
natural size (`computeIntrinsicSize()`) specifically so nothing wraps during that pass - a
single unbounded line gives the true "everything laid out in one row/column" natural size.

**Cross-axis line distribution** (`layout()`, when there's more than one line):
`alignContent` (`FlexAlign::Center`/`End`/`SpaceBetween`/`SpaceAround`/`SpaceEvenly`/
`Stretch`) distributes the wrapped lines themselves across the cross axis, exactly mirroring
how `justifyContent` distributes *items within a line* along the main axis (same
offset/leftover/between-gap math, just computed once for lines instead of once per line for
items - see the source for the full `switch` in both cases, they're structurally identical).

**Per-item placement within a line**: an item's cross-axis alignment resolves from its own
`alignment()` if set, else falls back to the Flex container's `alignItems()`:

```cpp
const Alignment itemAlign = item->alignment();
const Alignment effectiveAlign = (itemAlign == Alignment::None) ? m_alignItems.get() : itemAlign;
```

then positions within `thisLineCross` (the line's own cross-axis size, possibly stretched by
`alignContent` above) the same way Column/Row position a non-fill child within their
cross-axis: leading edge, trailing edge, or centered, with margins subtracted from the
available span either way.

## Rendering pipeline

`Renderer` (`src/include/tavoos/gfx/renderer.h`, `src/gfx/renderer.cpp`) owns all GL state.
It's a private-constructor singleton-per-`Application`, held behind `std::unique_ptr<Renderer>`
in `Application` (PIMPL'd specifically so `application.h` doesn't have to `#include` the GL
loader header).

### Per-frame sequence

```cpp
void Renderer::renderForWindow(Window& window) {
    if (!window.consumeDirty())
        return;

    glfwMakeContextCurrent(window.handle());
    window.flushPendingDestruction();
    ensureWindowResources(window);

    glViewport(0, 0, window.framebufferWidth(), window.framebufferHeight());
    projection = glm::ortho(0.0f, static_cast<float>(window.width()),
                            static_cast<float>(window.height()), 0.0f, -1.0f, 1.0f);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection);

    Color backgroundColor = window.color();
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& child : window.children())
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout();

    for (const auto& child : window.children())
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            if (widget->visible())
                renderWidget(*widget);
}
```

In order: skip entirely if `consumeDirty()` says nothing changed; destroy anything
detached-and-deferred by an event handler in a *previous* frame (see
[Building the tree](#building-the-tree)); lazily create this window's own GL resources
(`ensureWindowResources` - each window has independent VAOs/FBO, tracked in
`m_windowResources`, since multiple windows can share one GL context); set up the projection
from logical window size (not physical framebuffer size - see `HiDPI` note below); clear;
run the [dirty-flag-driven layout pass](#dirty-flag-propagation) on every top-level child;
render every visible one.

Note the ortho projection uses `window.width()`/`height()` (logical units) while
`glViewport` uses `window.framebufferWidth()`/`framebufferHeight()` (physical pixels) - this
is the entire HiDPI story: layout and all widget geometry happen in logical units throughout,
and the GPU maps that logical-unit orthographic projection onto however many physical pixels
the framebuffer actually has.

Each concrete widget's `render()` calls the matching `Renderer::render*()` method
(`renderRectangle`, `renderImage`, `renderSVG`, `renderText`) with its own shader/uniforms;
`Widget::renderChildren()` (not `Renderer`) recurses into children, in `z()` order:

```cpp
void Widget::renderChildren(Renderer& renderer) {
    if (renderer.isCapturingMask())
        return;   // see Clipping below - children must not render during mask capture
    for (Widget* const widget : zOrderedChildren(*this))
        if (widget->visible())
            renderer.renderWidget(*widget);
}
```

`zOrderedChildren()` (a private static `Widget` helper, shared with `Renderer::
renderForWindow`'s window-level loop and `Window::hitTestChildren`) collects a parent's
`Widget*` children and `std::stable_sort`s them ascending by `z()` - equal-z widgets keep
their document order, so this is a no-op for any tree that never sets `z()`. Rendering walks
the sorted list forward (lowest z first, painted underneath); hit-testing (below) walks it in
reverse (highest z checked first, so it wins an overlap).

### Clipping

`Widget::clip(true)` routes through `Renderer::renderWidget` differently:

```cpp
void Renderer::renderWidget(Widget& widget) {
    if (!widget.clip()) {
        widget.render(*this);
        return;
    }

    const int texW = static_cast<int>(std::ceil(std::max(widget.displayedWidth(), 0.0f)));
    const int texH = static_cast<int>(std::ceil(std::max(widget.displayedHeight(), 0.0f)));
    if (texW <= 0 || texH <= 0)
        return;

    ensureClipMask(widget, texW, texH);
    renderClipLayer(widget, texW, texH);
    compositeClipLayer(widget, texW, texH);
}
```

A three-step offscreen composite, each step rendering into a texture sized to the widget's
own displayed bounds via the shared `clipFBO`:

**1. `ensureClipMask`** - builds an alpha mask of the widget's *shape* (not its color) into
an `R8` texture:

```cpp
void Renderer::ensureClipMask(Widget& widget, int texW, int texH) {
    /* lazily (re)allocate widget.m_clipMaskTexture at texW x texH, GL_R8 */

    const SavedRenderTarget saved = beginClipTarget(clipFBO, cameraUBO, projection,
                                                     widget.m_clipMaskTexture, texW, texH, widget.worldMatrix());
    ScopeExit restoreTarget{[&] { endClipTarget(saved, clipFBO, cameraUBO, projection); }};

    // A layouter (Column/Row/Grid/Flex) has no shape of its own to draw as a mask - clear to
    // fully opaque so its clip is effectively "the full rect," letting children define the
    // actual visible shape. A shaped widget (e.g. RectangleWidget with rounded corners)
    // clears to fully transparent so only what it actually draws becomes opaque.
    glClearColor(widget.isLayouter() ? 1.0f : 0.0f, ..., widget.isLayouter() ? 1.0f : 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX);   // union: overlapping shapes' masks combine via max(), not blend
    const bool prevCapturingMask = m_capturingMask;
    m_capturingMask = true;    // renderChildren() no-ops while this is true
    ScopeExit restoreCaptureState{[&] {
        m_capturingMask = prevCapturingMask;   // NOT hard-coded false - see below
        glBlendEquation(static_cast<GLenum>(prevEquation));
    }};

    widget.render(*this);   // draws the widget's own shape (e.g. rounded rect) into the mask
}
```

**2. `renderClipLayer`** - renders the widget's *actual visible content* (including its
children, since `m_capturingMask` is `false` again by the time this runs) into a second,
`RGBA8` texture, using standard premultiplied-alpha-friendly blending.

**3. `compositeClipLayer`** - binds both textures (`uTexture` = color, `uMask` = mask) to a
composite shader and draws one screen-aligned quad, multiplying color by mask alpha, straight
into the real framebuffer at the widget's actual world-space position (with a pixel-snapping
step for axis-aligned, unit-scale widgets, `isAxisAlignedUnitScale`, to avoid subpixel
shimmer on the common case of an unrotated, unscaled clipped widget).

`m_capturingMask` is restored to its *previous* value on exit rather than hard-coded to
`false`, so nested clipped widgets (a clipped child inside a clipped parent) don't clear the
outer capture flag while it's still in progress.

## Animation system

`src/include/tavoos/animation/`.

### `AnimatableBase` / `AnimationManager`

```cpp
class TAVOOS_EXPORT AnimatableBase {
public:
    virtual ~AnimatableBase() = default;
private:
    friend class AnimationManager;
    virtual bool tick(float dt) = 0;
};

class TAVOOS_EXPORT AnimationManager {
    friend class Application;
public:
    static AnimationManager& instance();
    AnimationManager(const AnimationManager&) = delete;   // + move, + operator= (both)
    void registerAnimation(AnimatableBase* animation);
    void unregisterAnimation(AnimatableBase* animation);
    bool hasActiveAnimations() const noexcept { return !m_active.empty(); }
private:
    AnimationManager() = default;
    void tick(float dt);
    std::vector<AnimatableBase*> m_active;
};
```

`tick(float)` is a `private` pure virtual on `AnimatableBase`, friended only to
`AnimationManager`. Since access control for virtual calls is checked at the *call site*'s
static type, not the override, a subclass can freely implement `tick()` as private too - only
`AnimationManager` (via the friendship) can actually invoke it, regardless of the concrete
type.

`AnimationManager::tick(dt)` itself is `private`, friended only to `Application`, which calls
it once per frame:

```cpp
void AnimationManager::tick(float dt) {
    const auto activeSnapshot = m_active;
    std::vector<AnimatableBase*> finished;
    for (AnimatableBase* animation : activeSnapshot) {
        if (std::find(m_active.begin(), m_active.end(), animation) == m_active.end())
            continue;   // unregistered by an earlier callback in this same tick - skip it
        if (!animation->tick(dt))
            finished.push_back(animation);
    }
    for (AnimatableBase* animation : finished)
        unregisterAnimation(animation);
}
```

Same snapshot-plus-live-check reentrancy pattern as `State::notifyObservers()` - a completion
callback (fired from inside `tick()`) can legally start or cancel another animation, mutating
`m_active` mid-loop.

`registerAnimation`/`unregisterAnimation` are deliberately public - they're the intended
extension point for a consumer's own custom `AnimatableBase` subclass (a consumer can
subclass it, implement `tick()`, and register/unregister itself; see
[GUIDE.md](GUIDE.md#animation)). Locking `tick(dt)` down to `Application` only, while
leaving register/unregister public, is a deliberate asymmetry: a public `tick()` would let
any consumer arbitrarily fast-forward every active animation in the whole app at once, which
is a real correctness hazard; public register/unregister just lets a consumer plug their own
animation into the same per-frame loop everything else uses, which is the whole point of the
class existing as a public type.

### `AnimatedState<T>`

`State<T>` + `AnimatableBase` together (`animatedstate.h`):

```cpp
template<Interpolatable T>
class AnimatedState : public State<T>, public AnimatableBase {
public:
    void animateTo(const T& target, float durationSeconds, EasingFn easing = Easing::linear) {
        if (durationSeconds <= 0.0f) {
            if (m_animating) { m_animating = false; AnimationManager::instance().unregisterAnimation(this); }
            this->set(target);
            return;
        }
        m_from = this->get(); m_to = target;
        m_duration = durationSeconds; m_elapsed = 0.0f; m_easing = std::move(easing);
        if (!m_animating) { m_animating = true; AnimationManager::instance().registerAnimation(this); }
    }

private:
    bool tick(float dt) override {
        m_elapsed += dt;
        const float t = std::min(m_elapsed / m_duration, 1.0f);
        const float eased = m_easing ? m_easing(t) : t;
        this->set(lerp(m_from, m_to, eased));   // State::set() -> notifyObservers() -> every bound Property
        if (t >= 1.0f) { m_animating = false; return false; }
        return true;
    }
    T m_from{}, m_to{};
    float m_duration{0.0f}, m_elapsed{0.0f};
    bool m_animating{false};
    EasingFn m_easing{Easing::linear};
};
```

Requires `T` to satisfy `Interpolatable` (a `lerp(a, b, t) -> T` free function must exist -
`float`, `Color`, and `Paint` all have one in `types.h`; `Paint`'s handles cross-fading
gradient stops and interpolating the angle/center/radius fields together, falling back to a
hard switch at `t=1` if the two `Paint`s aren't structurally compatible - different `kind` or
stop count).

### `Widget::PairTween`

A private nested `AnimatableBase` used internally for `alignmentAnimation()`/
`fillAnimation()` - eases a widget's *displayed* x/y or width/height toward its newly-resolved
target instead of snapping:

```cpp
void Widget::syncPositionTween() {
    if (m_positionTween.isAnimating()) {
        if (m_resolvedX == m_positionTween.targetA() && m_resolvedY == m_positionTween.targetB())
            return;   // already animating toward this exact target - nothing to restart
    } else if (m_layoutInitialized && m_resolvedX == m_displayedX && m_resolvedY == m_displayedY) {
        return;   // no change at all
    }

    bool animate = m_layoutInitialized && m_alignmentAnimationEnabled.get() && m_alignmentAnimationDuration.get() > 0.0f;
    if (!animate) {
        if (m_positionTween.isAnimating()) m_positionTween.cancel();
        m_displayedX = m_resolvedX; m_displayedY = m_resolvedY;   // snap
        return;
    }

    m_positionTween.animateTo(this, &m_displayedX, &m_displayedY,
        m_displayedX, m_displayedY, m_resolvedX, m_resolvedY,
        m_alignmentAnimationDuration.get(), m_alignmentAnimationEasing.get());
}
```

`m_layoutInitialized` gates the very first layout pass from animating (there's no sensible
"from" position before the widget has ever been placed) - it snaps on the first call and only
eases on subsequent ones. `PairTween::animateTo` takes raw `float*` targets (`&m_displayedX`,
`&m_displayedY`) and writes directly into them each `tick()`, via `lerp()` - this is why
`alignmentAnimation`/`fillAnimation` only ever affect a widget's *own* rendered position/size
(`m_displayedX/Y/Width/Height`), never `m_resolvedX/Y/Width/Height` - layout itself always
uses the immediate, final resolved values, so children of an animating widget are never
themselves mid-animation-lag; only that one widget's paint position eases.

## Event system

`src/include/tavoos/events/` defines `Event`/`MouseEvent`/`KeyEvent`/`WheelEvent`; dispatch
logic lives entirely in `Window` (`window.cpp`), driven by GLFW callbacks registered in
`Window::setup()`.

### Bubbling

```cpp
template<typename EventT>
Widget* Window::dispatchBubble(Widget* start, EventT& event, void (Widget::*trigger)(EventT&)) {
    Widget* current = start;
    while (current) {
        if (current->hasHandlerFor(event.type())) {
            event.accept();
            (current->*trigger)(event);
            if (event.isAccepted())
                return current;
        }
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}
```

Walks from `start` up through `parent()` until it finds a widget with
`hasHandlerFor(event.type())`, fires the trigger method (a pointer-to-member, e.g.
`&Widget::triggerClick`), and stops. `dispatchBubble` calls `event.accept()` itself,
*before* invoking the handler - so it always stops at the first widget in the chain that has
*any* handler registered for that event type; the handler itself calling `ignore()` would
resume the walk, but no built-in handler does this today. `hasHandlerFor` is a simple
`switch` on `EventType` checking whether the matching `std::function` member is set:

```cpp
bool Widget::hasHandlerFor(EventType type) {
    switch (type) {
    case EventType::MouseClick: return static_cast<bool>(m_onClick);
    // ... one case per EventType, checking the matching m_on* std::function
    }
}
```

### Hit-testing

```cpp
Widget* Widget::hitTestTree(float px, float py) const {
    if (!visible()) return nullptr;
    const std::vector<Widget*> ordered = zOrderedChildren(*this);
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it)   // reverse: highest z / last-drawn first
        if (auto* hit = (*it)->hitTestTree(px, py))
            return hit;
    if (hitTest(px, py))
        return const_cast<Widget*>(this);
    return nullptr;
}

bool Widget::hitTest(float px, float py) const {
    const glm::vec4 localPoint = glm::inverse(worldMatrix()) * glm::vec4{px, py, 0.0f, 1.0f};
    return localPoint.x >= 0.0f && localPoint.x <= m_displayedWidth &&
           localPoint.y >= 0.0f && localPoint.y <= m_displayedHeight;
}
```

Children are walked in the *reverse* of render order (the same `zOrderedChildren()` list
used for rendering, reversed) - the topmost widget for painting is also the one that wins an
overlapping hit-test, matching rendering exactly. `hitTest` transforms the screen-space point
into the widget's *local* space via the inverse `worldMatrix()` - so rotation and scale
(applied in `localMatrix()`, see below) are correctly accounted for, not just axis-aligned
bounding boxes - then bounds-checks against `m_displayedWidth/Height`.

```cpp
glm::mat4 Widget::localMatrix() const {
    glm::vec3 position{m_displayedX, m_displayedY, 0.0f};
    glm::vec3 pivot{m_displayedWidth * 0.5f, m_displayedHeight * 0.5f, 0.0f};
    return glm::translate({}, position) * glm::translate({}, pivot)
         * glm::rotate({}, glm::radians(m_rotation), {0,0,1}) * glm::scale({}, {m_scale, m_scale, 1})
         * glm::translate({}, -pivot);
}
void Widget::updateWorldMatrix() {
    glm::mat4 parentWorld = /* parent Widget's worldMatrix(), or identity */;
    m_worldMatrix = parentWorld * localMatrix();
}
```

Rotation/scale pivot around the widget's own center (`pivot`), and `worldMatrix()` composes
with the parent's, so nested rotated/scaled widgets transform correctly relative to their
ancestors - both for rendering (the vertex shader uses `worldMatrix` directly) and for
hit-testing (its inverse).

### Press / release / click / double-click

```cpp
void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Widget* const hit = hitTestChildren(self, mx, my);   // fresh hit-test every call

    if (action == GLFW_PRESS) {
        self->m_pressedWidget = hit;
        if (hit) {
            dispatchBubble(hit, pressEvent, &Widget::triggerPress);

            const bool isDoubleClick = hit == self->m_lastClickWidget &&
                (now - self->m_lastClickTime) <= kDoubleClickTimeThreshold &&
                (dx*dx + dy*dy) <= kDoubleClickDistanceThreshold * kDoubleClickDistanceThreshold;
            if (isDoubleClick)
                dispatchBubble(hit, doubleClickEvent, &Widget::triggerDoubleClick);
        }
    } else if (action == GLFW_RELEASE) {
        if (self->m_pressedWidget)
            dispatchBubble(self->m_pressedWidget, releaseEvent, &Widget::triggerRelease);

        if (hit && hit == self->m_pressedWidget) {   // click only fires if release landed back on the pressed widget
            dispatchBubble(hit, clickEvent, &Widget::triggerClick);
            if (self->m_pressedWidget == hit) {       // still valid - the click handler may have removed it
                self->m_lastClickWidget = hit;
                self->m_lastClickTime = glfwGetTime();
                self->m_lastClickX = static_cast<float>(mx);
                self->m_lastClickY = static_cast<float>(my);
            }
        }
        self->m_pressedWidget = nullptr;
    }
}
```

Note `MouseRelease` dispatches to `self->m_pressedWidget` (the widget that was actually
pressed), not to a fresh `hit` at the release position - this is intentional mouse-capture
behavior: dragging off a pressed widget before releasing still correctly delivers the release
(and only the release, not a click, since `hit == m_pressedWidget` fails) to the originally
pressed widget, rather than to whatever happens to be under the cursor now. A `MouseClick`
additionally requires the same widget within `kDoubleClickTimeThreshold` (0.4s) *and*
`kDoubleClickDistanceThreshold` (5px) of the *previous* click for `MouseDoubleClick` to also
fire.

### Focus chain

```cpp
void Window::collectFocusable(Object* root, std::vector<Widget*>& out) {
    for (const auto& child : root->children()) {
        auto* const widget = dynamic_cast<Widget*>(child.get());
        if (!widget || !widget->visible()) continue;
        if (widget->focusable()) out.push_back(widget);
        collectFocusable(widget, out);   // depth-first, so a focusable container's own
    }                                     // focusable descendants come right after it
}

void Window::focusNext(bool reverse) {
    std::vector<Widget*> chain;
    collectFocusable(this, chain);
    if (chain.empty()) return;

    auto it = std::find(chain.begin(), chain.end(), m_focusedWidget);
    if (it == chain.end()) { setFocusedWidget(reverse ? chain.back() : chain.front()); return; }

    if (reverse) { if (it == chain.begin()) it = chain.end(); --it; }
    else         { ++it; if (it == chain.end()) it = chain.begin(); }
    setFocusedWidget(*it);
}
```

`keyCallback` intercepts Tab (`GLFW_KEY_TAB`) before anything else and calls `focusNext`
(`Shift+Tab` reverses); every other key only dispatches if `m_focusedWidget` is set. The
chain is recomputed from scratch on every Tab press rather than cached - simple, and cheap
enough in practice for typical UI sizes.

## Image & SVG rendering and caching

`ImageWidget` and `SVGWidget` each go through their own two-part caching scheme keyed by
source path, so the same asset loaded by multiple widgets only decodes/rasterizes once and
only uploads one GPU texture, reference-counted so it's freed once nothing uses it anymore.

**`ImageWidget`** (`src/gfx/texturecache.cpp`): `TextureCache` is a single
`path -> Entry{texture, width, height, refCount}` map. `acquire(path)` either bumps
`refCount` on a cache hit, or decodes via `stb_image` (from a resource or the filesystem,
via `parseResourcePath` - see [Resource embedding](#resource-embedding)) and uploads a new
`GL_RGBA8` texture on a miss. `release(path)` decrements `refCount` and deletes the GL
texture once it hits zero. `ImageWidget::reload()` (triggered by `source()` changing) calls
`releaseCurrent()` then `acquire()` for the new path; the destructor just calls
`releaseCurrent()`. `naturalWidth()`/`naturalHeight()` come straight from the decoded image
dimensions, cached on the `Entry`.

**`SVGWidget`** is two caches, because an SVG (unlike a raster image) doesn't have one
natural pixel size - it has to be rasterized *at* a target size, and that target size is
whatever the widget resolves to, which can differ per instance and can change with layout:

- **`SVGDocumentCache`** (`src/gfx/svgdocumentcache.cpp`) - `path -> lunasvg::Document`,
  parsed once per path (not ref-counted or released - the parsed vector document is cheap to
  keep around, and needed again any time a size changes). `ensureNaturalSizeKnown()` reads
  the document's own `width()`/`height()` from here for the widget's natural size.
- **`SVGTextureCache`** (`src/gfx/svgtexturecache.cpp`) - `"path:WxH" -> Entry{texture,
  refCount}`, one rasterized GPU texture *per size actually used*. `SVGWidget::ensureRasterized()`
  (called from `render()`) only re-rasterizes when the resolved size actually changed since
  the last render (`path == m_loadedPath && w == m_loadedWidth && h == m_loadedHeight` short-circuits
  otherwise); rasterization dimensions are clamped to `[1, 8192]`, refusing (and logging) anything
  outside that range rather than attempting a pathological allocation.

Both widgets' `render()` just delegates to the matching `Renderer::renderImage`/`renderSVG`,
which bind the cached texture and draw a textured quad - see
[Rendering pipeline](#rendering-pipeline).

## Text rendering

`src/text/`.

### `FontFace` / `FontManager` / `GlyphAtlas`

`FontManager` owns registered `FontFamily`s (weight/style variants per family name), falling
back to the system default via Fontconfig on Linux if nothing is registered for a requested
family. `FontFace` wraps a single FreeType face and owns one or more `GlyphAtlas` pages that
lazily rasterize and pack glyphs into a GPU texture the first time each codepoint is actually
needed - so an app that never uses, say, CJK glyphs never pays to rasterize or upload them.

### Wrap, elide, and line-clamp

These are free functions in `textlayout.h`/`.cpp` (`layoutTextLines`, `elideLine`,
`effectiveMaxLines`, `clampLines`), independently testable without a widget tree - `TextWidget`
just calls them and caches the result (`LineCache`, keyed on text/font/scale/wrap settings, so
re-wrapping unchanged text at an unchanged width is a cache hit).

**Wrapping** (`layoutTextLines`) walks the text codepoint-by-codepoint, tracking the last seen
word-break opportunity (a space, under `WrapMode::WordWrap`):

```cpp
const bool needsBreak = (wrapMode != WrapMode::NoWrap) && wrapWidth > 0.0f &&
                  lineWidth + advance > wrapWidth && lineWidth > 0.0f;

if (needsBreak) {
    if (wrapMode == WrapMode::WordWrap && lastBreakOpportunity != std::string::npos
        && lastBreakOpportunity > lineStart) {
        // break at the last space, dropping it, carry the remainder's width forward
        lines.push_back({lineStart, lastBreakOpportunity, widthAtLastBreak, ""});
        lineStart = lastBreakOpportunity + 1;
        lineWidth -= widthAtLastBreak;
    } else {
        // no usable word-break in this line (or it's WrapAnywhere) - hard break right here
        lines.push_back({lineStart, charStart, lineWidth, ""});
        lineStart = charStart;
        lineWidth = 0.0f;
    }
    lastBreakOpportunity = std::string::npos;
}
```

The `lastBreakOpportunity > lineStart` condition guards against a break opportunity recorded
at (or before) the *current* line's own start byte - e.g. a leading space before an overlong
word - being used as the break point, which would otherwise produce a spurious empty line
(`{lineStart, lineStart, 0, ""}`) before the actual word; that case falls through to the
hard-break branch instead.

**Eliding** (`elideLine`) binary-searches for the longest prefix/suffix that fits alongside an
ellipsis character, for `ElideMode::Right`/`Left`; `Middle` grows a prefix and suffix
independently (prefix first, up to half the available width, then suffix up to the *actual*
remaining width) rather than a single binary search, since a middle ellipsis has two
independent cut points.

**Line-clamping** (`clampLines`, applied when `maxLines()` truncates the wrapped result):
`effectiveMaxLines` takes the *stricter* of an explicit `maxLines()` and however many lines
fit in the widget's resolved height (`floor(resolvedHeight / lineHeight)`) - if both are set,
whichever allows fewer lines wins. `clampLines` then either leaves the elide-check to the
last natural line (if nothing was actually truncated) or slices to the limit and re-elides
whichever line(s) sit at the cut boundary (`Right` re-elides the last kept line; `Left` the
first; `Middle` re-elides the line just before the gap).

## Resource embedding

`cmake/TavoosResources.cmake`'s `tavoos_add_resources(target file...)` generates one `.cpp`
per listed file at configure/build time (`GenerateResource.cmake`), each defining a `static`
byte array and a static initializer that calls `ResourceRegistry::registerResource`:

```cpp
// resourceregistry.cpp - a thread-safe, process-wide map
static std::mutex& registryMutex() { static std::mutex instance; return instance; }
static std::unordered_map<std::string, ResourceRegistry::Entry>& registryMap() {
    static std::unordered_map<std::string, ResourceRegistry::Entry> instance;
    return instance;
}
void ResourceRegistry::registerResource(const std::string& path, const unsigned char* data, std::size_t size) {
    std::lock_guard lock{registryMutex()};
    registryMap()[path] = Entry{data, size};
}
```

`ResourceRegistry` only stores the raw pointer it's given, not a copy - the generated code's
`static` storage duration is what makes that safe. If you ever call `registerResource`
directly instead of through codegen, the buffer you pass must likewise outlive the whole
program (e.g. `static` storage) - there's no lifetime enforcement or documentation on the API
itself, so this is easy to get wrong.

At runtime, `parseResourcePath()` distinguishes the scheme:

```cpp
ParsedPath parseResourcePath(const std::string& uri) {
    if (uri.rfind("resource:/", 0) == 0) return { PathScheme::Resource, uri.substr(10) };
    if (uri.rfind("file:", 0) == 0)      return { PathScheme::File, uri.substr(5) };
    return { PathScheme::File, uri };   // bare path defaults to filesystem
}
```

A `resource:/...` path is looked up via `ResourceRegistry::find`; everything else (a `file:`
prefix or a bare path) is read from the filesystem at runtime.

## Application & Window lifecycle

```cpp
Application::Application() {
    if (m_instance != nullptr) throw std::runtime_error{"only one Tavoos::Application instance is allowed"};
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_instance = this;
    renderer = std::unique_ptr<Renderer>(new Renderer());   // new, not make_unique - Renderer's ctor is private
}

int Application::run() {
    double lastTime = glfwGetTime();
    while (!m_shouldQuit) {
        if (AnimationManager::instance().hasActiveAnimations())
            glfwPollEvents();    // non-blocking: keep animating even with no input
        else
            glfwWaitEvents();    // blocking: idle CPU when nothing's happening

        const float dt = /* time since lastTime */;
        AnimationManager::instance().tick(dt);

        for (const auto& window : m_windows)
            renderer->renderForWindow(*window);   // each independently no-ops via consumeDirty()
    }
    shutdown();
    return 0;
}
```

`renderer` is `std::unique_ptr<Renderer>` in `Application`'s header specifically to keep
`application.h` from needing to `#include` the GL loader (`Renderer` is only
forward-declared there) - PIMPL, but for compile-firewall reasons rather than ABI stability.
`Renderer()`'s constructor is `private` (friended to `Application`), so it's built with
`new Renderer()` wrapped manually rather than `std::make_unique<Renderer>()` -
`make_unique`'s internal `new T()` call happens inside the standard library's own template
body, which isn't covered by `Application`'s friendship with `Renderer` (friendship isn't
transitive into library code).

`glfwPollEvents()` vs `glfwWaitEvents()` is the idle-CPU story: with no active animations,
the loop blocks entirely until an OS input event arrives, so an idle Tavoos app uses ~0% CPU;
as soon as anything is animating, it switches to non-blocking polling so the animation can
keep advancing every iteration.

Multiple `Window`s share one GL context via `Application::m_shareContext` - a hidden
1x1 window created lazily (`shareContextHandle()`) purely to be the shared-context anchor
every real `Window` is created against, so textures/shaders/buffers created for one window
are usable from another without duplication.

## Adding a new widget type

1. Subclass `Widget` (or an existing concrete widget, if you're extending rather than
   replacing behavior - nothing is `final`).
2. Override `render(Renderer&)` (pure virtual on `Widget`) - typically by adding a matching
   `Renderer::render<Yours>()` method if it needs new GL state, or reusing
   `renderChildren(r)` if it's purely a layout container with no visual of its own.
3. Override `computeIntrinsicSize()` if your widget has a natural size that isn't just its
   explicit `width()`/`height()` (text measures its laid-out lines; Column/Row/Grid/Flex
   measure their children).
4. If it should be declarable inside `Window::build()`, add a `static void
   YourWidget(std::function<void(YourWidgetType&)>)` to `Builder` following the existing
   pattern (`create<YourWidgetType>(std::move(body))`).
5. If it needs to be a layout container, override `layout()` (private) and set
   `isLayouter()` to return `true`.
