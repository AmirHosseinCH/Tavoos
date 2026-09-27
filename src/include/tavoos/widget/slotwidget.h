#pragma once

#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/widget/rectangle.h>
#include <tavoos/widget/widget.h>

#include <functional>
#include <type_traits>

namespace Tavoos {

class TAVOOS_EXPORT SlotWidget : public Widget {
public:
    SlotWidget(Object* parent);

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

    unsigned contentRevision() const noexcept { return m_contentRevision; }

protected:
    void render(Renderer& renderer) override;
    Size computeIntrinsicSize() override;

    virtual void onSlotReplaced();

private:
    template<typename W>
    void installBackground(std::function<void(W&)> body) {
        if (m_background)
            replaceSlot(m_background);
        m_background = addChild<W>([&body](W& slot) {
            slot.z(-1).fill(Fill::Both);
            if (body)
                body(slot);
        });
    }

    template<typename W>
    void installContent(std::function<void(W&)> body) {
        if (m_content)
            replaceSlot(m_content);
        m_content = addChild<W>([&body](W& slot) {
            slot.alignment(Alignment::Center);
            if (body)
                body(slot);
        });
        ++m_contentRevision;
        requestRelayout();
    }

    void replaceSlot(Widget*& slot);

    Widget* m_background{nullptr};
    Widget* m_content{nullptr};
    unsigned m_contentRevision{0};
};

}
