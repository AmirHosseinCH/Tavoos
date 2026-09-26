#pragma once

#include <tavoos/application.h>
#include <tavoos/export.hpp>
#include <tavoos/object.h>
#include <tavoos/widget/buttonwidget.h>
#include <tavoos/widget/checkboxwidget.h>
#include <tavoos/widget/radiowidget.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <functional>

namespace Tavoos {

template<typename T>
concept WindowDerived = std::derived_from<T, Window>;

class TAVOOS_EXPORT Builder {
    friend class Window;

public:
    Builder() = delete;

    template<WindowDerived T>
    static void createWindowOf(std::function<void(Window&)> body) {
        std::unique_ptr<Window> window = std::make_unique<T>();

        body(*window);

        window->setup();

        Application::instance()->registerWindow(window);
    }

    static void Rectangle(std::function<void(RectangleWidget&)> body) { create<RectangleWidget>(std::move(body)); }
    static void Row(std::function<void(RowWidget&)> body)             { create<RowWidget>(std::move(body)); }
    static void Column(std::function<void(ColumnWidget&)> body)       { create<ColumnWidget>(std::move(body)); }
    static void Grid(std::function<void(GridWidget&)> body)           { create<GridWidget>(std::move(body)); }
    static void Flex(std::function<void(FlexWidget&)> body)           { create<FlexWidget>(std::move(body)); }
    static void Image(std::function<void(ImageWidget&)> body)         { create<ImageWidget>(std::move(body)); }
    static void SVG(std::function<void(SVGWidget&)> body)             { create<SVGWidget>(std::move(body)); }
    static void Text(std::function<void(TextWidget&)> body)           { create<TextWidget>(std::move(body)); }
    static void Button(std::function<void(ButtonWidget&)> body)       { create<ButtonWidget>(std::move(body)); }
    static void Checkbox(std::function<void(CheckboxWidget&)> body)   { create<CheckboxWidget>(std::move(body)); }
    static void Radio(std::function<void(RadioWidget&)> body)         { create<RadioWidget>(std::move(body)); }

    template<typename T>
        requires std::derived_from<T, Widget>
    static void Create(std::function<void(T&)> body) { create<T>(std::move(body)); }

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

}