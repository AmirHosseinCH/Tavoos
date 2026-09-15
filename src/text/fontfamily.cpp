#include <tavoos/text/fontfamily.h>

#include <climits>
#include <cmath>

#include <spdlog/spdlog.h>

namespace Tavoos {

void FontFamily::addFace(FT_Library ftLibrary, const std::string& sourcePath, FontWeight weight, FontStyle style) {
    auto face = std::make_unique<FontFace>(ftLibrary, sourcePath, weight, style);
    if (!face->isValid()) {
        spdlog::error("skipping invalid face for family '{}': '{}'", m_name, sourcePath);
        return;
    }
    m_faces.push_back(std::move(face));
}

FontFace* FontFamily::resolveFace(FontWeight weight, FontStyle style) {
    if (m_faces.empty()) {
        spdlog::error("font family '{}' has no loaded faces", m_name);
        return nullptr;
    }

    FontFace* best = nullptr;
    int bestDelta = INT_MAX;

    for (auto& face : m_faces) {
        if (face->style() != style)
            continue;
        const int delta = std::abs(static_cast<int>(face->weight()) - static_cast<int>(weight));
        if (delta < bestDelta) { bestDelta = delta; best = face.get(); }
    }
    if (best)
        return best;

    spdlog::warn("family '{}' has no faces with requested style; falling back across styles", m_name);
    bestDelta = INT_MAX;
    for (auto& face : m_faces) {
        const int delta = std::abs(static_cast<int>(face->weight()) - static_cast<int>(weight));
        if (delta < bestDelta) { bestDelta = delta; best = face.get(); }
    }
    return best;
}

}