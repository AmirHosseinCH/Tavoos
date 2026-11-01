#include <tavoos/gfx/visibility.h>

#include <algorithm>
#include <limits>

namespace Tavoos {

std::optional<Bounds> visibleBounds(const glm::mat4& localToClip) {
    const glm::mat4 clipToLocal = glm::inverse(localToClip);
    Bounds bounds{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                  std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for (const glm::vec2 corner : {glm::vec2{-1.0f, -1.0f}, glm::vec2{1.0f, -1.0f}, glm::vec2{1.0f, 1.0f}, glm::vec2{-1.0f, 1.0f}}) {
        const glm::vec4 point = clipToLocal * glm::vec4{corner, 0.0f, 1.0f};
        if (!(point.w > 1e-6f))
            return std::nullopt;
        const float x = point.x / point.w;
        const float y = point.y / point.w;
        bounds.left = std::min(bounds.left, x);
        bounds.top = std::min(bounds.top, y);
        bounds.right = std::max(bounds.right, x);
        bounds.bottom = std::max(bounds.bottom, y);
    }
    return bounds;
}

}
