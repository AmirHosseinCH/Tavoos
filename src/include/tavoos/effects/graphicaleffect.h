#pragma once

#include <tavoos/export.hpp>

namespace Tavoos {

class Renderer;
class Widget;

class TAVOOS_EXPORT GraphicalEffect {
    friend class Renderer;

public:
    virtual ~GraphicalEffect();

    GraphicalEffect(const GraphicalEffect&) = delete;
    GraphicalEffect& operator=(const GraphicalEffect&) = delete;

    Widget& owner() const noexcept { return *m_owner; }

protected:
    explicit GraphicalEffect(Widget& owner);

    virtual void renderBehind(Renderer&, Widget&) {}

    void invalidate();

    template<typename... Props>
    void bindInvalidateTriggers(Props&... props) {
        auto repaint = [this](const auto&) { invalidate(); };
        (props.onChange(repaint), ...);
    }

private:
    Widget* m_owner;
};

}
