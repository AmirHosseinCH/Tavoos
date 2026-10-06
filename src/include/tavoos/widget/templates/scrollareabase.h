#pragma once

#include <tavoos/animation/animationmanager.h>
#include <tavoos/export.hpp>
#include <tavoos/reactive/reactive.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/flickareabase.h>

#include <functional>
#include <type_traits>

namespace Tavoos {

enum class BarPolicy {
    Auto,
    Always,
    Never,
};

class TAVOOS_EXPORT ScrollAreaBase : public FlickAreaBase, private AnimatableBase {
public:
    ScrollAreaBase(Object* parent);
    ~ScrollAreaBase() override;

    template<typename W = RectangleWidget>
    decltype(auto) verticalTrack(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installBar<W>(self.m_verticalTrack, 10, self.m_verticalShown, self.m_verticalOpacity, std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) verticalThumb(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installBar<W>(self.m_verticalThumb, 11, self.m_verticalShown, self.m_verticalOpacity, std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) horizontalTrack(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installBar<W>(self.m_horizontalTrack, 10, self.m_horizontalShown, self.m_horizontalOpacity, std::move(body));
        return std::forward<decltype(self)>(self);
    }

    template<typename W = RectangleWidget>
    decltype(auto) horizontalThumb(this auto&& self, std::type_identity_t<std::function<void(W&)>> body) {
        self.template installBar<W>(self.m_horizontalThumb, 11, self.m_horizontalShown, self.m_horizontalOpacity, std::move(body));
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) verticalBarPolicy(this auto&& self, PropertyArg<BarPolicy> policy) {
        policy.applyTo(self.m_verticalPolicy);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) horizontalBarPolicy(this auto&& self, PropertyArg<BarPolicy> policy) {
        policy.applyTo(self.m_horizontalPolicy);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) barPolicy(this auto&& self, PropertyArg<BarPolicy> policy) {
        policy.applyTo(self.m_verticalPolicy);
        policy.applyTo(self.m_horizontalPolicy);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) autoHide(this auto&& self, PropertyArg<bool> value) {
        value.applyTo(self.m_autoHide);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) barHideDelay(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_hideDelay);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) barFadeDuration(this auto&& self, PropertyArg<float> seconds) {
        seconds.applyTo(self.m_fadeDuration);
        return std::forward<decltype(self)>(self);
    }

    decltype(auto) minThumbSize(this auto&& self, PropertyArg<float> size) {
        size.applyTo(self.m_minThumbSize);
        return std::forward<decltype(self)>(self);
    }

    BarPolicy verticalBarPolicy() const { return m_verticalPolicy; }
    BarPolicy horizontalBarPolicy() const { return m_horizontalPolicy; }
    bool autoHide() const { return m_autoHide; }
    float barHideDelay() const { return m_hideDelay; }
    float barFadeDuration() const { return m_fadeDuration; }
    float minThumbSize() const { return m_minThumbSize; }

    State<float>& verticalThumbSizeState() { return m_verticalThumbSize; }
    State<float>& verticalThumbPositionState() { return m_verticalThumbPosition; }
    State<float>& horizontalThumbSizeState() { return m_horizontalThumbSize; }
    State<float>& horizontalThumbPositionState() { return m_horizontalThumbPosition; }
    State<bool>& verticalBarShownState() { return m_verticalShown; }
    State<bool>& horizontalBarShownState() { return m_horizontalShown; }
    State<bool>& verticalThumbHoveredState() { return m_verticalThumbHovered; }
    State<bool>& verticalThumbPressedState() { return m_verticalThumbPressed; }
    State<bool>& horizontalThumbHoveredState() { return m_horizontalThumbHovered; }
    State<bool>& horizontalThumbPressedState() { return m_horizontalThumbPressed; }

protected:
    bool isContentChild(const Widget& child) const override;
    void onOffsetsSynced(bool offsetsChanged) override;
    bool hasHandlerFor(EventType type) override;
    void triggerPress(MouseEvent& event) override;
    void triggerRelease(MouseEvent& event) override;
    void triggerMouseMove(MouseEvent& event) override;
    void triggerMouseLeave(MouseEvent& event) override;
    void triggerDragMove(DragEvent& event) override;
    void triggerDragEnd(DragEvent& event) override;

private:
    enum class BarHit { None, VerticalThumb, VerticalTrack, HorizontalThumb, HorizontalTrack };
    enum class Axis { None, Vertical, Horizontal };

    template<typename W>
    void installBar(Widget*& slot, int z, State<bool>& shown, State<float>& opacity, std::function<void(W&)> body) {
        if (slot) {
            removeChild(slot);
            slot = nullptr;
        }
        slot = addChild<W>([z, &shown, &opacity, &body](W& bar) {
            bar.z(z).visible(shown).opacity(opacity);
            if (body)
                body(bar);
        });
    }

    bool tick(float dt) override;
    void syncBars();
    void wake();
    bool fades(BarPolicy policy) const;
    BarHit hitBar(const Point& point) const;
    void updateShown();
    void setPressed(Axis axis, bool pressed);

    Widget* m_verticalTrack{nullptr};
    Widget* m_verticalThumb{nullptr};
    Widget* m_horizontalTrack{nullptr};
    Widget* m_horizontalThumb{nullptr};

    Property<BarPolicy> m_verticalPolicy{BarPolicy::Auto};
    Property<BarPolicy> m_horizontalPolicy{BarPolicy::Auto};
    Property<bool> m_autoHide{true};
    Property<float> m_hideDelay{1.0f};
    Property<float> m_fadeDuration{0.2f};
    Property<float> m_minThumbSize{24.0f};

    State<float> m_verticalThumbSize{1.0f};
    State<float> m_verticalThumbPosition{0.0f};
    State<float> m_horizontalThumbSize{1.0f};
    State<float> m_horizontalThumbPosition{0.0f};
    State<float> m_verticalOpacity{0.0f};
    State<float> m_horizontalOpacity{0.0f};
    State<bool> m_verticalShown{false};
    State<bool> m_horizontalShown{false};
    State<bool> m_verticalThumbHovered{false};
    State<bool> m_verticalThumbPressed{false};
    State<bool> m_horizontalThumbHovered{false};
    State<bool> m_horizontalThumbPressed{false};

    bool m_verticalWanted{false};
    bool m_horizontalWanted{false};
    Axis m_barDrag{Axis::None};
    float m_idle{0.0f};
    bool m_animating{false};
};

}
