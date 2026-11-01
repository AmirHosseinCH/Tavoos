#pragma once

#include <tavoos/export.hpp>

#include <glm/glm.hpp>

#include <optional>

namespace Tavoos {

struct Bounds {
    float left;
    float top;
    float right;
    float bottom;
};

TAVOOS_EXPORT std::optional<Bounds> visibleBounds(const glm::mat4& localToClip);

}
