# Tavoos Guide

A guide to building UIs with Tavoos: core concepts, every property common to all widgets,
each concrete widget's own properties, types, animation, and resource embedding. If you're
looking to understand how Tavoos works *internally* instead, see
[ARCHITECTURE.md](ARCHITECTURE.md).

For a working, buildable starting point, check the [examples](../examples) folder and the
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
5. [Controls](#controls)
   - [`ButtonWidget`](#buttonwidget)
   - [`CheckboxWidget`](#checkboxwidget)
   - [`RadioWidget` / `ButtonGroup`](#radiowidget--buttongroup)
   - [`SwitchWidget`](#switchwidget)
   - [`ProgressBarWidget`](#progressbarwidget)
   - [`SliderWidget`](#sliderwidget)
   - [`TextFieldWidget`](#textfieldwidget)
   - [`SpinBoxWidget`](#spinboxwidget)
   - [`PopupWidget`](#popupwidget)
   - [`FlickAreaWidget`](#flickareawidget)
   - [`ScrollAreaWidget`](#scrollareawidget)
   - [`StackViewWidget`](#stackviewwidget)
6. [Theme & styling](#theme--styling)
7. [Custom controls](#custom-controls)
8. [Components](#components)
9. [Dragging](#dragging)
10. [Animation](#animation)
11. [Resource embedding](#resource-embedding)
12. [Keyboard & focus](#keyboard--focus)

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

A `State<T>` can also be observed directly, independent of any bound `Property`:

```cpp
checkbox.checkedState().onChange([](const bool& checked) { /* ... */ });
```

Useful for reacting to a control's value from outside it - every control's `*State()` getter
(`checkedState()`, `valueState()`, `focusedState()`, ...) returns a `State<T>&` for exactly
this.

For value transitions (not just instant swaps), see [Animation](#animation).

## Common `Widget` properties

Every widget (Rectangle, Text, Image, SVG, Column, Row, Grid, Flex) has all of these:

| Property | Type | Notes |
|---|---|---|
| `x()`, `y()` | `int` | Explicit position; ignored if `alignment()` sets a flag on that axis. |
| `width()`, `height()` | `int` | Explicit size; ignored if `fill()` or `widthFraction()`/`heightFraction()` sets that axis, or (for containers/text) if the widget has an intrinsic size and this is left at `0`. |
| `widthFraction()`, `heightFraction()` | `float` | Size as a fraction (`0`-`1`) of the parent's available area on that axis, resolved in the same layout pass - e.g. `widthFraction(0.5f)` is always half the parent's width, at any size. Unset (the default) means ignored. Priority is `fill()`, then fraction, then `width()`/`height()`. Like `alignment()`, only applies under a non-container parent (not inside Row/Column/Grid/Flex). |
| `xFraction()`, `yFraction()` | `float` | Position as a fraction (`0`-`1`) of the free space left in the parent after this widget's own size and margins: `0` is the leading edge, `1` the trailing edge, `0.5` centered. Applies when no alignment flag is set on that axis and takes priority over `x()`/`y()`. Same non-container-parent rule as the size fractions. |
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

- **Event callbacks** - `onClick`, `onPress`, `onRelease`, `onDoubleClick`, `onMouseMove`, `onMouseEnter`, `onMouseLeave` (all take `std::function<void(MouseEvent&)>`), `onWheel` (`WheelEvent&`), `onKeyPress`, `onKeyRelease`, `onTextInput` (`KeyEvent&`), `onFocusIn`, `onFocusOut` (`Event&`). Keyboard/focus callbacks only fire on the currently-focused widget (see [Keyboard & focus](#keyboard--focus)). `MouseEvent::x()`/`y()` are always relative to the widget whose handler is currently running, not the window.
- **`focus()`** - programmatically focuses this widget.
- **Coordinate mapping** - `mapToParent`/`mapFromParent` and `mapToWindow`/`mapFromWindow` (all `Point mapX(const Point&) const`) convert a point between this widget's local space and its parent's or the window's - useful when building a custom composite that needs to translate a coordinate across that boundary itself.

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

A [component](#components) is built the same way when created with `addChild<T>`.

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

**`Font`** - a plain bundle of `family` (`std::string`), `size` (`float`), `weight`
(`FontWeight`), and `style` (`FontStyle`), for controls whose `font()` setter takes the whole
group at once instead of the four separate `TextWidget` properties:

```cpp
b.font(Tavoos::Font{"Inter", 16.0f, Tavoos::FontWeight::Medium});
```

Same two-overload shape as every `style()` setter (a plain `Font` or a bound `State<Font>&`).

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

## Controls

Each control is a **behavior base** plus visuals: the base owns the state and interaction, and
the widgets below supply the look and a `style()`. Every control derives `Control`, which has:

| Property | Type | Notes |
|---|---|---|
| `enabled()` | `bool` | Disables interaction (also dims via each control's own `disabledColor`-style properties below). |
| `hovered()` | `bool` (read-only) | Live pointer state. |
| `enabledState()`, `hoveredState()` | `State<bool>&` | Reactive access to the above. |
| `background<W>(body)`, `content<W>(body)` | - | Replace either slot with your own widget. `background` fills the control; `content` has no default layout - its body positions it. |

Button, Checkbox, Radio, and Switch derive `ButtonBase`, which adds:

| Property | Type | Notes |
|---|---|---|
| `pressed()`, `pressedState()` | `bool` / `State<bool>&` | True while held with the mouse or Space. |
| `checkable()` | `bool` | Whether a click toggles `checked()`; default `false`. |
| `checked()`, `checkedState()` | `bool` / `State<bool>&` | The toggle state. |
| `exclusive()` | `bool` | When `true`, a click only checks, never unchecks. |
| `group(ButtonGroup&)` | - | Joins a mutually exclusive group (see [RadioWidget / ButtonGroup](#radiowidget--buttongroup)). |

All controls also support `focus()`/`focused()`/`focusedState()` (from `Widget` itself - see
[Keyboard & focus](#keyboard--focus)) and a `style()`/`Theme` pair - see
[Theme & styling](#theme--styling). To build your own control on these bases, see
[Custom controls](#custom-controls).

### `ButtonWidget`

```cpp
TB::Button([](Tavoos::ButtonWidget& b) {
    b.text("Save").variant(Tavoos::ButtonVariant::Filled).onClick([](Tavoos::MouseEvent&) { /* ... */ });
});
```

| Property | Type | Notes |
|---|---|---|
| `text()` | `std::string` |
| `font()` | `Font` |
| `variant()` | `ButtonVariant` - `Filled`/`Outlined`/`Text`. Sets idle/hover/pressed/disabled colors, border width, and text color as a preset - call `variant()` *before* any individual color override, since it unbinds them. |
| `icon()` | `std::string` - an SVG source (`resource:/`, `file:`, `data:`, or bare path). |
| `iconSize()` | `int` |
| `iconPosition()` | `ButtonIconPosition` - `Left`/`Right`. |
| `iconSpacing()` | `float` |
| `display()` | `ButtonDisplay` - `TextAndIcon`/`TextOnly`/`IconOnly`. |
| `radius()` | `int` |
| `idleColor()`, `hoverColor()`, `pressedColor()`, `disabledColor()` | `Paint` |
| `textColor()`, `disabledTextColor()` | `Paint` |
| `borderWidth()`, `borderColor()`, `disabledBorderColor()` | `float` / `Paint` |
| `transition()` | `float` - seconds, for color transitions between interaction states. |

Subclass `ButtonBase` (not `ButtonWidget`) directly for a custom control with its own
interaction states but no built-in color/variant system - react to `hoveredState()`/
`pressedState()`/`enabledState()` yourself.

### `CheckboxWidget`

```cpp
TB::Checkbox([](Tavoos::CheckboxWidget& c) { c.checked(true); });
```

A box that toggles `checked()` on click/Space/Enter - build your own label alongside it
(`TB::Row { Checkbox, Text }`), it isn't built in.

| Property | Type | Notes |
|---|---|---|
| `checked()` | `bool` |
| `checkedState()` | `State<bool>&` |
| `uncheckedColor()`, `checkedColor()`, `disabledColor()` | `Paint` - box fill. |
| `uncheckedBorderColor()`, `checkedBorderColor()`, `disabledBorderColor()` | `Paint` |
| `checkColor()`, `disabledCheckColor()` | `Paint` - the checkmark itself. |
| `radius()` | `int` |
| `transition()` | `float` |

### `RadioWidget` / `ButtonGroup`

A selectable dot: a `ButtonBase` that is `checkable` and `exclusive`, so a click only ever
checks it. Mutual exclusion is explicit - nothing is auto-grouped by proximity:

```cpp
class SettingsWindow : public Tavoos::Window {
public:
    void build() override {
        TB::Column([this](Tavoos::ColumnWidget& col) {
            TB::Radio([this](Tavoos::RadioWidget& r) { r.group(m_group).checked(true); });
            TB::Radio([this](Tavoos::RadioWidget& r) { r.group(m_group); });
        });
    }

private:
    Tavoos::ButtonGroup m_group;
};
```

| Property | Type | Notes |
|---|---|---|
| `checked()`, `checkedState()`, `group(ButtonGroup&)` | - | Inherited from `ButtonBase`. |
| `uncheckedColor()`, `checkedColor()`, `disabledColor()` | `Paint` |
| `uncheckedBorderColor()`, `checkedBorderColor()`, `disabledBorderColor()` | `Paint` |
| `radius()` | `int` - defaults to a perfect circle (`size/2`), tracked automatically across resizes until you call `radius()` explicitly. |
| `transition()` | `float` |

`ButtonGroup::checked()` returns the currently checked `ButtonBase*` (or `nullptr`). A group works
with any checkable button, not just radios - checkable `ButtonWidget`s in one group make a
segmented control. It isn't a widget, just a coordinator: keep it alive as long as its buttons are
in use - as a member next to the widgets, not a local inside `build()`. A button that is destroyed
leaves its group automatically, and if the group is destroyed first its buttons simply become
ungrouped.

### `SwitchWidget`

```cpp
TB::Switch([](Tavoos::SwitchWidget& s) { s.checked(false); });
```

| Property | Type | Notes |
|---|---|---|
| `checked()` | `bool` |
| `checkedState()` | `State<bool>&` |
| `uncheckedColor()`, `checkedColor()`, `disabledColor()` | `Paint` - track. |
| `thumbColor()`, `checkedThumbColor()`, `disabledThumbColor()` | `Paint` |
| `transition()` | `float` |

Always a pill shape (track radius = height/2) and a circular thumb - no radius override, since
being a pill is the whole visual identity of a switch.

### `ProgressBarWidget`

```cpp
TB::ProgressBar([](Tavoos::ProgressBarWidget& p) { p.minValue(0).maxValue(100).value(40); });
```

Determinate only. Built on `ProgressBarBase`, and purely a display: no press or focus.

| Property | Type | Notes |
|---|---|---|
| `value()`, `minValue()`, `maxValue()` | `int` | Default range `0`-`100`. |
| `valueState()` | `State<int>&` |
| `position()`, `positionState()` | `float` / `State<float>&` | The value as a `0`-`1` fraction of the range. |
| `trackColor()`, `fillColor()` | `Paint` |
| `radius()` | `int` |
| `transition()` | `float` |

### `SliderWidget`

```cpp
TB::Slider([](Tavoos::SliderWidget& s) { s.minValue(0).maxValue(100).value(50); });
```

Built on `SliderBase`: drag the handle, or press the track to jump the value to that spot.

| Property | Type | Notes |
|---|---|---|
| `value()`, `minValue()`, `maxValue()` | `int` |
| `valueState()` | `State<int>&` |
| `position()`, `positionState()` | `float` / `State<float>&` | The value as a `0`-`1` fraction of the range. |
| `pressed()`, `pressedState()` | `bool` / `State<bool>&` | True while the handle or track is held. |
| `trackColor()`, `fillColor()`, `thumbColor()` | `Paint` |
| `disabledColor()`, `disabledThumbColor()` | `Paint` |
| `transition()` | `float` |

### `TextFieldWidget`

```cpp
TB::TextField([](Tavoos::TextFieldWidget& f) {
    f.placeholder("Type here...").onSubmit([](const std::string& text) { /* Enter pressed */ });
});
```

Single-line text entry - no selection/clipboard yet.

| Property | Type | Notes |
|---|---|---|
| `text()` | `std::string` |
| `textState()` | `State<std::string>&` |
| `placeholder()` | `std::string` - shown (in `placeholderColor()`) whenever `text()` is empty. |
| `font()` | `Font` |
| `backgroundColor()`, `borderColor()`, `focusedBorderColor()` | `Paint` |
| `disabledColor()`, `disabledBorderColor()` | `Paint` |
| `textColor()`, `placeholderColor()`, `caretColor()` | `Paint` |
| `radius()`, `borderWidth()` | `int` / `float` |
| `innerPadding()`, `innerPaddingLeft/Top/Right/Bottom()` | `float` - gap between the border and the text/caret. |
| `transition()` | `float` |
| `onSubmit(fn)` | `std::function<void(const std::string&)>` | Fires on Enter. |

Backspace/Delete/Left/Right/Home/End all work; typed text scrolls horizontally to keep the
caret in view, clipped to stay inside the padded interior regardless of scroll position.

### `SpinBoxWidget`

```cpp
TB::SpinBox([](Tavoos::SpinBoxWidget& s) { s.minValue(0).maxValue(10).step(1); });
```

Built on `SpinBoxBase`: a text field plus up/down stepper buttons, sharing one background.

| Property | Type | Notes |
|---|---|---|
| `value()`, `minValue()`, `maxValue()`, `step()` | `int` | `value` is stored as-is and only clamped for display/on commit. |
| `valueState()` | `State<int>&` |
| `increase()`, `decrease()` | - | Step the value programmatically (respects `enabled()` and the range). |
| `commitText(string)`, `valueText()`, `valueTextState()` | - | What the field commits on Enter, and the clamped value as text. |
| `enabled()` | `bool` | Cascades to the internal field and both buttons. |
| `onValueChange(fn)` | `std::function<void(int)>` |
| `backgroundColor()`, `borderColor()`, `focusedBorderColor()` | `Paint` |
| `disabledColor()`, `disabledBorderColor()` | `Paint` |
| `radius()`, `borderWidth()` | `int` / `float` |
| `transition()` | `float` |

Typing a value commits on Enter (clamped to `[minValue, maxValue]`) or reverts to the current
value if what you typed doesn't parse as an integer; the up/down buttons commit immediately.

### `PopupWidget`

```cpp
Tavoos::PopupWidget* menu = nullptr;

TB::Button([](Tavoos::ButtonWidget& b) {
    b.text("Menu").onClick([](Tavoos::MouseEvent&) { menu->open(); });

    TB::Popup([](Tavoos::PopupWidget& p) {
        menu = &p;
        p.placement(Tavoos::Placement::Bottom).offset(6);
        TB::Column([](Tavoos::ColumnWidget& c) {
            c.spacing(4);
            TB::Button([](Tavoos::ButtonWidget& item) {
                item.text("Open").onClick([](Tavoos::MouseEvent&) { menu->close(); });
            });
        });
    });
});
```

A popup is declared **inside its anchor** and its children go inside it - there is no `content`
slot. It is hidden until opened, then drawn above everything else in the window, unclipped by the
anchor's ancestors, and sized to its children plus padding (set `width`/`height` to override).
Built on `PopupBase`, itself built on `OverlayBase`.

| Property | Type | Notes |
|---|---|---|
| `open()`, `close()` | - | Show / hide it. Opening twice or closing twice is a no-op. |
| `opened()`, `openedState()` | `bool` / `State<bool>&` | Whether it is open. |
| `onOpen(fn)`, `onClose(fn)` | `std::function<void()>` | Fired on each change. |
| `placement()` | `Placement` | `Bottom` (default), `Top`, `Left`, `Right`, `Center`. |
| `target()` | `PlacementTarget` | `Parent` (default) places it against the anchor, `Window` against the window. |
| `offset()`, `offsetLeft()` / `offsetTop()` / `offsetRight()` / `offsetBottom()` | `float` | Gap from the anchor, or inset from the window edge, for the side it ends up on. `offset(v)` sets all four. |
| `x()`, `y()` | `int` | Explicit position from the target's top-left corner. Overrides placement per axis; not clamped. |
| `closePolicy()` | `ClosePolicy` | Combine `ClickOutside` and `Escape` with `\|`; `None` for neither. Default: both. |
| `modal()` | `bool` | Blocks pointer input behind the popup, dims the window, and confines Tab to the popup. Read when it opens. |
| `enter<T>(args...)`, `exit<T>(args...)` | transition | Animate opening and closing. Default: a short fade in and out. An empty factory means no animation. |
| `background<W>(body)`, `scrim<W>(body)` | - | Replace the popup's background or the dimming layer with your own widget. |
| `backgroundColor()`, `borderColor()`, `scrimColor()` | `Paint` | |
| `borderWidth()`, `radius()` | `float` / `int` | |
| `padding()` | `float` | The inner spacing around the children (default 8, set by the style). |

Placement works on both targets. Against the **parent**, `Bottom`/`Top`/`Left`/`Right` put the popup
outside the anchor, start-aligned, and flip to the opposite side when the chosen one has no room;
`Center` centers it over the anchor. Against the **window**, the same values dock it inside that
edge, centered along it, and `Center` centers it in the window. The popup is always kept inside
the window, except where you set `x`/`y` explicitly. Whichever of `x`, `y`, and `placement` you set
last wins.

```cpp
TB::Popup([](Tavoos::PopupWidget& dialog) {
    dialog.target(Tavoos::PlacementTarget::Window)
        .placement(Tavoos::Placement::Center)
        .modal(true)
        .closePolicy(Tavoos::ClosePolicy::Escape)
        .scrimColor(Tavoos::Color::rgba(0, 0, 0, 140));
});
```

A press outside the popup closes it when `ClickOutside` is set. A normal popup lets that press
reach the widget behind it; a modal one swallows it. Escape closes it when `Escape` is set. A press
on the anchor itself counts as outside, so use `open()` in the anchor's click handler instead of
toggling, or the popup closes on press and reopens on release.

A popup fades in and out by default. `enter` and `exit` take a transition type and its constructor
arguments, like the [stack view's slots](#transitions):

```cpp
TB::Popup([](Tavoos::PopupWidget& drawer) {
    drawer.target(Tavoos::PlacementTarget::Window)
        .placement(Tavoos::Placement::Bottom)
        .enter<Tavoos::SlideIn>(Tavoos::Edge::Bottom, 0.25f)
        .exit<Tavoos::SlideOut>(Tavoos::Edge::Bottom, 0.2f);
});
```

Slides travel by the size of the window. While a popup fades out it no longer takes input: Escape
and outside presses skip it, a modal popup stops blocking, and presses reach the widgets behind it.
`onOpen` and `onClose` fire when `open()` and `close()` are called, not when the animation ends.
Opening a popup that is still closing finishes the exit at once and starts the enter.

### `FlickAreaWidget`

```cpp
TB::FlickArea([](Tavoos::FlickAreaWidget& f) {
    f.width(300).height(200).radius(12).backgroundColor(Tavoos::Color::rgba(245, 246, 250));
    TB::Column([](Tavoos::ColumnWidget& c) {
        c.fill(Tavoos::Fill::Width).spacing(8);
        for (int i = 0; i < 20; ++i)
            TB::Button([i](Tavoos::ButtonWidget& b) {
                b.fill(Tavoos::Fill::Width).text("Item " + std::to_string(i));
            });
    });
});
```

A clipped viewport onto content larger than itself. Children go inside it (there is no `content`
slot) and it scrolls with the wheel, with the keyboard, and by dragging the content. It scrolls
only on an axis where the content actually overflows, so content that fits is left alone. Built
on `FlickAreaBase`.

| Property | Type | Notes |
|---|---|---|
| `flickDirection()` | `FlickDirection` | `Horizontal`, `Vertical`, or both with `\|`. Default: both. Use it to lock an axis. |
| `contentX()`, `contentY()` | `float` | The current offset. Setting it jumps there, clamped to the range. |
| `contentXState()`, `contentYState()` | `State<float>&` | Reactive offsets. |
| `contentWidth()`, `contentHeight()` | `float` | The content size. Set it to override; `0` means "from the children". |
| `viewportWidth()`, `viewportHeight()`, `maxContentX()`, `maxContentY()` | `float` | Read-only: the visible size and how far each axis can scroll. |
| `wheelStep()` | `float` | Pixels per wheel notch and per arrow key. Default 48. |
| `dragScroll()` | `bool` | Drag the content with the mouse. Default on. |
| `keyNavigation()` | `bool` | Arrows, Page Up/Down, Home/End. Default on. |
| `scrollTo(x, y)`, `scrollBy(dx, dy)` | - | Move programmatically; clamped. |
| `scrollToTop()`, `scrollToBottom()`, `scrollToLeft()`, `scrollToRight()` | - | Jump to an end. |
| `onScroll(fn)` | `std::function<void(float, float)>` | Fired with the new offsets whenever they change. |
| `background<W>(body)` | - | Replace the background. Its shape is also the clip shape. |
| `backgroundColor()`, `radius()` | `Paint` / `int` | A larger radius rounds the clipped content too. |

For sideways scrolling, set the content size or put children at an `x` beyond the view:

```cpp
TB::FlickArea([](Tavoos::FlickAreaWidget& strip) {
    strip.width(300).height(120)
        .flickDirection(Tavoos::FlickDirection::Horizontal)
        .contentWidth(900);
    TB::Rectangle([](Tavoos::RectangleWidget& r) { r.width(200).height(100).color(Tavoos::Color::Cyan); });
    TB::Rectangle([](Tavoos::RectangleWidget& r) { r.width(200).height(100).x(300).color(Tavoos::Color::Yellow); });
});
```

Dragging inside an area does not click the item the drag started on, and a control inside that
handles its own drag, such as a slider, keeps it. When an inner area reaches its end, further
wheel input scrolls the area around it.

### `ScrollAreaWidget`

```cpp
TB::ScrollArea([](Tavoos::ScrollAreaWidget& area) {
    area.width(300).height(200).radius(8).backgroundColor(Tavoos::Color::rgba(245, 246, 250));
    TB::Column([](Tavoos::ColumnWidget& c) {
        c.fill(Tavoos::Fill::Width).spacing(8);
        for (int i = 0; i < 20; ++i)
            TB::Button([i](Tavoos::ButtonWidget& b) {
                b.fill(Tavoos::Fill::Width).text("Item " + std::to_string(i));
            });
    });
});
```

A scrolling area with scrollbars. It is built on `ScrollAreaBase`, which extends the same
`FlickAreaBase` as [`FlickAreaWidget`](#flickareawidget), so everything on a flick area
applies (`flickDirection`, `contentX`/`contentY`, `contentWidth`/`contentHeight`, `scrollTo`,
`onScroll`, `wheelStep`, and the rest), with one difference in defaults: **dragging the content
and keyboard scrolling are off**, as in most desktop scroll areas. Turn them on with
`dragScroll(true)` and `keyNavigation(true)`. The scrollbars overlay the content; they do not
take space from it.

| Property | Type | Notes |
|---|---|---|
| `barPolicy()` | `BarPolicy` | `Auto` (default): a bar exists only while its axis overflows. `Always`, or `Never`. Sets both axes. |
| `verticalBarPolicy()`, `horizontalBarPolicy()` | `BarPolicy` | The same for one axis. An axis missing from `flickDirection` never shows a bar. |
| `autoHide()` | `bool` | Fade the bars out when idle. Default on. |
| `barHideDelay()`, `barFadeDuration()` | `float` | Seconds before the fade starts (default 1.0) and how long it takes (default 0.2). |
| `minThumbSize()` | `float` | The smallest the thumb gets, in pixels. Default 24. |
| `verticalTrack<W>(body)`, `verticalThumb<W>(body)`, `horizontalTrack<W>(body)`, `horizontalThumb<W>(body)` | - | Replace a bar part with your own widget. |
| `verticalThumbSizeState()`, `verticalThumbPositionState()` (and `horizontal...`) | `State<float>&` | What a custom thumb binds: its length as a fraction of the track and its position. |
| `verticalThumbHoveredState()`, `verticalThumbPressedState()` (and `horizontal...`) | `State<bool>&` | Pointer state of each thumb. |
| `backgroundColor()`, `radius()` | `Paint` / `int` | As on a flick area. |
| `trackColor()` | `Paint` | Default transparent. |
| `thumbColor()`, `thumbHoverColor()`, `thumbPressedColor()` | `Paint` | The thumb in each state. |
| `thumbRadius()`, `barThickness()`, `barMargin()` | `int` / `int` / `float` | Bar shape, and the gap from the edge. |
| `transition()` | `float` | Thumb color transition, in seconds. |

The bars appear when the content scrolls, when the pointer is over the area, and while a thumb is
held, and fade out after `barHideDelay`. Drag a thumb to scroll in proportion, or click the track
to move one page toward the click. The wheel scrolls over the bars as well as the content. To
keep the bars permanently visible:

```cpp
TB::ScrollArea([](Tavoos::ScrollAreaWidget& area) {
    area.width(300).height(200)
        .barPolicy(Tavoos::BarPolicy::Always)
        .autoHide(false)
        .thumbColor(Tavoos::Color::rgba(40, 90, 200, 160))
        .keyNavigation(true);
    TB::Rectangle([](Tavoos::RectangleWidget& r) { r.width(280).height(600).color(Tavoos::Color::Cyan); });
});
```

`Always` shows the bar for every axis `flickDirection` allows, even one that doesn't overflow, as
a full-length thumb. Use `flickDirection` to limit the axes, or `Auto` to show a bar only when
needed.

### `StackViewWidget`

```cpp
class LoginPage : public Tavoos::Component<Tavoos::ColumnWidget> {
public:
    LoginPage(Tavoos::Object* parent) : Component{parent} { spacing(8); }

    TAVOOS_PROPERTY(std::string, heading, "Sign in")

protected:
    void build() override {
        TB::Text([this](Tavoos::TextWidget& t) { t.text(m_heading.state()); });
        TB::TextField([](Tavoos::TextFieldWidget& f) { f.width(200).height(32); });
        TB::Button([this](Tavoos::ButtonWidget& b) {
            b.text("Back").onClick([this](Tavoos::MouseEvent&) { Tavoos::StackViewWidget::of(*this)->pop(); });
        });
    }
};

class HomePage : public Tavoos::Component<Tavoos::ColumnWidget> {
public:
    HomePage(Tavoos::Object* parent) : Component{parent} { spacing(8); }

protected:
    void build() override {
        TB::Text([](Tavoos::TextWidget& t) { t.text("Home"); });
        TB::Button([this](Tavoos::ButtonWidget& b) {
            b.text("Sign in").onClick([this](Tavoos::MouseEvent&) {
                Tavoos::StackViewWidget::of(*this)->push<LoginPage>([](LoginPage& page) { page.heading("Welcome back"); });
            });
        });
    }
};

class MainWindow : public Tavoos::Window {
public:
    void build() override {
        TB::StackView([](Tavoos::StackViewWidget& stack) {
            stack.fill(Tavoos::Fill::Both);
            stack.initialItem<HomePage>();
        });
    }
};
```

A stack of **items** that each fill the view: the item on top is shown, and pushing another covers
it. Items are created by type, so they are usually [components](#components), and the body works like
`TB::Create`'s. The items below the top stay alive and keep their state (text typed into a field is
still there after you go back), and popping an item destroys it. An item reaches the stack it lives
in with `StackViewWidget::of(widget)`. Built on `StackViewBase`. The stack has no size of its own, so
give it one or `fill` it.

| Method or property | Notes |
|---|---|
| `push<T>(body)`, `replace<T>(body)` | Create an item of type `T`, push it or swap it for the top item. Return the new item. |
| `initialItem<T>(body)` | The same as the first `push`, for the start of a `TB::StackView` body. |
| `pop()` | Removes the top item; returns `false` if only one item is left. |
| `popTo(item)`, `popToRoot()` | Remove items down to `item` or to the first one. |
| `clear()` | Removes every item, with no animation. |
| `depth()`, `depthState()` | How many items there are. |
| `currentItem()`, `item(i)`, `indexOf(item)` | The top item, the item at an index (0 is the bottom), and an item's index or -1. |
| `onItemChange(fn)` | Called with the new top item (or null) as soon as an operation starts. |
| `transitionRunningState()` | `State<bool>`: true while a transition is running. |
| `of(widget)` | The stack that `widget` belongs to, or null. |

Focus follows the stack: pushing moves focus into the new item, and popping returns it to the
widget that had it before. Hidden items are skipped by Tab.

#### Transitions

Pushing, popping and replacing animate by default: a slide for push and pop and a crossfade for
replace. A transition is a small class; the stack has six slots (`pushEnter`, `pushExit`,
`popEnter`, `popExit`, `replaceEnter`, `replaceExit`), set with a transition type and its constructor
arguments:

```cpp
TB::StackView([](Tavoos::StackViewWidget& stack) {
    stack.fill(Tavoos::Fill::Both);
    stack.pushEnter<Tavoos::SlideIn>(Tavoos::Edge::Bottom, 0.3f)
        .pushExit<Tavoos::FadeOut>(0.2f)
        .popExit<Tavoos::SlideOut>(Tavoos::Edge::Bottom, 0.3f)
        .replaceEnter<Zoom>(0.4f)
        .popEnter<Tavoos::LambdaTransition>([](Tavoos::Widget& w, float t) { w.opacity(t); }, 0.15f);
    stack.initialItem<HomePage>();
});
```

The presets are `FadeIn`, `FadeOut`, `ScaleIn`, `ScaleOut`, `SlideIn(Edge)` and `SlideOut(Edge)`, plus
`LambdaTransition(fn)` for a one-off effect (`fn(widget, progress)`). Every preset takes a duration and an
optional easing last. An empty slot means no animation, and `clearTransitions()` empties all six.
To override them for one call, pass a `TransitionSet` (an enter factory and an exit factory), or
`TransitionSet::immediate()` for none:

```cpp
stack.push<LoginPage>({}, Tavoos::TransitionSet::immediate());
```

To write your own, derive `Transition` and implement three methods. `prepare` captures the
item's own values and sets the start state, `update` applies the eased progress (0 to 1), and
`finish` puts the values back:

```cpp
class Zoom : public Tavoos::Transition {
public:
    explicit Zoom(float duration = 0.25f) : Transition{duration} {}

    void prepare(Tavoos::Widget& item, float, float) override {
        m_item = &item;
        m_rotation = item.rotation();
        m_opacity = item.opacity();
    }

    void update(float progress) override {
        m_item->rotation(m_rotation + (1.0f - progress) * 45.0f);
        m_item->opacity(m_opacity * progress);
    }

    void finish(bool) override {
        m_item->rotation(m_rotation);
        m_item->opacity(m_opacity);
    }

private:
    Tavoos::Widget* m_item{nullptr};
    float m_rotation{0.0f};
    float m_opacity{1.0f};
};
```

Things to know:
- A transition replaces any binding on the properties it animates (`opacity`, `scale`, the margins
  for slides) for its duration, and restores plain values at the end.
- An operation that arrives while a transition is running finishes it immediately and then starts
  its own.
- The retiring item keeps receiving input until its transition ends. Use `transitionRunningState()`
  to disable your controls while it runs.
- To change the defaults for every stack, set `Theme::stackView` with a `StackViewStyle`.

## Theme & styling

Every control above (`ButtonStyle`, `CheckboxStyle`, ..., `SpinBoxStyle`, `PopupStyle`, `FlickAreaStyle`, `ScrollAreaStyle`, `StackViewStyle` - one plain struct
per control in `tavoos/widget/controls/style/`) has a matching entry on the app-wide theme:

```cpp
Tavoos::Application::instance()->theme().button.set(Tavoos::ButtonStyle{
    .idleColor = Tavoos::Color::rgba(20, 20, 30),
    .radius = 12,
});
```

Every control binds to its theme entry by default, so this restyles every `ButtonWidget` in
the app immediately, including ones created afterward. Two ways to override per-instance,
either works the same on every control:

```cpp
TB::Button([](Tavoos::ButtonWidget& b) {
    b.style(Tavoos::ButtonStyle{ .idleColor = Tavoos::Color::Red });   // whole struct at once
    b.radius(4);                                                       // or one field
});
```

A later call always wins over an earlier one, whether it's `.style(...)` or an individual
setter - except `ButtonWidget::variant()`, which is special-cased to always win once called,
even over a *later* `.style(...)` or a live retheme, so picking `Filled`/`Outlined`/`Text`
sticks until you explicitly call `variant()` again.

## Custom controls

Each behavior base owns the logic and exposes state; you supply the visuals as slot bodies that
bind to it. The bases are in `tavoos/widget/templates/`; include
`<tavoos/widget/templates/templates.h>` to get all of them. Slot bodies position their own widgets - no base imposes alignment or layout on
what you give it.

| Base | Owns | You provide |
|---|---|---|
| `ButtonBase` | `pressed`, click and Space/Enter, `checkable`/`checked`/`exclusive`/`group` | `background`, `content` |
| `RangeBase` | `value`/`minValue`/`maxValue`, `position` (0-1); the base of the three below | `background`, `content` |
| `ProgressBarBase` | the range, nothing more | `background`, `content` bound to `positionState()` |
| `SliderBase` | the range, `pressed`, press/drag/tap handling | `background`, `content`, and a `handle` positioned with `xFraction(positionState())` |
| `SpinBoxBase` | the range, `step`, `commitText`, `valueText`, `increase`/`decrease` | `background`, `content`, `up`, `down` |
| `OverlayBase` | open/close, `closePolicy`, `modal`, input blocking, focus, sizing to its children | `background`, optionally `scrim`; children are declared inside it |
| `PopupBase` | everything in `OverlayBase`, plus `placement`, `target`, offsets, `x`/`y` | `background`, optionally `scrim`; children are declared inside it |
| `FlickAreaBase` | scrolling: offsets, content size, wheel, drag, keys, clipping | `background` (also the clip shape); children are declared inside it |
| `ScrollAreaBase` | everything in `FlickAreaBase`, plus bar policy, auto-hide, thumb drag, track click, thumb size and position states | `background`, `verticalTrack`/`verticalThumb`, `horizontalTrack`/`horizontalThumb`; children are declared inside it |
| `StackViewBase` | a stack of items, push/pop/replace, transitions, focus handover, `depth`/`currentItem`/`indexOf` | `background`; items are created with `push<T>`/`initialItem<T>` |
| `TextFieldBase` | all text editing, caret, scrolling, text properties | `background` only |

**A progress bar** - bind the fill to the position and layout does the sizing:

```cpp
class ThinBar : public Tavoos::ProgressBarBase {
public:
    ThinBar(Tavoos::Object* parent) : ProgressBarBase{parent} {
        width(300).height(4);
        background([](Tavoos::RectangleWidget& track) { track.color(Tavoos::Color::LightGray); });
        content<Tavoos::RectangleWidget>([this](Tavoos::RectangleWidget& fill) {
            fill.fill(Tavoos::Fill::Height)
                .alignment(Tavoos::Alignment::Left)
                .widthFraction(positionState())
                .color(Tavoos::Color::Blue);
        });
    }
};

TB::Create<ThinBar>([](ThinBar& b) { b.value(40); });
```

**A slider** - dragging and tap-to-set are in the base. The base maps pointer positions to values
using the track length `width - handleWidth`, so bind the handle with `xFraction` and inset the
track by half the handle's width so the ends line up:

```cpp
class DotSlider : public Tavoos::SliderBase {
public:
    DotSlider(Tavoos::Object* parent) : SliderBase{parent} {
        width(240).height(16);
        background([](Tavoos::RectangleWidget& track) {
            track.fill(Tavoos::Fill::Width).height(4)
                .marginLeft(8).marginRight(8)
                .alignment(Tavoos::Alignment::CenterVertical)
                .color(Tavoos::Color::LightGray);
        });
        handle<Tavoos::RectangleWidget>([this](Tavoos::RectangleWidget& dot) {
            dot.width(16).height(16).radius(8)
                .alignment(Tavoos::Alignment::CenterVertical)
                .xFraction(positionState())
                .color(Tavoos::Color::Orange);
        });
    }
};
```

**A toggle** - `checkable(true)` is all it takes to get the toggling; react to `checkedState()`:

```cpp
class StarToggle : public Tavoos::ButtonBase {
public:
    StarToggle(Tavoos::Object* parent) : ButtonBase{parent} {
        checkable(true);
        width(24).height(24);
        checkedState().onChange([this](const bool& on) {
            m_color.set(on ? Tavoos::Color::Yellow : Tavoos::Color::Gray);
        });
        content<Tavoos::RectangleWidget>([this](Tavoos::RectangleWidget& star) {
            star.fill(Tavoos::Fill::Both).radius(4).color(m_color);
        });
    }

private:
    Tavoos::State<Tavoos::Paint> m_color{Tavoos::Color::Gray};
};
```

**A text field** - you supply only the background. Everything about how the text looks is a
property of `TextFieldBase` (`textColor`, `placeholderColor`, `caretColor`, `font`,
`innerPadding`, ...), and `content()` isn't available on it:

```cpp
class SoftField : public Tavoos::TextFieldBase {
public:
    SoftField(Tavoos::Object* parent) : TextFieldBase{parent} {
        width(240).height(36);
        focusedState().onChange([this](const bool& focused) {
            m_fill.set(focused ? Tavoos::Color::rgba(235, 240, 255) : Tavoos::Color::rgba(245, 245, 245));
        });
        background([this](Tavoos::RectangleWidget& box) { box.radius(4).color(m_fill); });
    }

private:
    Tavoos::State<Tavoos::Paint> m_fill{Tavoos::Color::rgba(245, 245, 245)};
};
```

**A spin box** - the base steps and commits; your content shows `valueText` and reports typed
input through `commitText`, and the `up`/`down` items just need to be clickable widgets:

```cpp
class StepBox : public Tavoos::SpinBoxBase {
public:
    StepBox(Tavoos::Object* parent) : SpinBoxBase{parent} {
        width(120).height(32);
        background([](Tavoos::RectangleWidget& box) { box.radius(4).color(Tavoos::Color::rgba(245, 245, 245)); });
        content<Tavoos::TextFieldWidget>([this](Tavoos::TextFieldWidget& field) {
            field.fill(Tavoos::Fill::Both).marginRight(34)
                .backgroundColor(Tavoos::Color::Transparent)
                .disabledColor(Tavoos::Color::Transparent)
                .borderWidth(0.0f);
            field.onSubmit([this](const std::string& text) { commitText(text); });
            valueTextState().onChange([&field](const std::string& text) { field.text(text); });
            field.text(valueText());
        });
        up<Tavoos::ButtonWidget>([](Tavoos::ButtonWidget& b) {
            b.text("+").alignment(Tavoos::Alignment::Right | Tavoos::Alignment::Top).width(30).height(16);
        });
        down<Tavoos::ButtonWidget>([](Tavoos::ButtonWidget& b) {
            b.text("-").alignment(Tavoos::Alignment::Right | Tavoos::Alignment::Bottom).width(30).height(16);
        });
    }
};
```

**Values that depend on the final size.** `width()` and `height()` are only the properties you
set; they don't reflect `fill()`, fractions, or a layout. To derive something from the actual
size - a circular radius, say - override `onResolvedSizeChanged()` and read
`resolvedWidth()`/`resolvedHeight()`:

```cpp
class RoundBadge : public Tavoos::ButtonBase {
public:
    RoundBadge(Tavoos::Object* parent) : ButtonBase{parent} {
        background([this](Tavoos::RectangleWidget& disc) { disc.radius(m_radius).color(Tavoos::Color::Blue); });
    }

protected:
    void onResolvedSizeChanged() override {
        m_radius.set(static_cast<int>(std::min(resolvedWidth(), resolvedHeight()) / 2));
    }

private:
    Tavoos::State<int> m_radius{0};
};
```

The hook runs when the size is first known and whenever it changes, before the children are laid
out, so what it sets takes effect in the same pass.

## Components

A **component** is a piece of UI you define once and reuse anywhere, with its own properties: a
labeled form field, a dialog, a whole page. It is an ordinary widget class. You derive
`Component<Root>`, where `Root` is the kind of widget it *is* (`ColumnWidget`, `RectangleWidget`,
a control, ...), and fill its tree in `build()` with the same `TB::` calls a window uses. To add
new interaction behavior use a [custom control](#custom-controls); to compose widgets that already
exist, use a component.

```cpp
class LabeledField : public Tavoos::Component<Tavoos::ColumnWidget> {
public:
    LabeledField(Tavoos::Object* parent) : Component{parent} { spacing(4); }

    TAVOOS_PROPERTY(std::string, label, "")
    TAVOOS_PROPERTY(std::string, placeholder, "")
    TAVOOS_CALLBACK(onSubmit, void(const std::string&))

    const std::string& text() const { return m_field->text(); }

protected:
    void build() override {
        TB::Text([this](Tavoos::TextWidget& t) { t.text(m_label.state()); });
        TB::TextField([this](Tavoos::TextFieldWidget& f) {
            m_field = &f;
            f.fill(Tavoos::Fill::Width).placeholder(m_placeholder.state());
            f.onSubmit([this](const std::string& s) { if (m_onSubmit) m_onSubmit(s); });
        });
    }

private:
    Tavoos::TextFieldWidget* m_field{nullptr};
};
```

Use it like any widget, with its own properties plus those of its root type:

```cpp
TB::Create<LabeledField>([](LabeledField& f) {
    f.label("Email").placeholder("you@example.com").spacing(8);
});
```

### How it is built

1. The component is constructed.
2. `build()` runs once, with `TB::` calls attaching to the component.
3. The instance's body runs, so it can set properties and declare more children, which attach to
   the component after the ones `build()` created.

Because `build()` runs before the body, bind to a property's state (`m_label.state()`) rather
than reading its value; a value set in the body then flows through. Keep pointers to inner
widgets in members (`m_field`); they stay valid while the component exists.

### Properties and callbacks

| Macro | Generates |
|---|---|
| `TAVOOS_PROPERTY(Type, name, default)` | A bindable member `m_name`, a fluent setter `name(value)` that accepts a value or a `State`, a getter `name()`, and `nameState()` for binding. |
| `TAVOOS_CALLBACK(name, Signature)` | A fluent setter `name(fn)` and a protected `std::function` member `m_name`; call it with `if (m_name) m_name(...)`. |

`Type` must not contain a top-level comma, and both macros leave the class in a `public:` section.

### The root type

A component *is* its root, so the root's setters (`spacing`, `width`, `fill`, `color`, ...) work
and keep returning the component type. The root can be a control; this dialog is a popup:

```cpp
class ConfirmDialog : public Tavoos::Component<Tavoos::PopupWidget> {
public:
    ConfirmDialog(Tavoos::Object* parent) : Component{parent} {
        target(Tavoos::PlacementTarget::Window).placement(Tavoos::Placement::Center).modal(true);
    }

    TAVOOS_PROPERTY(std::string, message, "Are you sure?")
    TAVOOS_CALLBACK(onConfirm, void())

protected:
    void build() override {
        TB::Column([this](Tavoos::ColumnWidget& column) {
            column.spacing(12);
            TB::Text([this](Tavoos::TextWidget& t) { t.text(m_message.state()); });
            TB::Row([this](Tavoos::RowWidget& row) {
                row.spacing(8);
                TB::Button([this](Tavoos::ButtonWidget& b) {
                    b.text("Cancel").onClick([this](Tavoos::MouseEvent&) { close(); });
                });
                TB::Button([this](Tavoos::ButtonWidget& b) {
                    b.text("OK").onClick([this](Tavoos::MouseEvent&) {
                        if (m_onConfirm)
                            m_onConfirm();
                        close();
                    });
                });
            });
        });
    }
};
```

A `RectangleWidget` root has no size of its own, because a rectangle does not grow to fit its
children. For a box that hugs its content, use a `Control` root and put the content in its
`content` slot. Inside a slot body, `TB::` is not available for the slot's own children; use
`addChild<T>` there (see [Dynamic children](#dynamic-children)).

### Composing components

A component can use other components in its `build()`, and a page is just a component placed in
the window:

```cpp
class LoginPage : public Tavoos::Component<Tavoos::ColumnWidget> {
public:
    LoginPage(Tavoos::Object* parent) : Component{parent} { spacing(12).width(320); }

    TAVOOS_PROPERTY(std::string, heading, "Sign in")
    TAVOOS_CALLBACK(onLogin, void(const std::string&, const std::string&))

protected:
    void build() override {
        TB::Text([this](Tavoos::TextWidget& t) { t.text(m_heading.state()); });
        TB::Create<LabeledField>([this](LabeledField& f) {
            m_email = &f;
            f.label("Email").placeholder("you@example.com");
        });
        TB::Create<LabeledField>([this](LabeledField& f) {
            m_password = &f;
            f.label("Password");
        });
        TB::Button([this](Tavoos::ButtonWidget& b) {
            b.text("Sign in").onClick([this](Tavoos::MouseEvent&) {
                if (m_onLogin)
                    m_onLogin(m_email->text(), m_password->text());
            });
        });
    }

private:
    LabeledField* m_email{nullptr};
    LabeledField* m_password{nullptr};
};

class MainWindow : public Tavoos::Window {
public:
    void build() override {
        TB::Create<LoginPage>([](LoginPage& page) {
            page.heading("Welcome back").x(20).y(20).onLogin([](const std::string& email, const std::string& password) {
                (void)email;
                (void)password;
            });
        });
    }
};
```

A component can also keep its own state. This counter holds the count itself and reports changes:

```cpp
class Counter : public Tavoos::Component<Tavoos::RowWidget> {
public:
    Counter(Tavoos::Object* parent) : Component{parent} { spacing(8); }

    TAVOOS_PROPERTY(int, step, 1)
    TAVOOS_CALLBACK(onChange, void(int))

    int count() const { return m_count.get(); }

protected:
    void build() override {
        TB::Button([this](Tavoos::ButtonWidget& b) {
            b.text("-").onClick([this](Tavoos::MouseEvent&) { change(-m_step.get()); });
        });
        TB::Text([this](Tavoos::TextWidget& t) { t.text(m_label); });
        TB::Button([this](Tavoos::ButtonWidget& b) {
            b.text("+").onClick([this](Tavoos::MouseEvent&) { change(m_step.get()); });
        });
    }

private:
    void change(int delta) {
        m_count.set(m_count.get() + delta);
        m_label.set(std::to_string(m_count.get()));
        if (m_onChange)
            m_onChange(m_count.get());
    }

    Tavoos::State<int> m_count{0};
    Tavoos::State<std::string> m_label{"0"};
};
```

To create a component after the window is built, use `addChild<T>`, which builds it the same way:

```cpp
someContainer.addChild<Counter>([](Counter& c) { c.step(5); });
```

## Dragging

Any widget, not just a control, can opt into dragging:

```cpp
TB::Rectangle([](Tavoos::RectangleWidget& r) {
    r.draggable(true).dragXAxis(Tavoos::DragAxis{.enabled = true, .min = 0, .max = 300});
});
```

| Property | Type | Notes |
|---|---|---|
| `draggable()` | `bool` | When `true`, dragging moves the widget itself via `x()`/`y()`. |
| `dragThreshold()` | `float` | Pixels of movement before a drag starts (default `0`). |
| `dragXAxis()`, `dragYAxis()` | `DragAxis{enabled, min, max}` | Locks and/or clamps movement along that axis. |

To build something that *computes* its position from a drag instead of free-translating (a
custom scrubber, say), hook the callbacks without setting `draggable(true)` - they
still fire either way:

```cpp
int startValue = 0;
thumb.onDragStart([&](Tavoos::DragEvent&) { startValue = currentValue; });
thumb.onDragMove([&](Tavoos::DragEvent& e) {
    setValue(startValue + static_cast<int>(e.totalDx()) / pixelsPerUnit);
});
```

`DragEvent::dx()`/`dy()` are the delta since the last move; `totalDx()`/`totalDy()` are the
cumulative delta since the drag started - prefer `total*` for computing an absolute value (no
incremental rounding drift), `dx`/`dy` for a pure follow-the-cursor translate.

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

**Transitions.** `Tavoos::Transition` and the presets (`FadeIn`, `SlideOut`, ...) animate a widget
through `prepare`, `update` and `finish`, and a `TransitionRunner` runs several of them on one clock.
The stack view uses them for its push, pop and replace animations (see
[`StackViewWidget`](#stackviewwidget)), popups use them for opening and closing (see
[`PopupWidget`](#popupwidget)), and you can write your own by deriving `Transition`.

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

A third scheme, `data:`, embeds content directly instead of referencing a file - mainly for a
small inline SVG icon:

```cpp
constexpr const char* kStarSvg = R"svg(<svg ...>...</svg>)svg";
icon.source(std::string("data:") + kStarSvg);
```

Same idea works for `Application::registerFont(name, "data:" + fontBytes)` if you have font
data in memory rather than as a file.

## Keyboard & focus

Set `.focusable(true)` on any widget to make it focusable - via Tab/Shift+Tab (cycling
forward/backward through all focusable, visible widgets in the tree, depth-first order,
wrapping at the ends) or by clicking it, or any of its descendants (the nearest focusable
ancestor of whatever was actually clicked receives focus). Buttons, checkboxes, radios,
switches, sliders, and text fields (see [Controls](#controls)) are `focusable(true)` by default. The currently-focused widget
receives:

- `onKeyPress` / `onKeyRelease` - `KeyEvent::keyCode()` compares against the `Key` enum (`Tavoos::Key::Enter`, `Tavoos::Key::A`, etc., matching GLFW key codes) and `.modifiers()` (`KeyModifier::Shift/Control/Alt/Super`, combine with `|`).
- `onTextInput` - fires per Unicode codepoint typed (for building text-entry widgets).
- `onFocusIn` / `onFocusOut` - fired when focus moves onto/off of this widget, including on window activation/deactivation if it was already focused.
- **`focused()`**, **`focusedState()`** - reactive read of whether this widget currently has
  focus, for anything beyond just handling `onFocusIn`/`onFocusOut` directly (e.g. driving a
  border-color `AnimatedState`).

Call `.focus()` on a widget to focus it programmatically instead of waiting for Tab.

Popups manage focus for you: opening one moves focus to its first focusable child, and closing it
gives focus back to whatever had it, unless you clicked somewhere else in the meantime. While a
modal popup is open, Tab cycles only through its own controls.

A flick area scrolls with the arrow keys, Page Up/Down, and Home/End. It does not need to be
focused: a key that the focused widget inside it ignores (a button, for example) bubbles up and
scrolls it. A scroll area has this off by default; enable it with `keyNavigation(true)`.

A stack view moves focus into a pushed item and gives it back when the item is popped, and its hidden
items are never Tab targets.
