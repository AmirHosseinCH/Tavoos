# Tavoos Guide

A guide to building UIs with Tavoos: core concepts, every property common to all widgets,
each concrete widget's own properties, types, animation, and resource embedding. If you're
looking to understand how Tavoos works *internally* instead, see
[ARCHITECTURE.md](ARCHITECTURE.md).

For a working, buildable starting point, see [examples/main.cpp](../examples/main.cpp) and the
quick-start example in [README.md](../README.md).

## Table of contents

1. [Core concepts](#core-concepts)
2. [Common `Widget` properties](#common-widget-properties)
   - [Dynamic children](#dynamic-children)
3. [Types reference](#types-reference)
4. [Per-widget reference](#per-widget-reference)
   - [`RectangleWidget`](#rectanglewidget)
   - [`TextWidget`](#textwidget)
   - [`ImageWidget`](#imagewidget)
   - [`SVGWidget`](#svgwidget)
   - [`ColumnWidget` / `RowWidget`](#columnwidget--rowwidget)
   - [`GridWidget`](#gridwidget)
   - [`FlexWidget`](#flexwidget)
5. [Animation](#animation)
6. [Resource embedding](#resource-embedding)
7. [Keyboard & focus](#keyboard--focus)

## Core concepts

**Application and Window.** One `Application` per process; one or more `Window`s, each a
subclass overriding `build()`:

```cpp
class MainWindow : public Tavoos::Window {
public:
    void build() override {
        // declare your widget tree here
    }
};

int main() {
    Tavoos::Application app;
    TB::createWindowOf<MainWindow>([](Tavoos::Window& window) {
        window.width(800).height(600).title("My App").color(Tavoos::Color::White);
    });
    return app.run();
}
```

**The builder pattern.** `using TB = Tavoos::Builder;` and then `TB::Rectangle(...)`,
`TB::Text(...)`, `TB::Column(...)`, etc., each taking a lambda that configures the widget and
declares its children by calling more `TB::` functions inside it:

```cpp
TB::Rectangle([](Tavoos::RectangleWidget& r) {
    r.width(200).height(80).color(Tavoos::Color::Blue);

    TB::Text([](Tavoos::TextWidget& t) {
        t.text("Hello").alignment(Tavoos::Alignment::Center);
    });
});
```

Every setter returns a reference to the widget, so calls chain: `r.width(200).height(80)
.color(...)`. This only works inside `build()` - to add widgets later (e.g. from a click
handler), see [Dynamic children](#dynamic-children) below.

**Reactive properties: `Property<T>`, `State<T>`, and plain values.** Every setter accepts
either a plain value or a `State<T>&` of that *exact* property type - note that a color
property is typed `Paint` (a solid `Color` converts to it implicitly for plain values, but a
bound `State<T>` must match `T` exactly):

```cpp
Tavoos::State<Tavoos::Paint> boxColor{Tavoos::Color::Red};

TB::Rectangle([&](Tavoos::RectangleWidget& r) {
    r.color(boxColor);   // bound - changes when boxColor changes
});

boxColor.set(Tavoos::Color::Green);  // the rectangle updates automatically
```

Use a plain value when the property never changes after construction; use a `State<T>` when
you need to update it later from outside the widget (a click handler, a timer, another
widget's callback) and want every bound widget to pick up the change automatically. A
`State<T>` can be shared across multiple widgets/properties at once.

For value transitions (not just instant swaps), see [Animation](#animation).

## Common `Widget` properties

Every widget (Rectangle, Text, Image, SVG, Column, Row, Grid, Flex) has all of these:

| Property | Type | Notes |
|---|---|---|
| `x()`, `y()` | `int` | Explicit position; ignored if `alignment()` sets a flag on that axis. |
| `width()`, `height()` | `int` | Explicit size; ignored if `fill()` sets a flag on that axis, or (for containers/text) if the widget has an intrinsic size and this is left at `0`. |
| `alignment()` | `Alignment` | `Left`/`Right`/`CenterHorizontal` and `Top`/`Bottom`/`CenterVertical` (combine with `\|`), or `Center` for both. Positions the widget within its available area instead of using `x()`/`y()`. |
| `fill()` | `Fill` | `Width`/`Height`/`Both`. Stretches the widget to fill available space on that axis instead of using `width()`/`height()`. Inside Row/Column/Flex, a filled item shares leftover space evenly with other filled siblings; inside Grid, it stretches to its cell. |
| `padding()`, `paddingLeft/Top/Right/Bottom()` | `float` | Inset applied to *this* widget's children's available area (only meaningful if this widget has children). |
| `margin()`, `marginLeft/Top/Right/Bottom()` | `float` | Space around this widget, respected by its parent container. |
| `rotation()`, `scale()` | `float` | Degrees / scale factor, applied around the widget's own center for rendering and hit-testing. |
| `opacity()` | `float` | `0`-`1`; multiplies with ancestors' opacity for the effective value used at render time. |
| `z()` | `int` | Stacking order among this widget's siblings, for both rendering and hit-testing - higher paints on top and receives clicks first on overlap. Default `0`; ties keep document order. Does **not** affect layout position - siblings with different `z()` still lay out exactly as if `z()` were unset. |
| `visible()` | `bool` | Hidden widgets are skipped for layout, rendering, and hit-testing. |
| `focusable()` | `bool` | Whether Tab/Shift+Tab can focus this widget - see [Keyboard & focus](#keyboard--focus). |
| `clip()` | `bool` | Clips this widget's own rendering (and its children's) to its shape - for `RectangleWidget` that includes rounded corners. |
| `gridRow()`, `gridColumn()`, `gridRowSpan()`, `gridColumnSpan()` | `int` | Only meaningful as a direct child of `GridWidget` - see [GridWidget](#gridwidget). |
| `alignmentAnimation(enabled, duration, easing)` | - | When enabled, changes to resolved *position* (from alignment or x/y changing) ease over `duration` seconds instead of snapping. Children always see the final layout immediately - only this widget's own rendered position eases. |
| `fillAnimation(enabled, duration, easing)` | - | Same, for resolved *size*. |

Plus, on every widget:

- **Event callbacks** - `onClick`, `onPress`, `onRelease`, `onDoubleClick`, `onMouseMove`, `onMouseEnter`, `onMouseLeave` (all take `std::function<void(MouseEvent&)>`), `onWheel` (`WheelEvent&`), `onKeyPress`, `onKeyRelease`, `onTextInput` (`KeyEvent&`), `onFocusIn`, `onFocusOut` (`Event&`). Keyboard/focus callbacks only fire on the currently-focused widget (see [Keyboard & focus](#keyboard--focus)).
- **`focus()`** - programmatically focuses this widget.

### Dynamic children

To add or remove widgets *after* `build()` has already run (e.g. from an event handler),
`TB::` functions won't work - use these instead:

```cpp
someContainer.addChild<Tavoos::RectangleWidget>([](Tavoos::RectangleWidget& r) {
    r.width(100).height(40).color(Tavoos::Color::Cyan);
    // nested children of a dynamically-added widget must also use addChild<T>, not TB::
});
```

- **`T* addChild<T>(std::function<void(T&)> body = {})`** - constructs `T` as a new child, runs `body` on it, and returns the pointer (which stays valid for as long as the widget remains in the tree).
- **`removeSelf()`** - detaches this widget (and its subtree) from its parent. Safe to call from within the widget's own event handler.
- **`removeChild(Widget* child)`** - same, called on the parent for a specific child.

## Types reference

**`Color`** - named constants (`Color::White`, `Black`, `Red`, `Green`, `Blue`, `Yellow`,
`Cyan`, `Magenta`, `Gray`, `LightGray`, `DarkGray`, `Orange`, `Purple`, `Pink`, `Brown`,
`Transparent`) or `Color::rgba(r, g, b, a = 255)` with 0-255 channel values.

**`Paint`** - anywhere a widget takes a color (`RectangleWidget::color`, `borderColor`,
`SVGWidget::color`, `TextWidget::color`), it actually takes a `Paint`, which converts
implicitly from `Color` for a solid fill, or can be a gradient:

```cpp
r.color(Tavoos::linearGradient({
    {0.0f, Tavoos::Color::Orange},
    {1.0f, Tavoos::Color::Purple},
}, /*angleDegrees=*/45.0f));
```

`linearGradient(stops, angle)`, `radialGradient(stops, centerX, centerY, radius)`, and
`conicGradient(stops, centerX, centerY, startAngle)` are all free functions in `types.h`;
`centerX`/`centerY`/`radius` are in `0`-`1` widget-relative units.

**`Alignment`** (bit flags, combine with `|`): `Left`, `Right`, `Top`, `Bottom`,
`CenterHorizontal`, `CenterVertical`, `Center` (= both centers).

**`Fill`** (bit flags): `Width`, `Height`, `Both`.

## Per-widget reference

### `RectangleWidget`

A solid or gradient-filled, optionally rounded and bordered rectangle - the basic visual
building block (buttons, cards, backgrounds are all built from these today; dedicated
control widgets are planned for phase 2).

| Property | Type |
|---|---|
| `color()` | `Paint` |
| `radius()` | `int` (all four corners) |
| `radiusTopLeft/TopRight/BottomRight/BottomLeft()` | `int` |
| `borderWidth()` | `float` |
| `borderColor()` | `Paint` |

### `TextWidget`

| Property | Type |
|---|---|
| `text()` | `std::string` |
| `family()` | `std::string` - must match a name passed to `Application::registerFont()`; falls back to the system default font if empty/unregistered. |
| `weight()` | `FontWeight` - `Thin`/`ExtraLight`/`Light`/`Regular`/`Medium`/`SemiBold`/`Bold`/`ExtraBold`/`Black`. |
| `style()` | `FontStyle` - `Normal`/`Italic`/`Oblique`. |
| `fontSize()` | `float` |
| `color()` | `Paint` |
| `textAlignment()` | `Alignment` - alignment of the text *within* the widget's own resolved bounds. |
| `wrapMode()` | `WrapMode` - `NoWrap`/`WordWrap`/`WrapAnywhere`. |
| `elideMode()` | `ElideMode` - `None`/`Left`/`Right`/`Middle`, applied when a line still doesn't fit after wrapping. |
| `maxLines()` | `int` - `0` means unlimited. |

### `ImageWidget`

Loads JPEG/PNG via `source()` (a `file:`, `resource:`, or bare filesystem path).

| Property | Type |
|---|---|
| `source()` | `std::string` |
| `fillMode()` | `ImageFillMode` - `Stretch`/`PreserveAspectFit`/`PreserveAspectCrop`/`Tile`/`TileVertically`/`TileHorizontally`. |
| `naturalWidth()`, `naturalHeight()` | `int` (read-only, from the decoded image) |

### `SVGWidget`

Rasterizes an SVG (via lunasvg) via `source()`.

| Property | Type |
|---|---|
| `source()` | `std::string` |
| `color()` | `Paint` - recolors the SVG if set (see `hasColor()`). |
| `hasColor()` | `bool` (read-only) - whether `color()` was explicitly set, vs. using the SVG's own colors. |
| `naturalWidth()`, `naturalHeight()` | `int` (read-only) |

### `ColumnWidget` / `RowWidget`

Stack children vertically (Column) or horizontally (Row).

| Property | Type |
|---|---|
| `spacing()` | `float` - gap between consecutive children. |

Children use the common `fill()`/`alignment()` properties for main-/cross-axis sizing and
positioning - see [Common Widget properties](#common-widget-properties).

### `GridWidget`

Auto-flow grid placement, with explicit row/column and spans available per-child via the
common `gridRow()`/`gridColumn()`/`gridRowSpan()`/`gridColumnSpan()` properties.

| Property | Type |
|---|---|
| `columns()` | `int` |
| `columnSpacing()`, `rowSpacing()` | `float` |

```cpp
TB::Grid([](Tavoos::GridWidget& grid) {
    grid.columns(3).columnSpacing(8).rowSpacing(8);

    TB::Rectangle([](Tavoos::RectangleWidget& r) {
        r.fill(Tavoos::Fill::Width).height(46).gridColumnSpan(2);  // spans 2 columns
    });
    TB::Rectangle([](Tavoos::RectangleWidget& r) {
        r.fill(Tavoos::Fill::Width).height(46);  // auto-placed in the next free cell
    });
});
```

### `FlexWidget`

Direction/wrap/justify/align - not grow/shrink/basis/order.

| Property | Type |
|---|---|
| `direction()` | `FlexDirection` - `Row`/`Column`. |
| `wrap()` | `FlexWrap` - `NoWrap`/`Wrap`. |
| `justifyContent()` | `FlexJustify` - `Start`/`Center`/`End`/`SpaceBetween`/`SpaceAround`/`SpaceEvenly`. |
| `alignItems()` | `Alignment` - cross-axis alignment for items within a line; a child's own `alignment()` overrides this per-item. |
| `alignContent()` | `FlexAlign` - `Start`/`Center`/`End`/`Stretch`/`SpaceBetween`/`SpaceAround`/`SpaceEvenly`, distribution of *lines* when wrapped. |
| `rowGap()`, `columnGap()`, `gap()` | `float` - `gap()` sets both. |

## Animation

**`AnimatedState<T>`** - a `State<T>` that can ease toward a new value over time instead of
jumping instantly. Works for any `T` with a `lerp(a, b, t)` free function - built in for
`float`, `Color`, and `Paint`.

```cpp
Tavoos::AnimatedState<float> boxScale{1.0f};

TB::Rectangle([](Tavoos::RectangleWidget& r) {
    r.scale(boxScale).onClick([](Tavoos::MouseEvent&) {
        boxScale.animateTo(boxScale + 0.1f, /*seconds=*/0.2f, Tavoos::Easing::easeInOutQuad);
    });
});
```

Available easing functions (`Tavoos::Easing::`, in `easing.h`): `linear`, `easeInQuad`,
`easeOutQuad`, `easeInOutQuad`. `EasingFn` is a plain `std::function<float(float)>`, so you
can pass your own.

For position/size transitions specifically (rather than an arbitrary property), see
`alignmentAnimation()`/`fillAnimation()` in the [common properties table](#common-widget-properties)
instead - those ease automatically whenever alignment/fill-driven layout changes, no
`AnimatedState` needed.

To animate a custom property over time yourself (not covered by `AnimatedState`), subclass
`AnimatableBase`, implement `tick(float dt) -> bool` (return `false` when the animation is
done), and call `AnimationManager::instance().registerAnimation(this)` /
`unregisterAnimation(this)` to start/stop it - this is the same mechanism `AnimatedState`
itself is built on.

## Resource embedding

Assets (fonts, images) can be compiled directly into your binary instead of shipped as loose
files, via CMake:

```cmake
tavoos_add_resources(your_app assets/fonts/Inter-Regular.ttf)
```

Then reference them with a `resource:/` path instead of a filesystem path:

```cpp
app.registerFont("Inter", "resource:/assets/fonts/Inter-Regular.ttf");
```

A plain path (or an explicit `file:` prefix) loads from the filesystem at runtime instead.

## Keyboard & focus

Set `.focusable(true)` on any widget to make it part of the Tab order. Tab/Shift+Tab cycle
forward/backward through all focusable, visible widgets in the tree (depth-first order),
wrapping at the ends. The currently-focused widget receives:

- `onKeyPress` / `onKeyRelease` - `KeyEvent::keyCode()` compares against the `Key` enum (`Tavoos::Key::Enter`, `Tavoos::Key::A`, etc., matching GLFW key codes) and `.modifiers()` (`KeyModifier::Shift/Control/Alt/Super`, combine with `|`).
- `onTextInput` - fires per Unicode codepoint typed (for building text-entry widgets).
- `onFocusIn` / `onFocusOut` - fired when focus moves onto/off of this widget, including on window activation/deactivation if it was already focused.

Call `.focus()` on a widget to focus it programmatically instead of waiting for Tab.
