#pragma once

namespace Tavoos {

struct GlyphMetrics {
    bool valid{false};
    float advance{0.0f};
    float bearingX{0.0f};
    float bearingY{0.0f};
    float width{0.0f};
    float height{0.0f};
    float u0{0.0f}, v0{0.0f};
    float u1{0.0f}, v1{0.0f};
    int atlasPage{-1};
};

}