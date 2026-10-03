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
6. [Theme & styling](#theme--styling)
7. [Custom controls](#custom-controls)
8. [Dragging](#dragging)
9. [Animation](#animation)
10. [Resource embedding](#resource-embedding)
11. [Keyboard & focus](#keyboard--focus)

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

## Theme & styling

Every control above (`ButtonStyle`, `CheckboxStyle`, ..., `SpinBoxStyle` - one plain struct
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
