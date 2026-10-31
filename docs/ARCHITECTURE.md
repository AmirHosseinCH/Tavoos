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
   - [The build-complete hook](#the-build-complete-hook)
   - [Components](#components)
4. [Reactive properties](#reactive-properties)
   - [`Property<T>`](#propertyt)
   - [`State<T>`](#statet)
   - [`PropertyArg<T>`](#propertyargt)
   - [`BindableState<T>`](#bindablestatet)
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
   - [Graphical effects](#graphical-effects)
7. [Animation system](#animation-system)
   - [`AnimatableBase` / `AnimationManager`](#animatablebase--animationmanager)
   - [`AnimatedState<T>`](#animatedstatet)
   - [`Widget::PairTween`](#widgetpairtween)
   - [Transitions](#transitions)
8. [Event system](#event-system)
   - [Bubbling](#bubbling)
   - [Hit-testing](#hit-testing)
   - [Hover](#hover)
   - [Cursor](#cursor)
   - [Press / release / click / double-click](#press--release--click--double-click)
   - [Focus chain](#focus-chain)
   - [Focus memory](#focus-memory)
   - [Shortcuts](#shortcuts)
9. [Interaction state & control widgets](#interaction-state--control-widgets)
   - [`Control` and the behavior bases](#control-and-the-behavior-bases)
   - [Click-to-focus](#click-to-focus)
   - [Coordinate mapping and mouse dispatch](#coordinate-mapping-and-mouse-dispatch)
   - [Dragging](#dragging)
   - [Drag and drop](#drag-and-drop)
   - [Text editing](#text-editing)
   - [Overlays and popups](#overlays-and-popups)
   - [Flick areas](#flick-areas)
   - [Scroll areas](#scroll-areas)
   - [List views](#list-views)
   - [Combo boxes](#combo-boxes)
   - [Tooltips](#tooltips)
   - [Stack views](#stack-views)
   - [Swipe views](#swipe-views)
   - [Theme and style structs](#theme-and-style-structs)
   - [The interaction-color pattern](#the-interaction-color-pattern)
10. [Image & SVG rendering and caching](#image--svg-rendering-and-caching)
11. [Text rendering](#text-rendering)
    - [`FontFace` / `FontManager` / `GlyphAtlas`](#fontface--fontmanager--glyphatlas)
    - [Wrap, elide, and line-clamp](#wrap-elide-and-line-clamp)
    - [Selection](#selection)
12. [Resource embedding](#resource-embedding)
13. [Application & Window lifecycle](#application--window-lifecycle)
14. [Adding a new widget type](#adding-a-new-widget-type)

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
 ├─ Window                (src/include/tavoos/window.h)       -- tree root, not a Widget
 └─ Widget                (src/include/tavoos/widget/widget.h)
     ├─ RectangleWidget, ImageWidget, SVGWidget, TextWidget   -- draw something of their own
     └─ EffectlessWidget                                      -- rejects shadows, background blur, glass
         ├─ ColumnWidget, RowWidget, GridWidget, FlexWidget   -- layout containers
         └─ Control                                           -- background/content slots, enabled, hovered
             └─ ButtonBase, RangeBase, OverlayBase, ...       -- behavior bases (widget/templates/)
                 └─ ButtonWidget, SliderWidget, ...           -- concrete controls (widget/controls/)
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

`EffectlessWidget` (`widget/effectlesswidget.h`) adds nothing but compile-time errors for the effect
setters (see [Graphical effects](#graphical-effects)). Controls are built on `Control` and a behavior
base (see [`Control` and the behavior bases](#control-and-the-behavior-bases)), and a
`Component<Root>` derives from whichever widget type it wraps (see [Components](#components)).

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
        std::unique_ptr<Object> object = std::make_unique<T>(parentObject);
        T* const raw = static_cast<T*>(object.get());
        if constexpr (std::derived_from<T, Widget>) {
            currentItem = object.get();
            body(*raw);
            parentObject->appendChild(object);
            raw->finishBuild();
            currentItem = parentObject;
        } else {
            body(*raw);
            parentObject->appendChild(object);
        }
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

`T` does not have to be a widget: `create<T>()` and `TB::Create<T>` accept any `Object`. A plain
object is constructed and appended the same way, but it skips `finishBuild()`, since it has no
build hook or layout, and `currentItem` stays on the enclosing widget while its body runs. A `TB::`
call inside the body therefore adds the widget to the enclosing widget rather than under an object
that cannot show it, and a nested plain object becomes the enclosing widget's child too. Every
traversal of `children()` (layout, rendering, hit-testing, focus, and the item scans of stack and
swipe views) casts each child with `dynamic_cast<Widget*>` and skips the rest, so a plain object
takes no layout slot and draws nothing.

Code that has to run a user body after the build can give the builder a parent for the length of
that body with `Builder::Scope`, an RAII object whose constructor points `currentItem` at a widget
and whose destructor restores the previous value. `ListViewBase` uses it for its delegate.

**2. `Widget::addChild<T>()`** (`widget.h`):

```cpp
template<typename T>
T* addChild(std::function<void(T&)> body = {}) {
    static_assert(std::is_base_of_v<Object, T>, "addChild<T>() requires T to derive from Object");
    auto child = std::make_unique<T>(this);
    T* raw = child.get();
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
```

Like `create<T>()`, it accepts any `Object` and only finishes the build and requests a relayout for
widgets.

This has no dependency on `Builder::currentItem` at all - it's the only safe way to grow the
tree dynamically, at runtime, after `build()` has already returned. If a dynamically-added
widget itself needs children, nest more `addChild<T>()` calls, not `TB::` ones - they'll
silently do nothing useful (`currentItem` is `nullptr`, so `TB::create<T>` dereferences it).

### The build-complete hook

A property setter runs its change callbacks immediately, so a widget whose callbacks do real work
repeats it for every setter in the body, and does it before the children, slots or pages it depends
on exist. `Widget` therefore has a build-complete hook:

- `built()` is false while the constructor and the body run, and true afterwards.
- `onBuilt()` is a protected virtual, empty by default. It runs once, after the body has run and the
  widget has been appended to its parent.
- `finishBuild()` is private and sets the flag, then calls `onBuilt()`. It is called from the two
  creation paths and nowhere else: `Builder::create` (a friend of `Widget`) and `addChild<T>`. Those
  are the only places a widget is created, so the flag is reliable.

Children finish before their parents, so a widget's `onBuilt()` sees all of its children, slots and
pages already built. A component's `build()` and its body both run before its `onBuilt()`.

A base class opts in by skipping derived work while `!built()` and doing it once in `onBuilt()`:

- `FlickAreaBase::syncOffsets()` only records `m_syncPending` before the build, and `onBuilt()`
  syncs once if something was deferred. `ScrollAreaBase` does the same for its bar updates
  (`m_wakeOnBuilt`).
- `SwipeViewBase` ignores `currentIndex` changes and `refreshPages()` before the build, and
  `onBuilt()` reconciles the pages and applies the requested index once, with every page present.

Setters called after the build behave as they always did. Derived states, such as `contentY()` or
`currentIndexState()`, keep their initial values until `onBuilt()`. `s_isBuilding` is separate: it
only suppresses relayout requests during the window's initial build.

### Removing widgets: two-phase deferred destruction

`Widget::removeSelf()`/`removeChild()` don't destroy anything immediately. `removeChild` takes
any `Object`, so plain objects are removed the same way:

```cpp
void Widget::detachAndDefer(Object* child) {
    Object* const parentObj = child->parent();
    if (!parentObj)
        return;

    std::unique_ptr<Object> detached = parentObj->detachChild(child);
    if (!detached)
        return;

    if (m_ownerWindow) {
        m_ownerWindow->clearReferencesTo(child);
        m_ownerWindow->markDirty();
        m_ownerWindow->deferDestruction(std::move(detached));
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
cached pointers that point into it and erases them from the hovered chain (see [Hover](#hover)):

```cpp
void Window::clearReferencesTo(Object* subtreeRoot) {
    auto isOrDescendantOf = [subtreeRoot](const Object* object) {
        while (object) {
            if (object == subtreeRoot)
                return true;
            object = object->parent();
        }
        return false;
    };

    std::erase_if(m_hoveredChain, isOrDescendantOf);
    if (isOrDescendantOf(m_pressedWidget))   m_pressedWidget = nullptr;
    std::erase_if(m_pressedChain, isOrDescendantOf);
    if (isOrDescendantOf(m_lastClickWidget)) m_lastClickWidget = nullptr;
    if (isOrDescendantOf(m_focusedWidget))   setFocusedWidget(nullptr);
}
```

Note the general shape this implies for any code that caches a `Widget*` in `Window`: since
an event handler can remove the very widget an event is being dispatched to (or an ancestor
of it), any write of a `Widget*` into `Window`'s own state *after* dispatching to it should
first check that the widget wasn't just invalidated - `clearReferencesTo`'s nulling is the
signal to check against, not new tracking.

### Components

`Component<Root>` (`tavoos/component/component.h`) and its non-template base `ComponentBase`
(`tavoos/component/componentbase.h`) build reusable compositions on top of the tree building above.
`Component<Root>` derives both `Root`, any widget type, and `ComponentBase`, and inherits
`Root`'s constructors, so every fluent setter of the root keeps working and keeps returning the
derived type. `ComponentBase` holds a virtual `build()` and a private `runBuild()` that only
`Builder` and `Widget` can call.

`build()` cannot run from a constructor, where virtual dispatch to the derived class does not
exist yet, so creation is two-phase and both creation paths know about it:

```cpp
template<typename T>
static void create(std::function<void(T&)> body) {
    const auto parentObject = currentItem;
    std::unique_ptr<Object> object = std::make_unique<T>(parentObject);
    T* const raw = static_cast<T*>(object.get());
    if constexpr (std::derived_from<T, Widget>) {
        currentItem = object.get();
        if constexpr (std::is_base_of_v<ComponentBase, T>)
            static_cast<ComponentBase&>(*raw).runBuild();
        body(*raw);
        parentObject->appendChild(object);
        raw->finishBuild();
        currentItem = parentObject;
    } else {
        body(*raw);
        parentObject->appendChild(object);
    }
}
```

`Widget::addChild<T>` makes the same `if constexpr` check and calls `runBuild()` before running
its body, so a component created after the window is built is built the same way. Detecting it at
compile time means `Widget` itself gains no virtual.

After the body returns and the component is appended, `finishBuild()` runs the component's
`onBuilt()` like any other widget's, after its `build()` and its body (see
[The build-complete hook](#the-build-complete-hook)).

`runBuild()` saves `Builder::currentItem` (which is why `ComponentBase` is a `Builder` friend),
points it at the component, calls `build()`, and restores the saved value. In `create` that saved
value is the component itself, so the instance's body then runs in the same scope. Two
consequences follow from the order *construct, `build()`, then the instance's body*:

- The internal tree exists before any property is assigned, so `build()` binds to the
  component's property states (`m_name.state()`) instead of reading values; a property set
  later flows through the binding.
- `TB::` calls in the body construct their widgets with `currentItem`, the component, as parent,
  so children declared inside an instance attach to the component, after whatever `build()`
  created.

Slot bodies (`background`, `content`, ...) run inside `addChild` before the slot is appended, and
they are *not* in a builder scope for the slot: a `TB::` call there would attach to
`currentItem`, the component, not to the slot. Inside a slot body a component uses `addChild<T>`
on the slot widget.

`TAVOOS_PROPERTY(Type, name, default)` expands to a `BindableState<Type>` member `m_name`, a
fluent `name(PropertyArg<Type>)` using deducing `this`, a `name()` getter, and a `nameState()`
accessor returning the output `State`. `TAVOOS_CALLBACK(name, Signature)` expands to a protected
`std::function` member `m_name` and a fluent setter. Both leave the class in a `public:` section.

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

        const auto callbacksSnapshot = m_callbacks;
        for (const auto& [id, callback] : callbacksSnapshot) {
            const bool stillRegistered = std::any_of(m_callbacks.begin(), m_callbacks.end(),
                                                     [id](const auto& entry) { return entry.first == id; });
            if (stillRegistered)
                callback(m_value);
        }
    }

    std::size_t onChange(std::function<void(const T&)> callback) {
        const std::size_t id = m_nextCallbackId++;
        m_callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void removeOnChange(std::size_t id) {
        std::erase_if(m_callbacks, [id](const auto& entry) { return entry.first == id; });
    }

    void set(const T& value) {
        m_value = value;
        notifyObservers();
    }

    const T& get() const noexcept { return m_value; }

private:
    T m_value{};
    std::vector<Property<T>*> m_observers;
    std::vector<std::pair<std::size_t, std::function<void(const T&)>>> m_callbacks;
    std::size_t m_nextCallbackId{1};

    void registerObserver(Property<T>* observer)   { m_observers.push_back(observer); }
    void unregisterObserver(Property<T>* observer) { std::erase(m_observers, observer); }
};
```

`notifyObservers()` snapshots the observer list *before* iterating, then checks each
snapshotted pointer is still present in the *live* list before calling it - the same
snapshot-then-recheck happens for `m_callbacks` right after, for the same reason: an
observer's own `onChange` callback (bound `Property`s or a direct `State::onChange`
registration alike) can legally unbind itself (or another observer/callback) mid-notify -
`Property::unbind()` or `State::removeOnChange()`, called from such a callback, would
otherwise be mutating the list being iterated. `onChange`/`removeOnChange` are how a
`State<T>` is observed *directly*, independent of any `Property` bound to it - every control's
`*State()` getter (`checkedState()`, `valueState()`, `focusedState()`, ...) returns exactly
this, so code outside the widget can react to a value changing without needing a `Property` of
its own in between.

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

### `BindableState<T>`

A `Property<T>` and a `State<T>` glued together (`bindablestate.h`), for a widget property
that both accepts external input the normal way (a plain value or a `State<T>&`, via
`PropertyArg<T>`) *and* needs to be read reactively by something else internally - typically a
child slot binding to a parent control's own property:

```cpp
template <typename T>
class BindableState {
public:
    explicit BindableState(const T& value) : m_input{value}, m_output{value} { connect(); }

    void set(PropertyArg<T> value) { value.applyTo(m_input); }
    const T& get() const noexcept { return m_output.get(); }
    State<T>& state() noexcept { return m_output; }

    std::size_t onChange(std::function<void(const T&)> callback) { return m_output.onChange(std::move(callback)); }

private:
    void connect() { m_input.onChange([this](const T& value) { m_output.set(value); }); }

    Property<T> m_input;
    State<T> m_output;
};
```

`m_input` is what the widget's own setter writes to (via `set()`, called from the fluent
setter the same way every other property does); `m_output` is what gets handed to a child via
`.state()`, e.g. `box.radius(m_radius.state())` in a control's constructor. `connect()` wires
`m_input`'s `onChange` straight into `m_output.set(...)`, so every value written to the input -
whether a plain value or a live binding to an external `State<T>` - propagates through to the
output automatically. A control reaches for `BindableState<T>` instead of a plain
`Property<T>` whenever the property is a pure passthrough with no extra selection logic
(`radius`, `borderWidth`, `iconSize`, `transition` on most controls); one with real
state-dependent selection logic (idle/hover/pressed/disabled color, for example) uses a plain
`Property<T>` input feeding an `AnimatedState<T>` output instead - see
[The interaction-color pattern](#the-interaction-color-pattern).

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
                     m_rotation, m_scale, m_scaleX, m_scaleY, m_rotationX, m_rotationY,
                     m_perspective, m_transformOrigin,
                     m_gridColumnSpan, m_gridRowSpan, m_gridRow, m_gridColumn);
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
    const bool widthFixed  = hasFlag(m_fill.get(), Fill::Width)  || m_width.get()  > 0 || m_widthFraction.get()  >= 0.0f;
    const bool heightFixed = hasFlag(m_fill.get(), Fill::Height) || m_height.get() > 0 || m_heightFraction.get() >= 0.0f;
    return widthFixed && heightFixed;
}
```

A "size boundary" is a widget whose own resolved size doesn't depend on its children (it's
either `fill()`-driven from its parent, has an explicit `width()`/`height()`, or is sized by
`widthFraction()`/`heightFraction()`) - so a
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
        ? effWidth
        : (m_widthFraction.get() >= 0.0f) ? std::clamp(m_widthFraction.get(), 0.0f, 1.0f) * effWidth
        : (m_width.get() > 0) ? static_cast<float>(m_width.get()) : natural.width;
    // m_resolvedHeight: same, with heightFraction / effHeight

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
parent" case: resolve size from `fill()`/fraction/explicit size/intrinsic size, then position
from `alignment()` flags or explicit `x()`/`y()`.

`widthFraction()`/`heightFraction()` resolve here too, as a fraction of `effWidth`/`effHeight`
(the parent's content area minus this widget's margins), so a control can express "this
child is N% of me" declaratively - `ProgressBarWidget` binds its fill's `widthFraction` to a
normalized position state - instead of computing pixels from a value. Because they live in
this branch they're ignored under a layouter parent, which sets the child's geometry
directly via `setResolved()`, and a layouter widget doesn't read them for its own size.

`xFraction()`/`yFraction()` position a widget at a fraction of the free space left after its own
size and margins (`effX + f * (effWidth - ownWidth)`), so 0 is the leading edge, 1 the trailing
edge, and 0.5 centered; they apply when no alignment flag is set on that axis and take priority
over an explicit `x()`/`y()`.

A widget that needs a value derived from its final size - a circular radius, an icon sized to its
box - can't read `width()`/`height()`, which are only the properties and ignore `fill()`,
fractions, and layout. `Widget::onResolvedSizeChanged()` is the hook for this: `Widget::layout()`
calls it as soon as the resolved size is known, and only when it differs from the last size it
announced:

```cpp
void Widget::notifyResolvedSizeIfChanged() {
    if (m_resolvedWidth == m_notifiedWidth && m_resolvedHeight == m_notifiedHeight)
        return;
    m_notifiedWidth = m_resolvedWidth;
    m_notifiedHeight = m_resolvedHeight;
    onResolvedSizeChanged();
}
```

It runs before this widget's children are laid out, so properties it sets on them are used in the
same pass - no extra pass, no frame of stale values. The first layout always announces (the
notified size starts unset), so a control's pre-layout defaults are replaced by values from its
real size. `RadioWidget`, `CheckboxWidget`, and `SwitchWidget` use it to derive their radius,
checkmark size, and thumb size from `resolvedWidth()`/`resolvedHeight()`. It's called from the
base `Widget::layout()`, so a layouter (Row, Column, Grid, Flex), which overrides `layout()`,
doesn't announce.

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

    window.layoutTree();

    for (Widget* const widget : Widget::zOrderedChildren(window))
        if (widget->visible())
            renderWidget(*widget);

    std::vector<OverlayBase*> overlays = window.retiredOverlays();
    overlays.insert(overlays.end(), window.overlays().begin(), window.overlays().end());
    for (OverlayBase* const overlay : overlays)
        if (overlay->visible())
            renderWidget(*overlay);
}

void Window::layoutTree() {
    for (const auto& child : children()) {
        if (auto* widget = dynamic_cast<Widget*>(child.get()))
            widget->layout();
    }
    std::vector<OverlayBase*> overlays = m_retiredOverlays;
    overlays.insert(overlays.end(), m_overlays.begin(), m_overlays.end());
    for (OverlayBase* const overlay : overlays) {
        overlay->place();
        overlay->layout(true);
    }
}
```

In order: skip entirely if `consumeDirty()` says nothing changed; destroy anything
detached-and-deferred by an event handler in a *previous* frame (see
[Building the tree](#building-the-tree)); lazily create this window's own GL resources
(`ensureWindowResources` - each window has independent VAOs/FBO, tracked in
`m_windowResources`, since multiple windows can share one GL context); set up the projection
from logical window size (not physical framebuffer size - see `HiDPI` note below); clear;
lay out with `Window::layoutTree`, which runs the
[dirty-flag-driven layout pass](#dirty-flag-propagation) on every top-level child and then places
and lays out the window's [overlays](#overlays-and-popups); render every visible top-level child in
`z()` order; then render the overlays on top, closing ones first and then the open ones in stack
order. `layoutTree` is also called by the drop search during a drag (see
[Drag and drop](#drag-and-drop)), so a frame after it finds the tree already clean.

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

`Widget::clip(true)` routes through `Renderer::renderWidgetContents` differently. `renderWidget`
reaches it after the widget's glass, background blur and shadows, either directly or through a
layer blur's capture (see [Graphical effects](#graphical-effects)):

```cpp
void Renderer::renderWidgetContents(Widget& widget) {
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
shimmer on the common case of an unrotated, unscaled clipped widget). The check also requires the
matrix's `w` row to be `(0, 0, 0, 1)`, so a widget with perspective is never snapped.

`m_capturingMask` is restored to its *previous* value on exit rather than hard-coded to
`false`, so nested clipped widgets (a clipped child inside a clipped parent) don't clear the
outer capture flag while it's still in progress.

A clipped widget's own `render()` must draw something (a shape - fill, border, whatever) via
the matching `Renderer::render*()` call *before* delegating to `renderChildren()`, since that
draw call is what actually becomes the mask when `m_capturingMask` is true; `renderChildren()`
itself no-ops during mask capture (see `ensureClipMask` above), so a widget whose `render()`
is *only* `renderChildren(r)` - a bare layout wrapper with no visual of its own - has nothing
to contribute to its own mask if `clip(true)` is set directly on it. To clip a composite's
content, put `clip(true)` on an actual shaped primitive (typically a `RectangleWidget`, even a
fully transparent one) and nest the content that needs bounding as *its* children, rather than
clipping the composite wrapper itself.

A `Control` can also clip itself if its `render()` draws a shape while the mask is being
captured: `FlickAreaBase` renders its `background` slot through `renderer.renderWidget(...)` in
that case, so the clip takes the shape of the background (see [Flick areas](#flick-areas)).

### Graphical effects

A `GraphicalEffect` (`src/include/tavoos/effects/graphicaleffect.h`) decorates a widget's drawing
without being part of the widget tree. It is not an `Object`: it has no parent, takes no part in
layout, focus or hit-testing, and holds only a reference to the `Widget` that owns it. Every
`Widget` owns an ordered `std::vector<std::unique_ptr<GraphicalEffect>>`:

```cpp
DropShadow& addDropShadow(const ShadowStyle& style = {});
DropShadow& addDropShadow(State<ShadowStyle>& style);
InnerShadow& addInnerShadow(const ShadowStyle& style = {});
InnerShadow& addInnerShadow(State<ShadowStyle>& style);

decltype(auto) dropShadow(this auto&& self, const ShadowStyle& style = {});
decltype(auto) dropShadow(this auto&& self, State<ShadowStyle>& style);
decltype(auto) innerShadow(this auto&& self, const ShadowStyle& style = {});
decltype(auto) innerShadow(this auto&& self, State<ShadowStyle>& style);

void removeEffect(GraphicalEffect& effect);
void clearEffects();
```

The public API is typed on purpose. Each `add*` function constructs the effect with the widget,
appends it through a private `attachEffect<E>()`, applies the given `ShadowStyle` and returns a
reference that stays valid until that effect is removed; the `dropShadow()` and `innerShadow()`
forms are the fluent versions for a builder chain. Naming each kind in the API keeps the
cardinality of each kind visible: shadows stack, while [layer blur](#layer-blur),
[background blur](#background-blur) and [glass](#glass) are at most one each and are ordinary
properties of `Widget`, not entries in this list. Adding,
removing, or changing any property an effect registered with `bindInvalidateTriggers(...)` calls
`GraphicalEffect::invalidate()`, which requests a repaint of the owner exactly like
`bindRepaintTriggers` does for a widget's own properties.

`ShadowStyle` (`effects/shadowstyle.h`) is a plain aggregate of the five shadow values, with a
`lerp` so that an `AnimatedState<ShadowStyle>` can animate a whole shadow, and it is the same
shape a style struct would carry when a widget class installs a default shadow. `ShadowBase`
keeps its five `Property` members and a `BindableState<ShadowStyle>` in front of them, the same
arrangement as `ScrollAreaSkin`: `style()` sets that state from a value or binds it to a
`State<ShadowStyle>`, and its change callback writes the five properties. Writing a property
unbinds it, so a style applied later replaces any earlier per-property binding.

Controls and layouts do not accept effects. `Control`, `ColumnWidget`, `RowWidget`, `GridWidget`
and `FlexWidget` derive from `EffectlessWidget` (`widget/effectlesswidget.h`), a thin `Widget`
subclass that hides `dropShadow`, `innerShadow`, `addDropShadow` and `addInnerShadow` behind a
`static_assert` (`detail::rejectEffect`). These widgets are made of other widgets and have no
drawing of their own to cast from, so there is no single shape to attach a shadow to. The parts are
ordinary widgets with their own effect lists, so the shadow goes on the one that should carry it,
for example inside a control's `background(...)` slot or on a `RectangleWidget` wrapped around a
layout. The block is compile-time only and applies through the named type, so a component whose
root is one of these widgets inherits it.

Rendering reaches an effect through virtuals that each name a stage of the widget's drawing. An
effect overrides the one for its stage, so ordering between kinds of effect is fixed by the
stages, and list order only matters between effects of the same stage:

- `renderBehind(Renderer&, Widget&)` runs before the widget draws anything. `Renderer` calls it
  from `renderWidget` for every effect in list order, after the glass or background blur and
  before the layer blur and the widget's own `clip()` branch.
- `renderInside(Renderer&, Widget&)` runs after the widget's own drawing and before its
  children. The widget calls it itself, through the protected `Widget::renderInnerEffects()`,
  at that point in its `render()`.

The first stage is the loop in the middle of `renderWidget`:

```cpp
void Renderer::renderWidget(Widget& widget) {
    if (!isCapturingOwn()) {
        if (widget.hasGlass() && widget.shapeRadii()) {
            renderGlass(widget, widget.glass());
        } else {
            const BlurStyle backdrop = widget.backgroundBlur();
            if (std::max(backdrop.startRadius, backdrop.endRadius) > 0.01f)
                renderBackgroundBlur(widget, backdrop);
        }

        for (const auto& effect : widget.effects())
            effect->renderBehind(*this, widget);
    }

    if (!isCapturingOwn()) {
        const BlurStyle blur = widget.layerBlur();
        if (std::max(blur.startRadius, blur.endRadius) > 0.01f) {
            renderLayerBlur(widget, blur);
            return;
        }
    }

    renderWidgetContents(widget);
}
```

Running before `renderWidgetContents`, which holds the [`clip()` branch](#clipping), is what lets
a clipped widget keep its own shadow: the clip layer is a texture exactly the size of the widget,
so anything drawn inside it would be cropped. An ancestor's clip still crops the shadow, because
the ancestor's layer is the render target at that point. While a clip mask or a widget's own
drawing is being captured (`isCapturingOwn()`) the hook is skipped, since only the widget's own
shape or drawing belongs in that capture. Effects do not touch layout or hit-testing, so a shadow
can extend past the widget's box without changing where anything is placed or what receives a
click. The glass, background blur and layer blur steps are described in their own sections below.

A shadow can be computed in two ways, and `virtual std::optional<CornerRadii> Widget::shapeRadii()
const` picks between them. It returns the four corner radii (`topLeft`, `topRight`,
`bottomRight`, `bottomLeft`) of a rounded box covering the widget's displayed size, or
`std::nullopt` for a widget with no such shape. `RectangleWidget` overrides it and gets the
analytic path described next; every other widget inherits `nullopt` and gets the
[alpha-based path](#alpha-based-shadows).

`DropShadow` is drawn by its own shader (`shadowFragmentShader`) over a quad larger than the
widget: the box moved by the offset, grown by `spread`, and padded by three standard deviations
(`sigma = radius / 2`) plus one pixel. The vertex shader is the rectangle's, and the rounded-box
distance function is shared between the two fragment shaders as `roundedBoxGLSL`. The blur is the
exact Gaussian convolution of a rounded box, computed per pixel without an offscreen pass:

- For one horizontal row of the box, the covered span is known in closed form, with the corner
  arcs shortening it near the top and bottom, so the row's contribution at a point is a
  difference of two error functions. The shader has no `erf`, so `erfApprox` uses a polynomial
  approximation.
- The vertical blur is the same row integral, sampled eight times between plus and minus three
  standard deviations and weighted by a Gaussian.
- Spread adds to each corner radius (clamped to zero and to the box), and each side of a row uses
  its own corner radius, so per-corner radii work.
- With a radius of zero (`sigma` below `0.01`) the shader uses the box's signed distance with
  an anti-aliased edge instead.

The widget's own shape is cut out of the shadow with the same distance function, so a
translucent or transparent widget does not show its shadow through itself. Alpha is the effect's
color alpha times the widget's `effectiveOpacity()`, and the quad is placed with the widget's
`worldMatrix()`, so scale and rotation apply to the shadow and offsets are in the widget's local
logical pixels.

`DropShadow` and `InnerShadow` share a base, `ShadowBase`, which holds the five properties
(`offsetX`, `offsetY`, `radius`, `spread`, `color`), the fluent setters and the repaint wiring.
Each derived class only overrides its stage hook and calls the matching `Renderer` function.

`InnerShadow` draws inside the widget's shape, after the fill and before the children, so it
never affects them. `RectangleWidget::render()` is `renderRectangle`, then `renderInnerEffects`,
then `renderChildren`, and `TextWidget`, `ImageWidget` and `SVGWidget` follow the same pattern;
`renderInnerEffects` returns immediately while a clip mask or a widget's own drawing is being
captured. `innerShadowFragmentShader` runs over a quad exactly the size of the widget and reuses
the same Gaussian code as the drop shadow, which lives in one shared GLSL string,
`blurredBoxGLSL`:

- The shadow is the complement of a blurred hole: the widget's box moved by the offset and
  shrunk by `spread`, so a positive spread grows the shadow inward.
- Coverage is `1 - blurredBox(hole)` multiplied by the widget's own inside mask, so nothing is
  drawn outside the rounded shape.
- With a radius of zero the hole's anti-aliased signed-distance edge is used instead.
- The shadow covers the whole shape, including the border.

Effects of one stage are painted in list order, so the first one added is the lowest. A further
kind of list effect that needs another point in the draw order would add a virtual for its stage.
The blurs and glass, which read what is behind the widget or wrap its whole drawing, are not list
effects: they are `Widget` properties that `renderWidget` handles directly, as shown above.

#### Alpha-based shadows

A widget without a rounded-box shape (text, an image, an SVG) has its shadow computed
from the alpha of its own drawing, in a few offscreen passes. `Renderer::renderDropShadow` and
`renderInnerShadow` fall through to `renderAlphaShadow` when `shapeRadii()` is `nullopt`:

1. **Capture.** `captureOwnDrawing` renders the widget's own `render()` into a transparent
   texture padded on every side by `ceil(3 * sigma) + |spread| + 1` pixels. It sets
   `m_capturingOwn`, which `Renderer::isCapturingOwn()` reports together with mask capture;
   `renderChildren` and `renderInnerEffects` use it to skip children and effects, so the capture
   contains only what the widget itself draws. Unlike mask capture it keeps normal colors. A
   container with no drawing of its own captures nothing and casts no shadow, in line with effects
   never propagating to children.
2. **Spread.** A nonzero `spread` runs a separable square max filter (dilation) or min filter
   (erosion), `effectMorphFragmentShader`, over the alpha. A drop shadow dilates for a positive
   spread; an inner shadow erodes its hole for a positive spread, so the shadow grows inward.
3. **Blur.** `effectBlurFragmentShader` runs a horizontal and a vertical Gaussian pass with
   `ceil(3 * sigma)` taps either side.
4. **Composite.** `alphaDropShadowFragmentShader` draws the blurred alpha, moved by the offset and
   tinted with the shadow color, with the widget's own alpha cut out of it.
   `alphaInnerShadowFragmentShader` draws `1 - blurred hole`, times the widget's own alpha, over
   the widget's area.

The captured alpha already contains the widget's opacity, so the composite shaders divide it back
out (`uOpacity`) before cutting out the shape and multiply the color alpha by it afterward; this
makes the result agree with the analytic path for a translucent widget. Intermediate textures come
from a small pool inside `Renderer` (`acquireTexture` / `releaseTexture`), keyed by size and
trimmed when it grows past a fixed count, so nothing is added to `Widget`. Each pass renders
through the same framebuffer helpers the clip layers use, which save and restore the previous
target, so the passes also work while an ancestor's clip layer is being rendered. The capture is
made at one texture pixel per logical pixel, so a scaled widget gets a correspondingly softer
shadow.

The square structuring element for spread is cheap but not exact: dilating a rounded shape by a
large amount makes its corners slightly squarer than the analytic path does. Everything else
matches the analytic path up to a pixel of anti-aliasing at the shape's edge.

#### Layer blur

`layerBlur` is a `Property<BlurStyle>` on `Widget` rather than an effect, because there is at most
one and it wraps the widget's whole drawing instead of decorating it. `BlurStyle`
(`effects/blurstyle.h`) holds a start radius, an end radius and a direction, with a `lerp`. The
setters follow the shadow API: a `const BlurStyle&`, a `State<BlurStyle>&`, and a `float` shorthand
for a uniform blur. A change requests a repaint.

`Renderer::renderWidget` draws the widget's behind-effects (the drop shadows) first, outside the
blur, and then dispatches on the layer blur. When the larger radius is above a small threshold and
no own-drawing or mask capture is in progress, `renderLayerBlur` runs; otherwise the widget is drawn
normally. The `clip()` branch lives in `renderWidgetContents`, separate from `renderWidget`, so the
blur can capture exactly what a normal render would draw:

1. **Capture.** `captureWidget(..., ownOnly = false)` renders `renderWidgetContents` into a texture
   padded by `ceil(3 * max sigma) + 1` pixels, using the same padded render target and texture
   pool as the alpha-based shadows. This includes the children, a `clip(true)` layer, nested blurs
   and nested clip layers. Because the capture uses the clip-layer blend state, the texture holds
   premultiplied RGBA.
2. **Blur.** `layerBlurFragmentShader` runs a horizontal and then a vertical pass over all four
   premultiplied channels, so transparent pixels contribute no color and no halo appears. The
   radius is computed per output pixel: the pixel's position in the widget's box is projected on the
   direction, the same way a linear gradient paint does, and `sigma = radius / 2` is interpolated
   between the start and end values. Only taps within three sigma of that pixel are read. A uniform
   blur is the case of equal radii, and a progressive one is an approximation, because each pass
   uses the output pixel's sigma for its own axis.
3. **Composite.** `layerBlurCompositeFragmentShader` draws the result at the widget's world matrix
   with premultiplied alpha blending, over a quad that includes the padding. The captured pixels
   already carry the widget's opacity, so no further opacity is applied.

Layout and hit-testing are unaffected, and so are overlays, which the window draws separately.
Nothing is cached: each frame re-renders the subtree into the texture and blurs it.

#### Background blur

`backgroundBlur` is a second `Property<BlurStyle>` on `Widget`, with the same setters as
`layerBlur`. `Renderer::renderWidget` handles it first, before the drop shadows, so the widget's own
shadow is not part of what is blurred. When the larger radius is above the threshold and no
own-drawing or mask capture is in progress, `renderBackgroundBlur` runs:

1. **Copy the backdrop.** The widget's box, padded by the blur reach, is projected through the
   current `projection` and the current viewport to find its bounding rectangle in pixels of the
   active render target, clamped to the viewport. That region is copied out of the framebuffer with
   `glCopyTexSubImage2D` into a pooled texture. Going through the projection and viewport makes the
   same code correct for the window (including HiDPI, where the viewport is in physical pixels), for
   clip layers and layer-blur captures (whose projection is relative to their texture), and for
   rotated, scaled, or tilted widgets. If a corner lies behind the viewer under perspective, its
   projection is meaningless, so the whole viewport is copied instead.
2. **Resample into the widget's space.** `backdropResampleFragmentShader` fills a texture laid out
   like the layer-blur capture, padded around the widget's box, by mapping each texel's local
   position through the saved widget-to-clip matrix, with the perspective divide, back to a pixel of
   the copied region. This makes the next step independent of rotation, perspective, and the
   window's pixel density.
3. **Blur.** `blurLayerTexture`, the same function layer blur uses, runs the two-pass per-pixel-sigma
   Gaussian on that texture, so uniform and progressive blur are shared with layer blur.
4. **Composite.** `backgroundBlurCompositeFragmentShader` draws the widget's box with the blurred
   texture, multiplied by a mask and by the widget's opacity, with premultiplied alpha blending. A
   widget with `shapeRadii()` uses the analytic rounded-box mask; any other widget is rendered
   through the clip-mask machinery by `captureCoverage` (`m_capturingMask`, max blending, the
   shaders' mask mode) and the red channel is the mask. That mode writes pure shape coverage, so the
   mask is independent of the fill color's alpha, unlike the own-drawing capture the alpha-based
   shadows use, whose alpha includes it.

The copy and the resample pass live in `captureBackdrop`, which background blur and glass share. The
resample shader forces alpha to 1 when the target is the window's framebuffer (`uOpaque`), because
that alpha is not meaningful: the renderer's blend function also blends alpha with the source alpha,
so antialiased edges leave it below 1, and the later premultiplied compositing would let the
destination show through at those pixels. Inside a clip layer or a layer-blur capture the texture
holds real premultiplied alpha, which is kept.

The copy reads whatever the current render target holds at that moment, so inside a clip layer or an
ancestor's layer blur it sees only that texture's content so far. `EffectlessWidget` also hides
`backgroundBlur`, for the same reason it hides the shadows: a control or layout has no single shape
to mask to, and the blur goes on the slot or child that draws.

#### Glass

`glass` is a `Property<GlassStyle>` plus a `Property<bool>` that says whether it is on, because a
default style is a valid glass. `GlassStyle` (`effects/glassstyle.h`) has `lightAngle`,
`lightIntensity`, `refraction`, `depth`, `dispersion`, `frost`, `splay`, `zoom`, `thickness`,
`bevel`, `rimWidth`, and the optional `tint`, `saturation` and `brightness`, with a `lerp`. It follows the same setter pattern as the blurs, and
`EffectlessWidget` rejects `glass` and `clearGlass`.

`renderWidget` checks glass before background blur and, when the widget has glass and a
`shapeRadii()`, calls `renderGlass` instead; a widget without a shape falls back to its background
blur. `renderGlass` shares `captureBackdrop` and `blurLayerTexture` with background blur: the
backdrop is copied and resampled into the widget's padded space, blurred with a uniform radius equal
to `frost`, and then drawn by `glassCompositeFragmentShader` over the widget's box with premultiplied
alpha blending and the widget's opacity. Everything specific to glass is in that shader, using the
analytic rounded-box distance (with the radii clamped to half the smaller side, which gives circles
and pills):

- **Edge normal.** The outward normal is the normalized gradient of the signed distance, taken by
  central differences. `inside = max(-distance, 0)` is how far in from the edge a pixel is.
- **Refraction.** The bevel is modelled as a circular arc of width `depth`. With `s = inside / depth`
  clamped to 0..1 and raised to the power `bevel`, the surface normal is tilted from the view
  direction by an angle `theta` with `sin(theta) = 1 - s`. The refracted angle follows Snell's law
  with an index `n = 1 + 0.8 * |refraction|`, `sin(theta_r) = sin(theta) / n`, and the backdrop is
  sampled displaced along the normal by `depth * thickness * tan(theta - theta_r)`: inward for a
  positive `refraction`, outward for a negative one. The shift is largest at the edge, falls to zero
  at `depth`, and is zero everywhere when `refraction` is 0 because then `n = 1`. The shift is also
  multiplied by `1 - smoothstep(0.35, 1, s)`, so it reaches zero with zero slope at the inner edge of
  the bevel; without that, the bent image has a crease there, which shows as a sharp corner where an
  edge behind the glass meets the rim. A `bevel` above 1 keeps the shift near its edge value across
  most of the bevel and drops it sharply near the inner edge, which gives a harder inner contour, and
  `thickness` scales the shift without changing the bevel's width.
- **Zoom.** Every sample position is also moved by `p * (1 / zoom - 1)`, where `p` is the pixel's
  position relative to the center of the shape, which scales the whole backdrop about that center.
- **Dispersion.** The three color channels use displacements scaled by `1 - spread`, `1` and
  `1 + spread` with `spread = dispersion * 0.25`, which separates them near the edge.
- **Color and tint.** Saturation mixes between the luminance and the color, brightness multiplies
  it, and the tint is mixed in by its alpha.
- **Highlight.** The light direction is `(sin(lightAngle), -cos(lightAngle))`, so 0 points to the top.
  The highlight is `lightIntensity * band * (0.15 + primary + secondary)`, where `band` falls off
  exponentially with `inside` over a width that grows from `rimWidth` to `0.45 * depth` with `splay`,
  `primary` is `max(dot(normal, light), 0)` raised to a power that falls from 6 to 1.5 with `splay`,
  and `secondary` is the same lobe on the opposite side at 60% strength. It is combined with a
  screen blend, so it brightens without exceeding white.

The sampling margin around the widget grows with the largest possible shift, `depth * thickness *
sqrt(n^2 - 1)`, and with the zoom reach, `|1 / zoom - 1|` times the half diagonal, so the displaced
and zoomed reads always have real backdrop pixels to find.

The mask is the analytic rounded box, multiplied by the widget's opacity. A glass inside a clip layer
or a layer-blur capture sees only that texture's content, like a background blur.

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

`AnimatableBase` doesn't have to be inherited publicly the way `AnimatedState<T>` does it -
`TextFieldBase`'s blinking caret inherits it privately (`class TextFieldBase : public
Control, private AnimatableBase`), registering/unregistering itself with
`AnimationManager::instance()` from its own member functions. The private inheritance is legal
because the upcast to `AnimatableBase*` needed for `registerAnimation`/`unregisterAnimation`
happens from inside `TextFieldBase`'s own methods, and it keeps the animation machinery out
of the class's public interface entirely - tighter encapsulation than `AnimatedState`'s public
shape, useful whenever the animated thing isn't itself a value a consumer should bind to.

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

### Transitions

`Transition` (`animation/transition.h`) animates one widget through a short lifecycle that a runner
drives: `prepare(item, containerWidth, containerHeight)` captures the widget's own values and sets
the start state, `update(progress)` applies the eased progress from 0 to 1, and `finish(completed)`
puts the captured values back. A transition object keeps its own state, so subclasses can
remember whatever they change. Its constructor takes only parameters (a duration and an
easing); it never touches a widget. A `TransitionFactory` (`std::function<std::unique_ptr<
Transition>()>`) creates a fresh instance for each use.

`TransitionRunner` (`animation/transitionrunner.h`) runs all the transitions of one operation on a
single clock, as an `AnimatableBase`. `start(entries, width, height, isAlive, onFinished)` calls
`prepare` on each entry and registers with the animation manager. Each frame it calls `update` with
that entry's own eased progress, and when the longest duration has elapsed it calls `finish(true)`
on every entry and then `onFinished`. `finishNow()` jumps every entry to progress 1 first, which is
what an operation arriving mid-transition does. A runner with only zero-duration entries completes
immediately without registering. `isAlive(Widget*)` is a callback the owner provides; the runner
asks it before touching an entry, so a widget removed by someone else mid-transition is skipped
instead of dereferenced.

The presets (`animation/transitions.h`) are plain subclasses: `FadeIn`, `FadeOut`, `ScaleIn`,
`ScaleOut`, `SlideIn(Edge)`, `SlideOut(Edge)` and a `LambdaTransition` that wraps a function. The
slides move a widget by adding `+d` to one margin and `-d` to the opposite one, which shifts it
without changing its size and without touching its alignment, and `finish` restores the captured
margins. The fades and scales set `opacity` and `scale` and restore them the same way. A binding
on one of those properties is replaced for the duration and then replaced by the captured plain
value.

Overlays run the same transitions on their own widget for opening and closing (see
[Overlays and popups](#overlays-and-popups)).

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
Widget* Widget::hitTestTree(float px, float py, const Widget* skip) const {
    if (!visible() || this == skip) return nullptr;
    if (m_clip && !hitTest(px, py)) return nullptr;
    const std::vector<Widget*> ordered = zOrderedChildren(*this);
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it)   // reverse: highest z / last-drawn first
        if (auto* hit = (*it)->hitTestTree(px, py, skip))
            return hit;
    if (hitTest(px, py))
        return const_cast<Widget*>(this);
    return nullptr;
}

bool Widget::hitTest(float px, float py) const {
    const Point localPoint = mapFromWindow({px, py});
    return localPoint.x >= 0.0f && localPoint.x <= m_displayedWidth &&
           localPoint.y >= 0.0f && localPoint.y <= m_displayedHeight;
}
```

Children are walked in the *reverse* of render order (the same `zOrderedChildren()` list
used for rendering, reversed) - the topmost widget for painting is also the one that wins an
overlapping hit-test, matching rendering exactly. `hitTest` maps the screen-space point into
the widget's *local* space with `mapFromWindow`, the inverse of `worldMatrix()` followed by the
perspective divide (see [Coordinate mapping and mouse dispatch](#coordinate-mapping-and-mouse-dispatch)),
so rotation, scale and tilt (applied in `localMatrix()`, see below) are accounted for, not just
axis-aligned bounding boxes - then bounds-checks against `m_displayedWidth/Height`. `skip`, null by
default, leaves one widget and everything under it out, as if it were hidden; the drop search
passes the widget being dragged (see [Drag and drop](#drag-and-drop)).

```cpp
glm::mat4 Widget::localMatrix() const {
    glm::vec3 position{m_displayedX, m_displayedY, 0.0f};
    const Point origin = m_transformOrigin.get();
    glm::vec3 pivot{m_displayedWidth * origin.x, m_displayedHeight * origin.y, 0.0f};
    glm::mat4 eye{1.0f};
    if (m_perspective > 0.0f)
        eye[2][3] = -1.0f / m_perspective;
    return flattenToPlane(translate(position) * translate(pivot) * eye
                          * rotateX(m_rotationX) * rotateY(m_rotationY) * rotateZ(m_rotation)
                          * scale(m_scale * m_scaleX, m_scale * m_scaleY) * translate(-pivot));
}
void Widget::updateWorldMatrix() {
    glm::mat4 parentWorld = /* parent Widget's worldMatrix(), or identity */;
    m_worldMatrix = parentWorld * localMatrix();
}
```

Read the product from right to left: the widget is moved so the pivot (its transform origin, a
fraction of its size) is at the origin, scaled, rotated in the plane, tilted around Y and then X,
given perspective, and moved back and into place. The tilt gives each point a depth `z`, and `eye`
makes the output `w = 1 - z / perspective`; the GPU divides by `w`, so points tilted away from the
viewer shrink toward the pivot, which is therefore also the vanishing point.

`flattenToPlane` then resets the matrix's `z` row and column to identity, leaving a 3x3 projective
map on `(x, y, w)`. That does two things. Each widget becomes a flat picture projected onto its
parent's plane: a child's tilt is drawn on that plane, and its depth is not projected a second time
by an ancestor's perspective, so products of local matrices stay exact. And `z` stays `0`, inside the
orthographic projection's `-1`..`1` range, so a tilted widget is not clipped by the near and far
planes. The clip-layer and capture paths need nothing new: they render children with
`ortho * inverse(worldMatrix)`, which still lands them in the widget's local space, and composite
the result with `worldMatrix`, which draws it in perspective with perspective-correct texture
coordinates.

`worldMatrix()` composes with the parent's, so nested transformed widgets transform correctly
relative to their ancestors - both for rendering (the vertex shader uses `worldMatrix` directly)
and for hit-testing (its inverse). `Renderer::renderWidget` skips a widget, with its subtree, whose
world matrix has a near-zero determinant: a widget seen edge-on, at exactly 90 degrees, has no area,
and the clip-layer and capture paths would otherwise invert a singular matrix.

Overlays are the one exception to the tree walk. `hitTestTree` skips overlay children, and
`Window::hitTestChildren` tests the overlay stack first, topmost first. If a modal overlay is
tested and nothing inside it is hit, the point counts as a miss and nothing beneath it is
reachable (see [Overlays and popups](#overlays-and-popups)).

A widget with `clip(true)` returns a miss for points outside its own rectangle without testing
its children. Children that are clipped out of view therefore cannot catch input meant for
whatever is visible at that spot, which matters for scrolled content.

### Hover

Hover is a state shared by every widget that contains the pointer, not an event that one widget
consumes. `Window` keeps the hovered widgets in `m_hoveredChain` and rebuilds it on every cursor
move from the hit-test result:

```cpp
std::vector<Widget*> Window::hoverChainAt(Widget* hit, const Point& windowPoint) {
    std::vector<Widget*> chain;
    for (Widget* widget = hit; widget; widget = widget->isOverlay() ? nullptr : dynamic_cast<Widget*>(widget->parent())) {
        if (widget->hitTest(windowPoint.x, windowPoint.y))
            chain.push_back(widget);
    }
    return chain;
}
```

The chain is the hit widget and its ancestors, with two rules:

- A widget is in the chain only while the pointer is inside its own rectangle, so a child that
  extends outside its parent hovers the child, not the parent.
- An overlay is a hover root: the walk stops after it, so the pointer over a popup does not hover
  the widget the popup belongs to.

A widget covered by another one is not hovered, since hit-testing returns the covering widget
and the chain follows that widget's ancestors.

`updateHover` compares the new chain with the previous one. Widgets that dropped out get
`MouseLeave`, innermost first, and widgets that joined get `MouseEnter`, outermost first. Each
event is delivered straight to its widget, with the position mapped into that widget's space,
when `hasHandlerFor` claims the type. Nothing bubbles and accepting an event stops nothing, so
nested widgets each get their own enter and leave. Moving between a widget's children keeps it in
both chains, so it gets no events. `Control` keeps `hovered()` in step from these two events.

The chain is rebuilt only when the cursor moves: a widget that moves or hides under a still
pointer keeps its hover state until the next move. Removed widgets are erased from it by
`clearReferencesTo`, and `updateHover` skips an enter for any widget no longer in the chain, so a
widget removed by an outer widget's enter handler gets no enter of its own.

### Cursor

Every widget has a `cursor` property, a `CursorShape` that defaults to `Inherit`. `Window::updateCursor`
picks the shape from a chain of widgets, innermost first: the first widget that does not say `Inherit`
decides, and with none the cursor is the arrow. While a mouse button is held, the chain is
`m_pressedChain`, built with `hoverChainAt` at the press and kept until release, so a drag keeps its cursor
after the pointer leaves the widget; otherwise it is the hovered chain (see [Hover](#hover)). Using the
same chain as hover means the cursor follows the same rules: a child outside its parent's rectangle does
not take the parent's cursor, and a popup does not take its anchor's.

`updateCursor` runs after every cursor move, press and release, and from the `cursor` property's change
handler, so a cursor set from code shows without the pointer moving. It calls `glfwSetCursor` only when
the shape changes. `cursorFor` creates each GLFW standard cursor the first time it is needed and keeps it
in a per-window map; `Arrow` passes `nullptr`, GLFW's default cursor. `~Window` destroys the map's cursors
after the GLFW window. `clearReferencesTo` erases removed widgets from the pressed chain as it does from the
hovered one.

### Press / release / click / double-click

```cpp
void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Widget* const hit = hitTestChildren(self, mx, my);

    if (action == GLFW_PRESS) {
        self->m_pressedWidget = hit;
        if (hit) {
            Widget* const target = clickTarget(hit);
            const bool isRepeat = target == self->m_lastClickWidget &&
                (now - self->m_lastClickTime) <= kDoubleClickTimeThreshold &&
                (dx*dx + dy*dy) <= kDoubleClickDistanceThreshold * kDoubleClickDistanceThreshold;
            self->m_clickCount = isRepeat ? self->m_clickCount + 1 : 1;
            self->m_clickTarget = target;

            pressEvent.setClickCount(self->m_clickCount);
            dispatchMouseBubble(hit, pressEvent, windowPoint, &Widget::triggerPress);
            if (isRepeat)
                dispatchMouseBubble(hit, doubleClickEvent, windowPoint, &Widget::triggerDoubleClick);
        }
    } else if (action == GLFW_RELEASE) {
        if (self->m_pressedWidget)
            dispatchMouseBubble(self->m_pressedWidget, releaseEvent, windowPoint, &Widget::triggerRelease);

        if (hit && self->m_clickTarget && clickTarget(hit) == self->m_clickTarget && !dragHandled) {
            dispatchMouseBubble(hit, clickEvent, windowPoint, &Widget::triggerClick);
            if (self->m_clickTarget) {
                self->m_lastClickWidget = self->m_clickTarget;
                self->m_lastClickTime = glfwGetTime();
                self->m_lastClickX = static_cast<float>(mx);
                self->m_lastClickY = static_cast<float>(my);
            }
        }
        self->m_pressedWidget = nullptr;
        self->m_clickTarget = nullptr;
    }
}
```

Note `MouseRelease` dispatches to `self->m_pressedWidget` (the widget that was actually
pressed), not to a fresh `hit` at the release position - this is intentional mouse-capture
behavior: dragging off a pressed widget before releasing still correctly delivers the release
to the originally pressed widget, rather than to whatever happens to be under the cursor now.

A press and a release make a click when they have the same **click target**: the nearest widget,
from the hit upward, that handles presses, clicks or double-clicks, or the hit itself when none
does:

```cpp
Widget* Window::clickTarget(Widget* hit) {
    for (Widget* widget = hit; widget; widget = dynamic_cast<Widget*>(widget->parent())) {
        if (widget->hasHandlerFor(EventType::MousePress) || widget->hasHandlerFor(EventType::MouseClick)
            || widget->hasHandlerFor(EventType::MouseDoubleClick))
            return widget;
    }
    return hit;
}
```

Comparing targets instead of hit widgets means a press on a button's label and a release on its
background still click the button, and a text field's caret, which the press moves under the
pointer, doesn't take the click from the field. Releasing over a different control still makes no
click. The click event itself goes to the widget under the pointer and bubbles as usual. The
`m_clickTarget` check after dispatching it covers a click handler that removed the target, which
`clearReferencesTo` nulls.

Repeated clicks are counted by the same target. A press continues a series when it has the last
click's target and comes within `kDoubleClickTimeThreshold` (0.4 s) and
`kDoubleClickDistanceThreshold` (5 px) of it; `m_clickCount` then goes up, and otherwise restarts at
1. The press, double-click, release and click events of that press carry it as
`MouseEvent::clickCount()`. `MouseDoubleClick` is sent for every repeat press, so a widget that needs
a triple-click reads the count. `ButtonBase` builds its own click event on release and copies the
count into it.

A click is also not sent after a drag that some widget handled. On release, the result of
dispatching `DragEnd` is kept: if a widget accepted it, the release is still delivered but no
`MouseClick` follows, so dragging scrollable content does not click the item under the pointer.
Presses that are never handled as drags still click as usual.

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
(`Shift+Tab` reverses); other keys go through the shortcut check (see [Shortcuts](#shortcuts)) and
then to `m_focusedWidget`, if one is set. The
chain is recomputed from scratch on every Tab press rather than cached - simple, and cheap
enough in practice for typical UI sizes. While a modal overlay is open, the chain is collected
from that overlay's subtree only, so Tab never leaves it.

### Focus memory

A widget that temporarily takes over focus, such as a popup or an item of a stack view, needs to
give focus back to whatever had it. `Window` keeps a list of `(key, saved)` pairs, `m_focusMemory`,
where `key` is the widget that took over and `saved` is the widget that was focused when it did:

| Call | What it does |
|---|---|
| `rememberFocus(key)` | Stores `(key, current focus)` unless `key` already has an entry. |
| `focusFirstIn(root)` | Focuses the first focusable widget inside `root`; if there is none, focus does not change. |
| `restoreFocus(key)` | Removes `key`'s entry. If the focused widget is `key` or a descendant of it, focuses the saved widget (or clears focus if there is none). If focus has already moved elsewhere, it is left alone. |
| `transferFocusMemory(from, to)` | Moves `from`'s saved widget to `to`, overwriting or creating `to`'s entry. |

`restoreFocus` only acts while focus is still inside `key`, so a click that moved focus elsewhere
is respected, and it must run before `key` is removed, because removing the subtree clears focus
and the entry. `clearReferencesTo` also removes entries whose key was removed and nulls saved
widgets that were removed, so a restore never focuses a dead widget. Overlays use these calls when
they open and close, and stack views use them when items are pushed, popped and replaced.

### Shortcuts

**`KeySequence`** (`events/keysequence.h`) is a `Key` with its `KeyModifier`s, compared exactly.
`fromString` (`src/events/keysequence.cpp`) splits text on `+`: every part but the last must be a
modifier name and the last a key name, both case-insensitive; anything else is logged and gives an
empty sequence, which matches nothing. `toString` writes the modifiers in a fixed order (Ctrl, Alt,
Shift, Meta) and the first name of the key.

**`Shortcut`** (`events/shortcut.h`) is a plain `Object` (see
[Building the tree](#building-the-tree)), not a widget, so it takes no layout slot and is
skipped by every traversal. Its constructor walks up its parents to the nearest widget, whose
`ownerWindow()` it takes, or to the window itself, and registers there with `addShortcut`; the
destructor calls `removeShortcut`. Removing it, or a widget above it, goes through
`clearReferencesTo`, which erases it from `m_shortcuts` at once, before the deferred destruction.
`matches(sequence, repeat)` checks that it is enabled and not empty, that the sequence is equal, that
a key repeat is allowed by `autoRepeat`, and that every widget above it is visible.

A key press or repeat that is not Escape for a popup or Tab reaches this, in `keyCallback`:

```cpp
KeyEvent event{EventType::KeyPress, key, km};
if (!self->claimsShortcut(event) && self->runShortcut(KeySequence{km, static_cast<Key>(key)}, action == GLFW_REPEAT))
    return;
if (self->m_focusedWidget)
    dispatchBubble(self->m_focusedWidget, event, &Widget::triggerKeyPress);
```

`claimsShortcut` sends an `EventType::ShortcutOverride` event to the focused widget and then each
ancestor that has a handler, through the virtual `triggerShortcutOverride`, which runs
`onShortcutOverride`. The event starts unaccepted, and the first widget to accept it claims the key:
the `&&` then skips `runShortcut`, and the key goes to the focused widget as usual.

`runShortcut` collects the matching shortcuts of every window: `Application` shortcuts from any
window, `Window` shortcuts only from the window that got the key, and only if `reachesShortcut`
allows them. That finds the topmost overlay with `modalActive()`, and with one open, accepts only
shortcuts inside it. One match runs `onActivated`; several run `onActivatedAmbiguously` on each, and
none activates. Either way the key is consumed. With no match, the key goes to the focused widget.
Key releases are not checked.

## Interaction state & control widgets

### `Control` and the behavior bases

Every concrete control (Button, Checkbox, Radio, Switch, ProgressBar, Slider, TextField,
SpinBox) is built in two layers: a **behavior base** that owns the state and interaction logic,
and a **concrete widget** that supplies only visuals and style. A custom control derives the
base and fills in slots - it never reimplements toggling, dragging, stepping, or editing.
Slot bodies decide their own layout; no base imposes alignment or position on what it's given.
The bases live in `widget/templates/` (`templates/templates.h` includes them all) and the concrete
widgets in `widget/controls/`, with their style structs in `widget/controls/style/`.

**`Control`** (`src/include/tavoos/widget/templates/control.h`) is the common root. It gives a widget two
replaceable children - `background` and `content` - plus `enabled()` and `hovered()`:

```cpp
template<typename W = RectangleWidget>
decltype(auto) background(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
    self.template installBackground<W>(std::move(body));
    return std::forward<decltype(self)>(self);
}

template<typename W = RectangleWidget>
decltype(auto) content(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
    self.template installContent<W>(std::move(body));
    return std::forward<decltype(self)>(self);
}
```

Each slot is a normal child widget (`RectangleWidget` unless another type is given, e.g.
`.content<TextWidget>(...)`), installed via `addChild<W>`; `background` additionally gets
`z(-1)` and `Fill::Both` so it paints first and fills the control, while `content` gets no
layout at all - its body decides. Calling a slot again replaces the previous one (`replaceSlot`,
via deferred destruction - see
[Removing widgets](#removing-widgets-two-phase-deferred-destruction)) and bumps
`contentRevision()`, which a subclass that rebuilds its own default content (e.g. Button
rebuilding an icon+label Row) checks to avoid clobbering a slot the user has replaced.

`Control` claims `MouseEnter`/`MouseLeave` in `hasHandlerFor` and keeps `hovered()` in step, and
`enabled()` is a `Property<bool>` mirrored into `enabledState()`. It deliberately does not make
itself focusable: only bases that are interactive set `focusable(true)`, and keep it in step
with `enabled()` themselves (a ProgressBar or a composite's container must not become a Tab
stop just because it was re-enabled).

**`ButtonBase`** (`templates/buttonbase.h`) adds `pressed()`, click and keyboard activation, and the
toggle state: `checkable()` (default `false`), `checked()`, `exclusive()`, and `group()`:

```cpp
void ButtonBase::handleClick(MouseEvent& event) {
    if (m_checkable && enabled()) {
        if (m_group || m_exclusive) {
            if (!m_checked.get()) {
                if (m_group)
                    m_group->select(this);
                else
                    m_checked.set(true);
            }
        } else {
            m_checked.set(!m_checked.get());
        }
    }
    Widget::triggerClick(event);
}
```

Checkbox and Switch are `checkable(true)`; Radio is `checkable(true).exclusive(true)`; a plain
Button is neither. In a `ButtonGroup` (or when `exclusive`) a click only ever checks, and
`ButtonGroup::select` unchecks every other member - so any checkable button, not just a Radio,
can be part of a mutually exclusive set (a segmented control is just checkable Buttons in a
group). A group tracks its members' `checkedState()` to expose `checked()`, the current pick, and
keeps no dangling pointers: a button removes itself from its group when it is destroyed or moved to
another group, and a destroyed group detaches its remaining members, which simply become ungrouped.

`hasHandlerFor` claims press, release, click, and key events unconditionally, so a button always
accepts them at the bubble step it's reached at (see [Bubbling](#bubbling)) and never lets a click
fall through to something behind it. Space and Enter both route through one `sendClick`, so
keyboard and mouse activation share one code path:

```cpp
void ButtonBase::triggerKeyPress(KeyEvent& event) {
    const int key = event.keyCode();
    if (key == static_cast<int>(Key::Space)) { m_spaceDown = true; m_pressed.setIfChanged(true); }
    else if (key == static_cast<int>(Key::Enter) || key == static_cast<int>(Key::KpEnter))
        sendClickFromKeyboard(event.modifiers());
    else event.ignore();
    Widget::triggerKeyPress(event);
}
```

`triggerClick` itself is a deliberate no-op on `ButtonBase`; clicks are driven through
`sendClick`/`handleClick` (called from `triggerRelease` for the mouse path and
`sendClickFromKeyboard` for Space/Enter), so a subclass overrides the `protected virtual
handleClick(MouseEvent&)` hook to react to "a real click happened" regardless of input method,
rather than `triggerClick` (which `Window`'s own click dispatch also targets and would
double-fire).

**`RangeBase`** (`templates/rangebase.h`) is the shared base of the three range controls. It owns
`value`/`minValue`/`maxValue`, `valueState()`, and a normalized `position` (0-1) published as
`positionState()`. The protected `commitValue(long long)` clamps to the range and sets the value
only if it changed, returning whether it did. The protected virtual `onRangeChanged()` runs after
the value or either bound changes. `RangeBase` is itself a `Control`, so every range control has
`enabled` and `hovered` too.

**`ProgressBarBase`** (`templates/progressbarbase.h`) is `RangeBase` under its own name. It has no
visuals; a concrete bar binds its fill to the position, e.g. `fill.widthFraction(positionState())`,
and layout does the pixel math
(see [`Widget::layout()`](#widgetlayout---the-non-layouter-default)).

**`SliderBase`** (`templates/sliderbase.h`) adds `pressed` and a `handle` slot to the range.
Interaction lives entirely in the base: press, release, and drag events bubble up from
whichever child was hit (the handle or the track), and the base claims them in `hasHandlerFor`,
so no child needs a callback. Pressing the track sets the value, pressing the handle does not
jump, and a drag continues from the value held when it started:

```cpp
void SliderBase::triggerDragMove(DragEvent& event) {
    if (!enabled())
        return;

    const long long span = detail::spanOf(minValue(), maxValue());
    const double valuePerPixel = static_cast<double>(span) / static_cast<double>(travel());
    const long long newValue = static_cast<long long>(m_dragStartValue) + std::llround(event.totalDx() * valuePerPixel);
    commitValue(newValue);
    Widget::triggerDragMove(event);
}
```

`travel()` is the displayed width minus the registered handle's displayed width, read at event
time. The `handle` slot only registers the widget (so the base knows its size and bounds); the
concrete slider positions it, binding `xFraction(positionState())` so the handle tracks the
value.

**`SpinBoxBase`** (`templates/spinboxbase.h`) adds `step`, `increase()`/`decrease()`, and
`commitText(string)` to the range. `commitText` parses typed input, clamps it, and commits it - an
unparseable entry re-notifies `valueTextState()` with the current text so the display reverts.
It overrides `onRangeChanged()` to keep `valueTextState()` current.
It has `up` and `down` slots; the base attaches the click handler that adjusts the value to
whatever widget is installed (a button swallows clicks, so a position test on bubbled events
would never see them). The content item, typically a text field, binds to `valueTextState()` and
calls `commitText`; the base knows nothing about `TextField`.

**`TextFieldBase`** (`templates/textfieldbase.h`) follows the QML `TextField` model instead of exposing
slots: the user supplies only a `background`, and the base builds its own clipped viewport, text
item, placeholder, and caret internally. The editing itself is a `TextEditModel` that the base
feeds keys and pointer positions into (see [Text editing](#text-editing)); the base owns what the
model doesn't know about: horizontal scrolling, echo modes, and the caret blink (see the note on
private `AnimatableBase` inheritance under [Animation](#animation-system)). Everything about how the
text looks is a property of the base (`textColor`, `placeholderColor`, `caretColor`, `font`,
`innerPadding*`, applied as margins on the internal viewport), never a user-supplied visual, and
`content()` is hidden so the internals can't be replaced. The viewport clips to its own rect, so
the text and caret can never draw outside the padded interior.

**`OverlayBase`** and **`PopupBase`** (`templates/overlaybase.h`, `templates/popupbase.h`) are
the bases for window-level layers such as popups; they are covered in
[Overlays and popups](#overlays-and-popups).

**`FlickAreaBase`** (`templates/flickareabase.h`) is the base for scrolling content; see
[Flick areas](#flick-areas).

**`ScrollAreaBase`** (`templates/scrollareabase.h`) adds scrollbars to a flick area; see
[Scroll areas](#scroll-areas).

**`ListViewBase`** (`templates/listviewbase.h`) is a scroll area over rows built on demand; see
[List views](#list-views).

**`StackViewBase`** (`templates/stackviewbase.h`) is a stack of items with animated transitions;
see [Stack views](#stack-views).

**`SwipeViewBase`** (`templates/swipeviewbase.h`) lays out its children as pages and snaps between
them; see [Swipe views](#swipe-views).

`focused()`/`focusedState()` are the one piece of interaction state that lives on `Widget`
itself (see [Click-to-focus](#click-to-focus) below), since focus is meaningful for any widget.

### Click-to-focus

`Widget::triggerFocusIn`/`triggerFocusOut` update reactive state by default:

```cpp
virtual void triggerFocusIn(Event& event)  { m_focused.setIfChanged(true);  if (m_onFocusIn) m_onFocusIn(event); }
virtual void triggerFocusOut(Event& event) { m_focused.setIfChanged(false); if (m_onFocusOut) m_onFocusOut(event); }
```

exposed as `bool focused() const` / `State<bool>& focusedState()`. A click focuses the
nearest focusable ancestor of whatever was actually hit, mirroring how bubbling itself walks
up looking for a handler:

```cpp
static Widget* nearestFocusable(Widget* start) {
    Widget* current = start;
    while (current) {
        if (current->focusable()) return current;
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}
```

called from `mouseButtonCallback`'s press handling, before dispatch: `if (Widget* const
focusTarget = nearestFocusable(hit)) focusTarget->focus();`. This is why clicking a button's
inner label still focuses the *button* - `hit` is often a non-focusable descendant (the label
widget itself isn't `focusable()`), so the walk continues up to the nearest ancestor that is.
`focus()` reuses the same `Window::setFocusedWidget` path Tab navigation already goes
through, so both agree on exactly one focused widget at a time.

### Coordinate mapping and mouse dispatch

`Widget` exposes four coordinate-space conversions, each a thin wrapper around
`localMatrix()`/`worldMatrix()` (see [Hit-testing](#hit-testing)) or their inverse:

```cpp
static Point projectPoint(const glm::mat4& m, const Point& point) {
    const glm::vec4 p = m * glm::vec4{point.x, point.y, 0.0f, 1.0f};
    if (!(p.w > 0.0f))
        return { NaN, NaN };
    return { p.x / p.w, p.y / p.w };
}
Point Widget::mapToParent(const Point& point) const { return projectPoint(localMatrix(), point); }
Point Widget::mapFromParent(const Point& point) const { return projectPoint(glm::inverse(localMatrix()), point); }
// mapToWindow / mapFromWindow are the same shape, using worldMatrix() instead of localMatrix()
```

Dividing by `w` applies the perspective, and the inverse of a projective map undoes it exactly, so a
click lands on the right spot of a tilted widget. A result with `w <= 0` lies behind the viewer;
`NaN` fails every bounds comparison, so such a point never hits anything, and `!(p.w > 0.0f)` also
catches a `w` that is already `NaN`.

(`Point{float x, y}` - a plain pair type in `types.h`, with equality and `lerp` so it also works
in `State` and `AnimatedState`, kept out of the public API to avoid leaking `glm` types into
widget-facing signatures.) Rule of thumb: apply the matrix to go
*outward* (local space to parent/window space), its inverse to go *inward*.

Every `MouseEvent` a widget's handler receives carries coordinates relative to *that widget*,
not raw window coordinates - `Window::dispatchMouseBubble` recomputes the position at each
step of the bubble, not once against the original hit target:

```cpp
Widget* Window::dispatchMouseBubble(Widget* start, MouseEvent& event, const Point& windowPoint,
                                     void (Widget::*trigger)(MouseEvent&)) {
    Widget* current = start;
    while (current) {
        if (current->hasHandlerFor(event.type())) {
            event.setPosition(current->mapFromWindow(windowPoint));
            event.accept();
            (current->*trigger)(event);
            if (event.isAccepted()) return current;
        }
        current = dynamic_cast<Widget*>(current->parent());
    }
    return nullptr;
}
```

This matters because the widget that's actually *hit* (deepest match) is often not the widget
that ends up *handling* the event (the nearest ancestor with a handler, per
[Bubbling](#bubbling)) - recomputing per-step means a handler always sees coordinates
relative to itself, never relative to whatever leaf happened to be hit. `dispatchMouseBubble`
is used for the positional mouse events press/release/click/double-click/move; enter and leave
do not bubble (see [Hover](#hover)). The generic `dispatchBubble` (no position) still handles
Wheel/KeyPress/KeyRelease/TextInput.

### Dragging

`draggable(bool)`, `dragThreshold(float)`, and `dragXAxis`/`dragYAxis(DragAxis{enabled, min,
max})` on `Widget` itself - opt in on any widget, not just controls. `DragAxis` is a plain
aggregate (`types.h`), so `dragXAxis`/`dragYAxis` take it directly (with a `State<DragAxis>&`
overload for binding) rather than the generic `PropertyArg<T>` every scalar setter uses -
`PropertyArg<DragAxis>` would reject `.dragXAxis({.enabled = true, .min = 0, .max = 300})`,
since a designated-initializer braced-list needs two implicit conversions (braced-list to
`DragAxis`, then `DragAxis` to `PropertyArg<DragAxis>`) and C++ only allows one. See
[Theme and style structs](#theme-and-style-structs) below for the same fix applied to every
`*Style` struct.

`DragEvent` carries both the incremental delta and the cumulative total since the drag
started:

```cpp
class DragEvent : public Event {
public:
    float dx() const; float dy() const;         // since the last move
    float totalDx() const; float totalDy() const; // since drag start
};
```

`Widget::triggerDragMove`'s default body only moves the widget itself when `draggable(true)`
was set, clamping to each enabled axis's `min`/`max`:

```cpp
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
```

`dragBase(horizontal)` returns the offset the delta is added to. For an axis with an explicit `x`/`y` it is
just that property. For an axis placed by alignment or a fraction it is the widget's resolved position minus the
parent's content origin and the widget's margin, which is what layout adds to `x`, so the widget starts moving
from where it is drawn. It also clears the fraction on that axis, because layout prefers a fraction over `x`; the
`x(...)`/`y(...)` call that follows replaces the alignment with the explicit position.

The `m_onDragMove` callback always runs, independent of the `m_draggable` guard - so a widget
can hook `onDragStart`/`onDragMove` *without* ever calling `draggable(true)`, receiving drag
events but computing its own constrained position instead of the free-translate default. This
is how `SliderBase` is built: drag events bubble from the handle or the track to the slider,
whose own `triggerDragMove` derives a clamped value from `event.totalDx()` against the value
held at drag start - nothing sets `draggable(true)`.

### Drag and drop

**The source.** `Widget` stores `m_dragKeys`, `m_dragData` (`DragData`, a
`std::map<std::string, std::string>` from `events/dropevent.h`) and `m_dragHotSpot`, a
`Property<Point>` holding a fraction of the widget's size. When a drag starts, `cursorPosCallback`
keeps the widget that handled `DragStart` as the window's `m_dragSource`, if it is draggable and
has keys:

```cpp
Widget* const handler = dispatchBubble(self->m_pressedWidget, dragStartEvent, &Widget::triggerDragStart);
if (handler && handler->draggable() && !handler->dragKeys().empty())
    self->m_dragSource = handler;
```

**The target.** `DropAreaWidget` (`widget/droparea.h`) is a widget that only draws its children. It
holds its keys, its callbacks, `m_containsDrag` and `m_dragSource`, and `Window`, a friend, drives
it. `acceptsKeys` is true when the area has no keys or shares one with the drag. `DropEvent` uses
three event types, `DropEnter`, `DropMove` and `Drop`, and starts accepted. After every `DragMove`,
the window calls `updateDropTarget`:

```cpp
void Window::updateDropTarget() {
    layoutTree();
    const Point point = dragHotSpotInWindow();
    std::vector<DropAreaWidget*> candidates;
    for (Widget* const widget : hoverChainAt(hitTestChildren(this, point.x, point.y, m_dragSource), point)) {
        if (auto* const area = dynamic_cast<DropAreaWidget*>(widget); area && area->acceptsKeys(m_dragSource->dragKeys()))
            candidates.push_back(area);
    }
    std::erase_if(m_dropRejected, [&candidates](DropAreaWidget* area) { return !std::ranges::contains(candidates, area); });
    std::erase_if(candidates, [this](DropAreaWidget* area) { return std::ranges::contains(m_dropRejected, area); });

    for (DropAreaWidget* const area : candidates) {
        if (!m_dragSource)
            return;
        if (area == m_dropTarget) {
            DropEvent event{EventType::DropMove, m_dragSource, m_dragSource->dragKeys(), m_dragSource->dragData(), {}};
            event.setPosition(area->mapFromWindow(point));
            if (area->m_onPositionChanged)
                area->m_onPositionChanged(event);
            return;
        }
        DropEvent event{EventType::DropEnter, m_dragSource, m_dragSource->dragKeys(), m_dragSource->dragData(), {}};
        event.setPosition(area->mapFromWindow(point));
        if (area->m_onEntered)
            area->m_onEntered(event);
        if (!m_dragSource)
            return;
        if (!event.isAccepted()) {
            m_dropRejected.push_back(area);
            continue;
        }
        leaveDropTarget();
        m_dropTarget = area;
        area->m_dragSource = m_dragSource;
        area->m_containsDrag.setIfChanged(true);
        return;
    }
    leaveDropTarget();
}
```

It first runs `layoutTree`, so the source's position includes the move just made, and maps the hot
spot to the window. The hit test skips the source (see [Hit-testing](#hit-testing)), so the dragged
widget never hides what is beneath it, and `hoverChainAt` (see [Hover](#hover)) turns the hit into
the widgets containing the point, innermost first. The drop areas among them that accept the keys
are the candidates.

`m_dropRejected` holds the areas that refused this drag. An area is forgotten as soon as it is no
longer a candidate, so it is asked again when the hot spot comes back, and it is skipped while it
stays one. The loop then walks the candidates from the inside out. Reaching the current target means
nothing inside it accepted, so it gets `DropMove` and stays the target. Any other area gets
`DropEnter`; if `onEntered` ignores it, the area is remembered and the next one out is tried. The
first area that accepts becomes the target, and only then does `leaveDropTarget` reset the old one
and call its `onExited`, so a refusing child never makes its parent exit and enter again. With no
candidate left, the old target is left as well. Each event carries the hot spot mapped into the area
with `mapFromWindow`. A callback can remove the source, so `m_dragSource` is checked after each one.

**Release.** `mouseButtonCallback` calls `finishDrag` before dispatching `DragEnd`. It runs
`updateDropTarget` once more for the final position, takes the target, sends it `Drop` through
`onDropped`, and resets its `dragSource` and `containsDrag` without calling `onExited`. Then it
clears `m_dragSource` and `m_dropRejected`.

**Removal.** `clearReferencesTo` nulls `m_dropTarget` without a callback when the target is removed,
and erases removed areas from `m_dropRejected`. When the source is removed, it clears `m_dragSource`
and `m_dropRejected` and calls `leaveDropTarget`, so a target that is still there gets `onExited`.
The target is checked first, so an area removed together with the source gets nothing.

**Files.** `glfwSetDropCallback` installs `dropCallback`, which GLFW calls with the paths already
decoded from the dropped `text/uri-list` data:

```cpp
void Window::dropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* const self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    const Point windowPoint{static_cast<float>(mx), static_cast<float>(my)};

    const std::vector<std::string> keys{"text/uri-list"};
    for (Widget* const widget : hoverChainAt(hitTestChildren(self, mx, my), windowPoint)) {
        auto* const area = dynamic_cast<DropAreaWidget*>(widget);
        if (!area || !area->acceptsKeys(keys))
            continue;
        if (area->m_onDropped) {
            DropEvent event{EventType::Drop, nullptr, keys, {}, std::vector<std::string>(paths, paths + count)};
            event.setPosition(area->mapFromWindow(windowPoint));
            area->m_onDropped(event);
        }
        return;
    }
}
```

There is no hot spot, so it uses the pointer, the same hit test as a click, and the innermost area
that accepts `"text/uri-list"`. GLFW reports a file drag only when it is dropped, which is why there
is no enter or exit. On X11, GLFW moves the cursor position along with the drag, so
`glfwGetCursorPos` is the drop point; on Wayland it does not, and the last pointer position is used.

### Text editing

Editing is split into three pieces that a text field, and later a multi-line editor, share: a
model, a key mapping, and the control that connects them to a `TextWidget`.

**`TextEditModel`** (`text/texteditmodel.h`) holds the text, a cursor and an anchor, with no widget
code. Positions are byte offsets that always sit at the start of a character (`codepointStartAt` and
`previousCodepointStart` in `utf8.h`). The selection is the text between the anchor and the cursor,
and every move takes an `extend` flag that moves only the cursor, so the anchor stays where the
selection began. A control converts to character counts for its public API with
`byteOffsetOfCharacter` and `characterIndexAt`.

Every edit, from a typed character to a deleted word, ends in one private function:

```cpp
void TextEditModel::replace(std::size_t start, std::size_t end, std::string inserted, bool typing) {
    if (m_readOnly)
        return;
    if (m_validator) {
        std::string next = m_text;
        next.replace(start, end - start, inserted);
        if (m_validator(next) == ValidatorState::Invalid)
            return;
    }
    const bool merge = typing && m_typing && start == end && !m_undoStack.empty()
                       && m_undoStack.back().position + m_undoStack.back().inserted.size() == start;
    if (merge)
        m_undoStack.back().inserted += inserted;
    else
        m_undoStack.push_back({start, m_text.substr(start, end - start), inserted, m_anchor, m_cursor});
    m_redoStack.clear();
    m_text.replace(start, end - start, inserted);
    m_typing = typing;
    finishEdit(start + inserted.size());
}
```

Because every edit passes here, read-only and validation are each one check, and the history is one
record per edit: the position, the removed and inserted text, and the selection before it. `undo`
puts the removed text back and restores that selection, and `redo` applies the edit again. `type`
is an `insert` that may join the previous record, when it continues exactly where that record ended
with nothing selected. Any move, selection change or other edit clears `m_typing`, so the next
keystroke starts a new record. `setText`, for text from code, clears both stacks.

Limits apply before `replace`. A single-line model turns inserted line breaks into spaces, and
`insertText` cuts the inserted text to the characters still free under `maximumLength`. `setText`
truncates to the maximum too, but is not validated.

`regexValidator` (`text/validators.cpp`) uses PCRE2 rather than `std::regex`, whose matcher recurses
once per character and overflows the stack on long text. A validator needs partial matching, to tell
whether more input could still match, and PCRE2 has it but refuses to combine it with end anchoring.
So the pattern is compiled as `(?:pattern)\z` instead, after compiling it as written, so that a
pattern with a stray `)` is reported rather than balanced by the wrapper. When an unterminated `\Q`
or an extended-mode comment swallows the wrapper's `)`, the wrapped pattern fails to compile and is
retried closed with `\E` or a newline. Each check is a full match, which is `Acceptable`, then a hard
partial match: a partial result, or a PCRE2 resource limit, is `Intermediate`, and anything else is
`Invalid`. Empty text is always `Intermediate`, since PCRE2 never reports a partial match before
reading a character. The pattern is JIT-compiled where PCRE2 supports it.

Words come from three kinds of character: word characters (ASCII letters, digits, `_`, and every
non-ASCII character, which keeps other scripts working without Unicode tables), spaces, and
punctuation. A word is a run of one kind. `moveWordRight` skips the run it is in, unless it is
spaces, and then any spaces; `moveWordLeft` skips spaces backwards and then the run before them.
`wordAt` returns the run at a position, for double-click.

The model reports `onTextChanged` and then, only if the cursor or anchor moved, `onCursorChanged`, so
an owner can lay out the new text before placing the caret on it.

**The key mapping** (`text/texteditactions.h`) is two halves with a named list of actions between
them:

```cpp
if (const auto action = textEditActionFor(key, event.modifiers()))
    applyTextEditAction(m_model, *action);
```

`textEditActionFor` matches modifiers exactly, so Ctrl+Left and Ctrl+Shift+Left are different
actions, and any combination it doesn't list stays free. A control asks it first and handles its own
keys only when it returns nothing: the text field adds Enter, and a multi-line editor would add Enter
as a line break and the vertical arrows. `applyTextEditAction` is a switch of model calls, plus the
clipboard for Copy, Cut and Paste, so a menu can run an action without a key. `changesText` tells
which actions edit the text, and `isTextInputKey` which keys type a character: those without Ctrl,
Alt or Super.

The same two answers decide which keys a focused control claims from window shortcuts (see
[Shortcuts](#shortcuts)). `TextFieldBase::triggerShortcutOverride` accepts a key when it maps to an
action the field allows, or when it types text and the field is editable, and then runs the user's
`onShortcutOverride`, which can still ignore it. Because the claim and the handling come from the
same mapping, they cannot disagree, and a newly mapped action is claimed without any other change.

**`TextFieldBase`** connects the model to its inner `TextWidget`. On `onTextChanged` it writes user
edits back to its `text` property, updates `textState`, `canUndo`, `canRedo` and
`acceptableInput`, and redraws the text, caret and selection; `onCursorChanged` redraws the caret and
selection.

*What is drawn.* `shown()` reduces the echo mode to `Plain`, `Masked` or `Hidden`.
`PasswordEchoOnEdit` is masked until `beginPasswordEdit` runs on the first edit after focusing, which
clears the text through `setText` so the history can't bring the old password back. `displayText()`
builds the string the inner text widget draws, and two functions translate positions between it and
the model:

```cpp
std::size_t TextFieldBase::toDisplay(std::size_t position) const {
    switch (shown()) {
    case Shown::Plain:
        return position;
    case Shown::Hidden:
        return 0;
    case Shown::Masked:
        break;
    }
    return characterIndexAt(m_model.text(), position) * mask().size();
}
```

`toModel` is the reverse, with `Hidden` returning the current cursor so a click doesn't move it. The
caret, the scroll offset and the selection go through `toDisplay`, and presses, drags and
double-clicks through `toModel`. `allows` refuses actions that change the text in a read-only field,
and Copy and Cut in every mode but `Normal`, where `applyAction` also turns word actions into
start-or-end ones so word boundaries stay hidden.

*The mouse.* A press stores its window position, and a drag moves the cursor to that position plus
`totalDx`/`totalDy` with `extend` set. Each point is mapped into the scrolled text widget, which is
what makes the text scroll as the pointer passes an edge. `MouseEvent::clickCount()` picks the unit:
2 selects `wordAt` the character under the pointer and drags by words around it, and 3 or more
selects everything. The character under the pointer is not where a click puts the caret:
`byteIndexForXOffset` rounds to the nearest gap between characters, so `characterAtPoint` steps back
one character when that gap is to the right of the pointer.

*Bindings.* User edits write the model's text back into the `text` property, which replaces a
binding there, as typing always has. Changes from code, meaning the `text` or `maximumLength`
property changing, run with `m_changeFromCode` set, and the write-back is skipped. A bound value the
model truncated therefore stays bound, and `text()` and `textState` report the model's text. The
same flag keeps `onTextEdited` to edits the user makes.

### Overlays and popups

An **overlay** is a widget whose `isOverlay()` returns true. It is out of flow: its parent's
layout, render pass, and hit-test all skip it (`participatesInLayout()` is false), and it is
laid out against the *window* rather than its parent - its content area is the window and its
world matrix ignores the parent chain. Ancestor `clip(true)`, rotation, and layout therefore
never affect it, even though it is declared as a child of an anchor widget.

`Window` keeps the open overlays in a typed stack, `std::vector<OverlayBase*>` (`addOverlay`
/ `removeOverlay`, topmost last). After the normal tree, `Window::layoutTree` calls each
overlay's `place()` and `layout(true)`, and `Renderer::renderForWindow` then renders them in stack
order, as in the snippet under [Per-frame sequence](#per-frame-sequence). A second, render-only list,
`m_retiredOverlays`, holds overlays that have left the stack but are still playing an exit
transition. `retireOverlay` moves an overlay there (restoring focus at that moment) and
`releaseOverlay` drops it when the exit ends; retired overlays are placed, laid out and drawn
first, beneath the open ones, and `clearReferencesTo` clears both lists. Hit-testing
walks the stack in reverse before the tree:

```cpp
Widget* Window::hitTestChildren(Window* self, double x, double y) {
    for (auto it = self->m_overlays.rbegin(); it != self->m_overlays.rend(); ++it) {
        if (auto* hit = static_cast<Widget*>(*it)->hitTestTree(static_cast<float>(x), static_cast<float>(y)))
            return hit;
        if ((*it)->modalActive())
            return nullptr;
    }
    /* ... then the normal tree ... */
}
```

**`OverlayBase`** (`templates/overlaybase.h`) extends `Control` and owns everything about being
an overlay: `open()`/`close()`, `opened()`/`openedState()`, `onOpen`/`onClose`, `closePolicy`,
`modal`, and the `scrim` slot. It claims mouse, drag, and wheel events so nothing inside it
bubbles on to the anchor, sizes itself to its children's extent plus padding (an explicit
`width`/`height` wins), and gives its `background` the whole overlay rather than the padded area
through `Widget::contentAreaFor()`. As with `TextFieldBase`, `content()` is hidden: the overlay's
children are declared inside it. A subclass overrides the virtual `place()` to choose where it
sits.

The **scrim** is a slot like `background`, painted first (`z(-2)`) and sized to the whole window
through `contentAreaFor()`; it is shown only while the overlay is open *and* modal. A transparent
scrim is installed by default, so an overlay never needs one to block input. Because the scrim is
a child of the overlay, a modal overlay is still a single stack entry.

Dismissal is driven from `Window`, using the overlay's `closePolicy` (a bitmask of `None`,
`ClickOutside`, `ClickOutsideParent`, `Escape`; `ClickOutside | Escape` by default):

- A press outside the topmost overlay closes it when `ClickOutside` is set. "Outside" means
  the press hit nothing inside the overlay, or only its scrim. A non-modal overlay then lets the
  press continue to whatever is behind it; a modal one swallows it. A press on the *anchor* counts
  as outside, so an anchor that toggles on click should call `open()` rather than flip state.
- With `ClickOutsideParent` instead, a press outside the overlay closes it only if the widget it hit is
  not the overlay's parent or inside it. `Window::mouseButtonCallback` walks up from the hit widget
  looking for the parent. A press on the anchor then reaches the anchor with the overlay still open, so
  the anchor can toggle it; the combo box relies on this.
- `Escape` closes the topmost overlay when `Escape` is set, and the key is consumed before the
  focused widget sees it.
- `modal` is read when the overlay opens. While it is open, hit-testing blocks hover, press, drag,
  and scroll to everything behind it, and Tab is confined to its own controls.

Focus follows the stack through the [focus memory](#focus-memory): `Window::addOverlay` calls
`rememberFocus` and `focusFirstIn`, and `removeOverlay` and `retireOverlay` call `restoreFocus`,
so focus returns to the widget that had it, unless it has moved elsewhere in the meantime.

**Transitions.** `OverlayBase` owns two `TransitionFactory` slots, `enter` and `exit`, and a
`TransitionRunner` that animates the overlay widget itself, with the window size as the container.
Its opacity reaches the background, content and scrim through the inherited effective opacity, so
they fade together. `applyOpened` first calls `finishNow()`, so a transition in flight completes
before the new one starts. Opening shows the overlay, adds it to the stack, and runs `enter`.
Closing with an `exit` factory and a window retires the overlay and runs `exit`; its completion
callback hides the overlay and the scrim and releases it. Without an `exit`, or without a window,
it hides immediately. `modalActive` clears at the moment of `close()`, so the animation never
blocks input, and `onOpen`/`onClose` fire at that moment too.

**`PopupBase`** (`templates/popupbase.h`) is an `OverlayBase` that places itself relative to a
target rectangle - the parent widget by default, or the window (`target(PlacementTarget::Window)`)
- in `place()`, on every layout pass, so it follows an anchor that moves or resizes:

| Target | `Bottom` / `Top` / `Left` / `Right` | `Center` |
|---|---|---|
| `Parent` | Outside the anchor, start-aligned on the cross axis; flips to the opposite side if the preferred side is too small and the other has more room | Centered over the anchor |
| `Window` | Docked inside that edge of the window, centered on the cross axis; no flip | Centered in the window |

Each side has its own offset (`offsetLeft`/`Top`/`Right`/`Bottom`; `offset(v)` sets all four): the
gap from the anchor when the popup sits on that side, or the inset from the window edge. After a
flip the *new* side's offset applies. Results are clamped into the window. `x()` and `y()` are
explicit coordinates measured from the target's top-left corner; they override placement per
axis and are not clamped. Whichever of `x`, `y`, and `placement` was set last wins, tracked by
the properties' change notifications rather than call order, so a bound value that changes later
counts as the latest. A subclass can call the protected `setCenteredOnAnchor(true)` to center on
the cross axis against the parent instead of start-aligning; tooltips do.

**`PopupWidget`** (`controls/popupwidget.h`) is the concrete popup. It installs a rectangle
`background` and a rectangle `scrim`, both bound to `PopupStyle` (`backgroundColor`, `borderColor`,
`scrimColor`, `borderWidth`, `padding`, `radius`, and the `enter` and `exit` transition
factories, which default to a short fade) through `Theme::popup`, using the same
`style()`/`applyStyle()` pattern as every other control (see
[Theme and style structs](#theme-and-style-structs)).

### Flick areas

**`FlickAreaBase`** (`templates/flickareabase.h`) extends `Control` with a clipped viewport over
content that can be larger than itself. Children are declared inside it and `content()` is
hidden, as with popups; the only slot is `background`. `FlickAreaWidget`
(`controls/flickareawidget.h`) is the concrete version: a background rectangle styled by
`FlickAreaStyle` (`backgroundColor`, `radius`) through `Theme::flickArea`, transparent and square
by default.

**Scrolling is a layout shift.** The area overrides `contentAreaFor(child)`: the background gets
the full viewport, and every other child gets the viewport area shifted by `-contentX`/`-contentY`.
A scrolled child therefore has the correct resolved position, displayed geometry, and world
matrix, which is what rendering and hit-testing already read, so no event position is ever
transformed. The cost is that each offset change marks the area's layout dirty and the force
layout pass runs over the whole content subtree. Sizes are cached, so what repeats is the
position and matrix work per descendant. Offsets are rounded for layout so text stays crisp.

**Offsets and content size.** The requested offsets (`contentX`, `contentY`) are `Property`s; the
effective, clamped ones are the `contentXState`/`contentYState` states, updated by
`syncOffsets()`, which runs whenever an offset, the content size, or the direction changes and
whenever the area's own resolved size changes. Content size comes from `computeContentSize()`,
the extent of the children (margin plus `x`/`y` plus intrinsic size, with a `Fill` axis counting
as 0), raised to at least the viewport; an explicit `contentWidth`/`contentHeight` overrides it.
The maximum offset on an axis is the content size minus the viewport, never below 0, and 0 for
an axis missing from `flickDirection` (default: both). `computeContentSize()` is virtual so a
virtualized list can supply its own extent.

Setting offsets while the area is built only records that a sync is pending; the sync runs once
in `onBuilt()`, so `onScroll` fires once for an initial offset even when it is registered after
the setter.

**The clip shape.** `FlickAreaBase` sets `clip(true)`, which routes it through the three-step
[clip composite](#clipping). A `Control` draws nothing while the mask is captured, so its
`render()` draws the `background` slot's shape in that case and the real content otherwise. The
base installs a transparent background in its constructor so there is always a shape, and
replacing `background` with a rounded rectangle makes the clip rounded too.

**Input.**
- **Wheel:** `triggerWheel` scrolls by `wheelStep` per notch on each axis. `hasHandlerFor(Wheel)`
  is true only while the area can actually scroll, and a tick that moves nothing is `ignore()`d,
  so a wheel at the end of an inner area continues to an outer one.
- **Drag:** with `dragScroll` on and something to scroll, the area claims `DragStart`/`DragMove`/
  `DragEnd` and moves the content with the pointer. Drags bubble up from the hit widget, so a
  slider or other control inside claims its own drag first, and the click-suppression rule under
  [Press / release / click](#press--release--click--double-click) keeps the pressed item from
  being clicked afterward.
- **Keys:** arrows scroll by `wheelStep`, Page Up/Down by the viewport size, Home/End jump to the
  ends. The area does not need focus itself: keys the focused child ignores bubble up to it.
  Unrecognised keys and keys that move nothing are `ignore()`d.

A subclass customizes the area through two protected hooks. `isContentChild(child)` says which
children are scrolled content (the default is everything except the background); a child it
rejects is laid out against the whole viewport and does not count toward the content extent.
`onOffsetsSynced(offsetsChanged)` runs at the end of every `syncOffsets()`, whether the offsets
moved or only the size, direction, or content changed.

### Scroll areas

**`ScrollAreaBase`** (`templates/scrollareabase.h`) extends `FlickAreaBase` with scrollbars.
`ScrollAreaWidget` (`controls/scrollareawidget.h`) is the concrete version, styled by
`ScrollAreaStyle` through `Theme::scrollArea`. Everything about content, offsets, and the clip
is inherited; the one change to the inherited behavior is that the constructor turns
`dragScroll` and `keyNavigation` off, the usual desktop convention, since dragging content
fights with selection and with drags inside it. Both are the same properties and can be turned
back on.

**Bar slots.** There are four, `verticalTrack`, `verticalThumb`, `horizontalTrack`, and
`horizontalThumb`, each installed like `background` (`z` 10 for tracks, 11 for thumbs). They
overlay the content rather than reserving space. `isContentChild()` rejects them, so they are
not scrolled and do not enlarge the content extent, and `contentAreaFor()` gives them the whole
viewport. The base binds each slot's `visible` and `opacity` before running the user's body, so
a body can still override either.

**What the base publishes.** For each axis, a thumb size (the visible fraction of the content,
raised to at least `minThumbSize` pixels) and a thumb position (offset divided by the maximum
offset), plus hover and pressed states. A concrete thumb binds two properties:

```cpp
thumb.heightFraction(verticalThumbSizeState()).yFraction(verticalThumbPositionState());
```

`yFraction` is already "fraction of the free space along the track", so the position state
needs no conversion and dragging uses the same arithmetic in reverse.

**Policy and visibility.** Each axis has a `BarPolicy`: `Auto` (a bar exists only while that
axis overflows), `Always`, or `Never`; an axis missing from `flickDirection` never has one.
`syncBars()`, run from `onOffsetsSynced()`, recomputes the sizes, positions, and whether each bar
is wanted.

**Auto-hide.** `ScrollAreaBase` also derives `AnimatableBase` privately, as `TextFieldBase`
does for the caret. A fading axis (`autoHide` on and policy `Auto`) has an opacity state; a bar's
slot is shown only while it is wanted *and* its opacity is above zero, so a faded-out bar is
hidden and cannot be hit. `wake()` resets an idle timer and registers the animation, and it is
called when the offsets change, when the pointer enters or leaves the area, and when a thumb is
pressed. `tick(dt)` keeps the idle timer at zero while the area is hovered or a thumb is
pressed, raises the opacity toward 1 while `idle < barHideDelay`, lowers it toward 0 after, over
`barFadeDuration`, and returns false once settled so the animation manager unregisters it.
Axes with policy `Always`, or with `autoHide` off, stay at opacity 1.

**Bar interaction.** The bars are ordinary children, so their events bubble up to the area.
The base claims `MousePress` and `MouseMove` only to read the position, which arrives in the
area's own coordinates, and tests it against the visible slots' rectangles. If the point is not
on a bar it calls `ignore()`, so presses elsewhere keep bubbling to ancestors.
- A press on a **thumb** starts a bar drag on that axis. While it lasts the base claims
  `DragMove` and scrolls by `dy * maxContentY / (trackLength - thumbLength)`, using the slots'
  displayed sizes (the horizontal case is the same with `dx`). A drag that starts anywhere else
  is still the inherited content drag when `dragScroll` is on.
- A press on a **track** away from the thumb scrolls one viewport toward the click.
- Release and `DragEnd` clear the pressed and drag state. The wheel needs nothing extra, since
  wheel events over a bar already bubble to the area.

**`ScrollAreaSkin`** (`controls/scrollareaskin.h`) is a header-only class template that sits between a
scrolling base and its concrete widget. `ScrollAreaSkin<Base>` derives from `Base`, which can be any
`ScrollAreaBase`, including `ListViewBase`, and holds the style side of a scroll area. It owns the style
properties (`backgroundColor`, `trackColor`, the three thumb colors, `radius`, `thumbRadius`,
`barThickness`, `barMargin`, `transition`), `style()`, and the thumb color logic: the thumb color is an
`AnimatedState<Paint>` that moves between `thumbColor`, `thumbHoverColor`, and `thumbPressedColor` over
`transition` seconds as the base's hover and pressed states change. `ScrollAreaStyle` also carries the
fade timings and `minThumbSize`, which `applyStyle` forwards to the base's setters. Its constructor only
connects those internal callbacks: it builds no widgets and applies no theme. A protected
`installDefaultSlots()` installs the background, plus a rectangle track and thumb for each axis (tracks
hug an edge, and thumbs bind the two fractions above), and nothing calls it unless a derived class does.

**`ScrollAreaWidget`** is `ScrollAreaSkin<ScrollAreaBase>`. Its constructor calls
`installDefaultSlots()` and then `style(theme().scrollArea)`, so the defaults come first and the theme's
values are in place before any per-instance setter runs. A class that derives from the skin directly gets
the style properties without the default visuals, and installs its own slots, with no default built and
discarded. The layers are therefore: the base owns behavior, the skin adds the style properties, and the
widget installs the defaults and applies its theme.

### List views

**`ListViewBase`** (`templates/listviewbase.h`) extends `ScrollAreaBase` with a row `count` and a
delegate that builds one row from its index. `ListViewWidget` (`controls/listviewwidget.h`) is the
concrete version, `ScrollAreaSkin<ListViewBase>` with the same default slots as `ScrollAreaWidget`,
reading `Theme::listView`. `ListViewStyle` is an alias of `ScrollAreaStyle`. The list scrolls
vertically only: its `flickDirection` is `Vertical`.

**Row table.** The base keeps `m_heights`, the measured height of every row (0 meaning not measured
yet), and `m_tops`, the prefix sums of the row tops with `count + 1` entries, so `m_tops[i]` is where
row `i` starts in content coordinates and the last entry is the total. A row that has not been measured
counts as `m_estimate`, the average of the measured rows, or 40 before any is measured. `rebuildOffsets()`
recomputes the estimate and the tops, and runs when a height, the count or the spacing changes.
`indexAt(y)` is a binary search over the tops.

**Placement.** `contentAreaFor(row)` takes the base area (the viewport shifted by `-contentX/Y`) and adds
`m_tops[index]`, with the row's measured height. Rows are `Fill::Width`, so a row's own height is its
explicit or natural height, which is what was measured. As with the flick area and swipe view, rows
therefore have real layout positions and no event is translated. The live rows are a contiguous index
range, a deque `m_rows` of `LiveRow` (the row widget and, for a model-bound list, its states) with the
index of the first one in `m_first`.

**Syncing.** `syncRows()` runs from three places: `onOffsetsSynced()` (the scroll position, the size,
or a property that feeds them changed), `render()` on every normal frame, so a row that grows later is
noticed, and `refreshRows()`, which calls `scrollTo(contentX, contentY)` to force the first after a
change to `count`, `spacing`, `cacheBuffer` or the delegate. It does nothing while the view has no
height, so nothing is built before the first layout. Otherwise it runs `syncPass()` repeatedly, at most
8 times:

1. Work out the range to keep, from `indexAt(top - cacheBuffer)` to `indexAt(bottom + cacheBuffer)`.
2. Remove rows from the front and back that are outside it, and create the missing ones at either end.
3. Measure every live row (`intrinsicSize().height` plus margins, at least 1) and store the heights that
   changed.
4. If any changed, rebuild the offsets and set `contentY` so the row that was at the top of the view stays
   where it was on screen, then run another pass, because the new offsets may call for different rows.

A pass that measures nothing new ends the loop. The cap only bounds the work done in one sync; `render()`
continues on the next frame. When the estimate changes, thousands of unbuilt rows above the view change
height, so `contentY` can move by a large amount while the rows on screen do not move.

**Content size and visible rows.** `computeContentSize()` returns `m_tops[count]` minus the trailing gap,
which `rebuildOffsets()` also publishes as `rowsHeight()` and `rowsHeightState()` for anything that sizes
itself to the rows, such as a combo box popup,
so the scroll limit and the scrollbar come from the inherited flick area code. `updateVisible()` sets the
first and last visible indices from the same table: the first row whose bottom is below the top of the
view, and the last row whose top is above the bottom.

**Delegate.** `delegate<T>(body)` stores a function that builds a row with `addChild<T>`, sets
`Fill::Width`, and runs `body(row, index)` inside a `Builder::Scope` on the row. Rows are created after
the window is built, when the builder has no current parent, so the scope is what lets `TB::` calls in
the body attach to the row. Removed rows go through the usual deferred destruction, and the window drops
any focus or hover state pointing into them.

**Data changes.** A `count` change resizes `m_heights` (new rows start unmeasured), removes the rows past
the end, and re-syncs. `reload()` destroys every row, forgets every measured height, and re-syncs.

**Models.** `ListModel<T>` (`model/listmodel.h`) owns a `std::vector<T>` and derives from the non-template
`ListModelBase`, which keeps the observers. Every edit calls `notify(ListChange)` with a kind (`Inserted`,
`Removed`, `Changed`, `Moved`, `Reset`) and the index range (`index`, `count`, and `to` for a move);
`clear()` is a `Removed` of everything and `assign()` a `Reset`. The base destructor sends `Destroyed`.
Callbacks are copied before notifying and skipped if removed meanwhile, the same as `State`.

`model(m)` subscribes `modelChanged` to the model and clears the delegate, so `rowCount()` now comes from
the model. `delegate<Row, Item>(body)` `dynamic_cast`s the attached `ListModelBase` to `ListModel<Item>`;
on failure, or with no model, it logs an error and installs nothing. The stored delegate creates a
`ModelRowState<Item>`, which holds a `State<Item>` copied from the model and a `State<int>` index, builds
the row, and passes both states to `body`. They live in the `LiveRow` next to the widget, behind a small
virtual `RowState` interface (`setIndex`, `refresh`) so the non-template base can update them. A plain
`delegate<T>(body)` detaches the model and stores no states.

`modelChanged` keeps every piece of row bookkeeping in step with the model, which has already changed when
it is called:

- **`Changed`** calls `refresh()` on the live rows in the range, which sets their `item` state from the
  model. Rows are not rebuilt; the next sync re-measures them, so a height change is picked up there.
- **`Inserted`, `Removed`, `Moved`** first record the anchor with the old row table: the row at the top of
  the view and how far the view's top is into it. They then splice `m_heights` the same way (inserted
  rows start unmeasured, a moved row keeps its height) and splice the live rows: `insertRows` builds rows
  for items inserted inside the live range, or retires the rest of the range if more items were inserted
  than there are live rows, and only shifts `m_first` for inserts above it; `removeRows` retires the rows
  in the range and recomputes `m_first`; `moveRow` takes the row out and puts it back at its new index,
  building it if it was not live but lands inside the range and retiring it if it lands outside.
  `reindexRows()` then sets every live row's `index` state, and only rows whose index changed notify.
- The anchor index is mapped through the same change (shifted, or replaced by the row now at that index
  with offset 0 if it was removed, or following its own row if that row moved), the offsets are rebuilt,
  and `contentY` is set so the anchor row is where it was on screen, or to 0 if the view was at the top.
  A normal sync then fills in and measures rows.
- **`Reset`** reloads. **`Destroyed`** drops the model and the delegate and reloads, leaving the list
  empty.

Row positions are tracked by `trackedCount()`, the size of `m_heights`, rather than by the model's size,
so the old table is still consistent while a change is being applied. A change that arrives during a sync
(from inside a delegate) cannot be spliced safely, so it sets `m_reloadPending` and the sync reloads when
it finishes.

Rows leave through `retireRow`, which removes the widget with the usual deferred destruction and moves its
states to `m_retiredStates`, cleared at the start of the next `render()`. A click handler that removes its
own row from the model therefore still finds its `index` state alive until it returns.

**Building.** Following the [build-complete hook](#the-build-complete-hook), the `count`, `spacing` and
`cacheBuffer` callbacks and the delegate install do nothing until the list is built, and `onBuilt()` sizes
the height table and rebuilds the offsets once before the first sync. The rebuild must come first: the
sync reads the content size, which reads `m_tops[count]`. `computeContentSize()` returns 0 until built.

**`scrollToIndex(i)`.** It stores the clamped index as `m_target`, scrolls to that row's estimated top
with syncing held so the two offset updates do not each run a sync, and then runs one sync. While a target
is set, each pass anchors on the target row at offset 0 instead of on the row at the top, so as rows near
the target are measured it is re-aimed to its real top; the target is cleared when the sync ends. Called
before the build, the index is remembered and applied in `onBuilt()`, and kept until the first sync that
has a viewport.

**Current index.** `currentIndex` is a `Property<int>`, mirrored into `m_currentIndexState` for observers.
Every change goes through `applyCurrentIndex`, which turns any value outside the rows into -1 (before the
list is built the value is kept as given and checked in `onBuilt`, so the order of `.currentIndex()` and
`.count()` in a builder chain does not matter), updates the state, and moves the highlight. Changes the
list makes itself, from the model or from `count`, go through `setCurrentIndexQuietly`, which moves the
highlight without animating it: the rows move on screen at the same moment, so an animation would slide
away and back. For an insert, remove or move, `modelChanged` maps the index with the pure free function
`indexAfterChange(index, change)` from `model/listmodel.h`, shared with the [combo box](#combo-boxes): an insert at or before it adds `count`, a removed block entirely before
it subtracts `count`, a removed block containing it gives -1, and a move carries the moved item to `to` and
shifts an index that the item crosses or lands on by one, toward where the item came from. The new index is
applied after the row table is rebuilt, so the highlight lands on the right row. A model reset, a new model
attached after the list is built, model destruction, and a `count` shrinking past the index all set -1. A
model attached while the list is still being built keeps the requested index, which `onBuilt` checks.

**Highlight.** `highlight<W>(body)` creates one widget, a child of the list like the rows but not one of
them, with `Fill::Both` and `z(-1)` so it draws under the rows. `contentAreaFor` places it from two
`AnimatedState<float>`s, its top and height, which `updateHighlight` takes from the row table: `m_tops`
and `heightOf`, measured or estimated, so it needs no built row. An index change animates both over
`highlightMoveDuration` with `easeOutQuad`; each sync then snaps them to the current row's position unless
a move is in progress, which corrects estimates as rows are measured. The two states call `markLayoutDirty`
when they change, and `updateHighlight` only writes values and visibility that differ, so an idle list
does not schedule frames. It is hidden at -1, and with `highlightFollowsCurrent(false)` `updateHighlight`
and `contentAreaFor` leave it to the caller.

**Keys and `ensureVisible`.** `hasHandlerFor` claims key presses whenever `keyNavigation` is on, unlike the
flick area, which only does while it can scroll, because a short list in a popup must still move its index.
`triggerKeyPress` handles Up, Down, Home and End by setting the index and calling `ensureVisible`; other keys
go to the flick area's pixel scrolling. `ensureVisible(i)` compares the row's span from the table with the
view: a row above is scrolled to the top edge, a row below to the bottom edge, or to its top if it is taller
than the view.

**Costs and limits.** A sync costs about the number of live rows, plus O(log n) for each `indexAt`.
`rebuildOffsets()` is O(count) and runs when a height changes and after each structural model change. Rows keep no state when they leave
the range. A row needs a natural height (an explicit `height()` or a layouter's content), because a plain
widget without one measures 0 and is clamped to 1 pixel.

### Combo boxes

**`ComboBoxBase`** (`templates/comboboxbase.h`) is a `ButtonBase` with three slots and no look:
`popup<W>()`, constrained with `std::derived_from<W, PopupBase>`, `list<W>()`, constrained to `ListViewBase`
and placed inside the popup, and `indicator<W>()`. It works only through `PopupBase*` and `ListViewBase*`,
so like every other base it includes nothing from `widget/controls`. `ComboBoxWidget`
(`controls/comboboxwidget.h`) fills the slots with a `PopupWidget`, a `ListViewWidget` with a highlight and
an SVG chevron, installs the background and a label bound to `displayTextState()`, and follows
`Theme::comboBox`. Installing the popup sets its placement and a `ClickOutsideParent | Escape` close policy
before the caller's body and its `onClose` after it, so the body cannot replace the handler the combo box
depends on. Installing the list hooks it up: model, rows, current index and height.

**The chosen index.** `currentIndex` is a `Property<int>` mirrored into `m_currentIndexState`. While the
popup is closed, the chosen index *is* the list's `currentIndex`: `syncFromList` copies it whenever the list
changes it, including when the list itself shifts it for a model change, so closed-state tracking reuses
the list's rules. Opening stores the chosen index in `m_remembered`; from then on the list's `currentIndex`
is only the highlight, moved by hover and keys, and changes to it are ignored. Model changes while open are
applied to `m_remembered` with `indexAfterChange`, the same function the list uses, so it stays correct.
`pick(i)` sets the list's index, closes with `m_picking` set, commits, and calls `onActivated`. Any other
close (Escape, a press outside, a click on the box) reaches `popupClosed` through `onClose` with
`m_picking` false and restores the list's index to `m_remembered`. Without a list, the base keeps the chosen
index itself and maps it through `indexAfterChange`.

**Building.** `onBuilt` applies the requested `currentIndex`, because the list is built inside the
constructor, before the caller's builder chain: checked there, `.currentIndex(1).model(m)` would be
rejected against an empty list. `text<Item>()` and `delegate<Row, Item>()` check the model's type with
`dynamic_cast` and store a row installer, `m_installRows`, which runs now if the list exists and again
whenever a list is installed, so the order of the calls does not matter.

**Rows.** Each popup row is a `ColumnWidget` wrapper made by the list delegate, holding either a text label
(`text<Item>()`) or the caller's row (`delegate<Row, Item>()`). The wrapper carries `onMouseEnter`, which
moves the highlight, and `onClick`, which picks. A widget keeps one function per event and setting one
replaces it, so putting these on the caller's row would collide with handlers the caller sets; the wrapper
keeps them apart. Default labels pass through the virtual `styleRowText(TextWidget&)`, empty in the base,
where the widget applies its text color and font.

**Keys and focus.** While open, the focused list handles Up, Down, Home and End; Enter bubbles from the
list to the combo box, which picks, and Escape is handled by `Window` before any widget. While closed, the
combo box handles Up and Down itself and steps the chosen index, and Space and Enter reach `ButtonBase`,
whose click opens the popup. Focus moves to the list on open and back to the box on close, through the
overlay focus memory and an explicit `focus()` in `popupClosed`.

**Sizing.** Opening sets the popup's width to the box's and scrolls the chosen row to the top. The list's
height is `min(rowsHeight(), popupMaxHeight)`, recomputed from `rowsHeightState()`, so it settles as rows
are measured; rows are estimated until the popup has been shown once.

### Tooltips

**`ToolTipBase`** (`templates/tooltipbase.h`) is a `PopupBase` with no look, placed above its parent,
centered, 6 px away, with close policy `None`. It splits the overlay's open state in two: `opened` is the
*request*, usually bound to a state such as `hoveredState()` or `pressedState()`, and `openedState()` is
whether the tooltip is *showing*. It overrides `OverlayBase::applyOpened`, which is virtual for this: a
request to open waits `delay` seconds, counted in an `AnimatableBase` tick, and a request to close cancels a
pending open and hides it. An open request while it is already showing skips the delay. Once shown,
`timeout` hides it after that many seconds.

**Why `show()` and `hide()` leave `opened` alone.** `Property::set` removes a binding, so writing `opened`
from inside the tooltip would cut it loose from the state it follows. `hide()`, which the timeout also uses,
hides now and cancels a pending open; `show(text, timeout)` sets the text and the timeout and takes the same
delayed path as an open request. Neither touches `opened`, so a bound tooltip shows again on the next
false-to-true change of its binding. The close policy is `None` for the same reason: the window's Escape and
outside-press handling call `close()`. The text is a `BindableState<std::string>`; the public API is
`text(...)` and `text()`, and subclasses bind to it through the protected `textState()`.

**`ToolTipWidget`** (`controls/tooltipwidget.h`) installs a rectangle background and a `TextWidget` bound to
`textState()`, styled from `ToolTipStyle` through `Theme::toolTip`.

**The control shortcut.** `ButtonBase` owns one tooltip. `toolTip()` returns it, calling the protected
virtual `createToolTip()` first when there is none, and `toolTip<W>(body)` replaces it with a `W`
constrained to `ToolTipBase`. Installing sets `delay(0.5)` and `opened(hoveredState())` before the caller's
body, so the body can change either. `triggerPress` calls `hide()`; `opened` stays true while the pointer is
over the control, so the tooltip stays hidden until the pointer leaves and comes back. Bases include
nothing from `widget/controls`, so `ButtonBase::createToolTip()` installs a plain `ToolTipBase`, which draws
nothing, and the Button, Checkbox, Radio, Switch and ComboBox widgets override it to install a
`ToolTipWidget`. Because `toolTip()` returns `ToolTipBase&`, styling one tooltip goes through
`toolTip<ToolTipWidget>(body)`.

**Other overlays.** A showing tooltip is an ordinary entry on the overlay stack. With close policy `None`,
while it is the topmost overlay, Escape and outside presses close nothing, so a popup beneath it stays open
until the tooltip hides. A hover tooltip usually hides first, because the pointer leaves its control before
pressing elsewhere.

### Stack views

**`StackViewBase`** (`templates/stackviewbase.h`) extends `Control` with a stack of items that each
fill the view. `StackViewWidget` (`controls/stackviewwidget.h`) is the concrete version, styled by
`StackViewStyle` through `Theme::stackView`.

**The items are the children.** The stack keeps no list of its own. Its items are its children,
minus the `background` slot, overlay children and items that are being retired, in creation order,
so the last one is the top. Depth, `currentItem()`, `item(i)` and `indexOf()` are computed from that,
so they cannot go stale. `push<T>(body)` creates an item through `addChild<T>`, so a component's
`build()` runs. It sets `fill(Fill::Both)` before the body, so the body can still override it, and the
previous top item stays alive, hidden once the transition ends. `Object::ancestor<T>()` walks the
parent chain, and `StackViewBase::of(widget)` uses it so an item can reach its stack without a stored
pointer.

**Operations.** `pop` removes the top item, `popTo` and `popToRoot` remove several, `replace<T>`
swaps the top item, and `clear` removes everything. A new operation first calls
`finishTransition()`, which finishes any running transition at once, so nothing queues. `pop`,
`popTo` and `replace` mark the leaving item as *retiring* before updating the depth, so
`depth()`, `currentItem()` and `onItemChange` already describe the result while the item is still
animating out; the item is removed through the usual deferred destruction when its transition ends.
`popTo` removes the items between the top and the target immediately and animates only the top
item against the target.

**Transitions.** Six slots, `pushEnter`, `pushExit`, `popEnter`, `popExit`, `replaceEnter` and
`replaceExit`, each hold a `TransitionFactory`. They are set with a transition type and its
constructor arguments (`pushEnter<SlideIn>(Edge::Right, 0.3f)`), or with a ready factory (which is
how the style applies them). An operation builds an enter transition for the incoming item and an
exit transition for the outgoing one, and hands both to the stack's `TransitionRunner`. An empty
slot means no animation. A `TransitionSet` (an enter and an exit factory) passed to `push`,
`pop`, `popTo`, `popToRoot` or `replace` overrides the slots for that call, and
`TransitionSet::immediate()` forces no animation.

The stack turns `clip(true)` on only while a transition runs, using the same mask-shape trick as
`FlickAreaBase`: a transparent default background and a `render()` that draws it during mask
capture, so a slide cannot spill outside the stack. While a transition runs,
`transitionRunningState()` is true. Input to the retiring item is not blocked; an app that wants
that can disable its controls from this state.

**Focus.** Pushing calls `rememberFocus` for the new item and `focusFirstIn` on it, then clears
focus if it is still inside the previous item (the new item had nothing focusable), so keys never
go to a hidden item. Popping calls `restoreFocus` on the leaving item before it is removed.
`replace` calls `transferFocusMemory` from the replaced item to the new one, so popping the
replacement still returns focus to the widget that had it before the replaced item was pushed. See
[Focus memory](#focus-memory).

**`StackViewWidget`** applies `StackViewStyle`, which holds the six factories, through its `style()`
overloads and `applyStyle`. The defaults are a slide for push and pop (`SlideIn(Right)` and
`SlideOut(Left)`, and the reverse for pop) and a crossfade for replace.

### Swipe views

**`SwipeViewBase`** (`templates/swipeviewbase.h`) extends `Control` with pages that sit side by
side along an `Orientation`. `SwipeViewWidget` (`controls/swipeviewwidget.h`) is the concrete
version, styled by `SwipeViewStyle` (`duration`, `easing`) through `Theme::swipeView`. Like the
stack view it owns no background of its own: it installs a transparent one, which also serves as the
clip shape, and always has `clip(true)`.

**One number drives everything.** The view keeps a float `position`, measured in pages. `0` shows
page 0, `1` shows page 1, and `0.5` is halfway between. The pages are the children, minus the
`background` slot and overlays, in creation order, so a page's index is its place in that list.
Nothing is transformed. `contentAreaFor(page)` returns the viewport rectangle moved along the axis by

```
round((pageIndex - position) * pageSize)
```

so each page resolves a real position, and rendering, hit-testing and focus read it like any other
layout result, with no event positions translated. This is the same layout-shift approach as
`FlickAreaBase`. Every position change marks the view's layout dirty.

**Visibility.** `syncVisibility()` runs on every position change and shows only the pages from
`floor(position)` to `ceil(position)`, which is one page at rest and two while moving. The other
pages get `visible(false)`, so rendering, hit-testing and the Tab chain skip them. Layout still runs
for them, because the view is not a layouter.

**Index.** `m_currentIndex` is the requested value and can be out of range or set before any pages
exist. `currentIndexState()` is the clamped one. `applyIndex(animated)` reconciles them whenever the
request changes:

1. It clamps the request, and writes the clamped value back when pages exist.
2. It finds the page at that index and, if it is a different page from before, moves focus.
3. It updates the state and calls `onCurrentIndexChange`.
4. It animates `position` to the index, or jumps there when `animated` is false, the size or duration
   is zero, there is no window, or the user is dragging.

**Page changes.** `Object::appendChild` and `detachChild` are private, so the view cannot be told
when a child is added or removed. `refreshPages()` instead compares the current page list with the
last one it saw, and runs on every render, at the first layout (from `onResolvedSizeChanged()`, so an
initial index shows on the first frame), and at the start of `setCurrentIndex` and drags. When the list
changed it drops focus memory for pages that are gone, updates `countState()`, and keeps the same page
current if it still exists (setting the index to its new place) or otherwise re-applies the clamped
request, without animating. A page added or removed at runtime can therefore take one frame to settle.
`onBuilt()` runs `refreshPages()` once the pages exist, so an index set in the body is applied
before the first frame and `onCurrentIndexChange` fires once for it.

**Animation.** `animateTo(index)` records the start and end positions and registers with the
`AnimationManager`. Each tick advances an elapsed time and sets
`position = from + (to - from) * easing(t)`, ending exactly on the target. The tween is on `position`,
not the index, so a drag or a new request that interrupts it continues from where it is.

**Dragging.** The view claims `DragStart`, `DragMove` and `DragEnd` when `interactive` is set and
there are two or more pages, so a drag that starts on a widget ignoring drags bubbles up to it.

- `DragStart` refreshes the pages, stops the animation and sets `m_dragging`, which makes
  `applyIndex` leave the position alone.
- `DragMove` does `position -= delta / pageSize`, clamped to `0..count-1`, and stores a
  timestamped sample of `position * pageSize`, keeping about the last 100 ms.
- `DragEnd` estimates the release velocity from those samples, and treats it as zero if the last sample
  is more than 80 ms old. The target is the nearest page, unless the speed is above 400 px/s, in which
  case it is the next page in the direction of travel (`ceil` forward, `floor` back). It clears
  `m_dragging` and requests that index, so the snap animates from the release point; even
  a snap back to the same page animates, because `position` is not on it yet.

A handled `DragEnd` also makes the window suppress the click that would follow, so a drag that
starts on a button does not press it.

**Keys.** `KeyPress` is claimed when `keyNavigation` is set and there are two or more pages. The
arrow keys matching the orientation call `setCurrentIndex(index ± 1)`. If the index does not change
(at an end) or the key is another one, the event is ignored so it bubbles on. The view is not
focusable; it receives keys from a focused descendant that ignored them, the same way as a flick area.

**Focus.** When the current page changes and the focused widget is inside the old page, the view
saves that widget against the old page, then focuses the new page's saved widget if it is still in
that page's tree (checked by searching the tree for the pointer, never dereferencing it), or its first
focusable widget, or clears focus if it has none. This is a small per-page memory of its own, because
`Window`'s [focus memory](#focus-memory) restores only while focus is still inside the key widget.

### Theme and style structs

Each control has a plain aggregate `*Style` struct (`widget/controls/style/*.h` - `ButtonStyle`,
`CheckboxStyle`, `TextFieldStyle`, ...) holding every themeable value with a sensible default,
and a matching `State<XStyle>` member on `Theme` (`theme.h`), owned by `Application` and
reached via `Application::instance()->theme()`. Every control exposes the same two-overload
setter:

```cpp
decltype(auto) style(this auto&& self, const ButtonStyle& style) {
    self.m_style.set(style);
    return std::forward<decltype(self)>(self);
}

decltype(auto) style(this auto&& self, State<ButtonStyle>& style) {
    self.m_style.set(style);
    return std::forward<decltype(self)>(self);
}
```

Two overloads for the same reason `dragXAxis`/`dragYAxis` above don't take a generic
`PropertyArg<XStyle>`: a struct-valued property meant to be constructed with designated
initializers at the call site needs its parameter to accept that struct type directly, not a
wrapper requiring a second implicit conversion. Every control binds to its theme's style by
default in its own constructor, then reacts to it via `onChange` calling a private
`applyStyle(const XStyle&)` that forwards each field to the matching individual setter - so a
live `theme().button.set(...)` restyles every existing (and future) `ButtonWidget` at once,
while an individual widget's own explicit setter call after `.style(...)` still overrides just
that one field on just that one instance.

A style struct can nest another control's style struct for a composite control -
`SpinBoxStyle` holds a `TextFieldStyle field` and a `ButtonStyle stepperButton` alongside its
own background/border fields, so `Theme::spinBox` styles the whole composite (background,
text, and buttons) from one struct; `SpinBoxWidget::applyStyle` forwards `value.field` to the
inner `TextFieldWidget`'s own `style()` and `value.stepperButton` to both inner buttons'.

### The interaction-color pattern

Every control with state-dependent color (idle/hover/pressed/disabled) follows the same
shape: plain `Property<Paint>` inputs, one `AnimatedState<Paint>` output bound to the actual
drawn slot, and a private `updateColor(bool animate)` re-evaluated on every input or state
change:

```cpp
void ButtonWidget::updateColor(bool animate) {
    const Paint& target = !enabled() ? m_disabledColor.get()
                         : pressed() ? m_pressedColor.get()
                         : hovered() ? m_hoverColor.get()
                         : m_idleColor.get();
    m_color.animateTo(target, (animate && m_settled) ? m_transition.get() : 0.0f);
}
```

`m_settled` (set `true` on a control's first `render()`) gates the *duration* only, not the
color itself: `updateColor` runs from the constructor too (so a control built already-disabled
resolves its correct color immediately), but `animate && m_settled` is false at that point, so
`animateTo` snaps instead of easing - a control never visibly animates into its starting state
on the very first frame it's shown. The same shape recurs for border color (`TextFieldWidget`,
`SpinBoxWidget`) and any other state-dependent `Paint`, just with a different priority chain
(TextField and SpinBox also check `focused()` for a highlighted border).

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

### Selection

`TextWidget::selection(start, end)` marks a byte range, and `Renderer::renderText` draws it in two
steps around the glyph loop.

First, before any glyph, each laid-out line the range covers gets a highlight behind it. The range
is cut to that line's bytes, and the x where the cut starts and its width are summed from the same
glyph advances the glyph loop uses. The rectangle is drawn with the rectangle shader, placed by the
text's world matrix times a translation, so a rotated or tilted text widget carries its highlight
along, and a range over three wrapped lines draws three rectangles.

Then the glyph loop recolors the selected characters:

```cpp
while (i < end) {
    const bool selected = original && i >= selectionStart && i < selectionEnd;
    Utf8Decoded d = decodeUtf8At(source, i);
    i += d.length;
    const GlyphMetrics& m = face->glyph(d.codepoint);
    if (!m.valid) continue;
    if (selected != drawingSelected) {
        flush();
        applyPaintUniforms(textShader, "u", "uColor", selected ? text.selectedTextColor() : text.color(), op);
        drawingSelected = selected;
    }
    emitGlyph(m, penX, baselineY);
}
```

When a glyph's byte position crosses into or out of the range, the glyphs collected so far are drawn
in the current color before the color uniform switches, so a selection in the middle of a line costs
at most two extra draw calls. Elided lines draw a separate shortened string whose bytes don't match
the text's (`original` is false for them), so they get neither a highlight nor recoloring.

## Resource embedding

`cmake/TavoosResources.cmake`'s `tavoos_add_resources(target [BASE_DIR dir] file...)` generates
one `.cpp` per listed file at configure/build time (`GenerateResource.cmake`), each defining a
`static` byte array and a static initializer that calls `ResourceRegistry::registerResource`.
The path as written in the call is the registered path, and the file itself is looked up
relative to `BASE_DIR` (the calling `CMakeLists.txt`'s directory by default), so targets in
different directories can embed one shared asset folder under the same `resource:/` paths:

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

A third scheme, `data:`, embeds raw content (typically an inline SVG string) directly in
source rather than referencing a separate asset - `parseResourcePath` strips the prefix and
hands the rest through as-is:

```cpp
if (uri.rfind("data:", 0) == 0) return { PathScheme::Data, uri.substr(5) };
```

`SVGWidget::source("data:" + svgString)` parses it immediately (no lifetime concern - lunasvg
copies what it needs). Fonts are different: FreeType keeps a live pointer into the font bytes
for the `FT_Face`'s entire lifetime, so `FontFace` owns a persistent `m_ownedFontData` buffer
for `data:`-sourced fonts (`fontface.cpp`), rather than parsing-and-discarding like the SVG
case.

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
6. If setting a property triggers work that needs the whole widget, such as its children or
   slots, ignore that callback while `!built()` and do the work once in `onBuilt()`.
