#pragma once

#include "karting/core.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace karting::bridge {

// Dimensions are intentionally kept in the same physical scales as Track:
// road coordinates are world units, vertical barrier sizes are metres.
inline constexpr double kFillMinRise = 3.0;
inline constexpr double kFillMaxRise = 245.0;
inline constexpr double kDeckMinRise = 145.0;
inline constexpr double kDeckHalfWidth = Track::wall + 7.0;
inline constexpr double kBarrierInner = Track::wall - 19.0;
inline constexpr double kBarrierOuter = Track::wall + 1.0;
inline constexpr float kBarrierFootHeight = 0.17f;
inline constexpr float kBarrierRailBottom = 0.27f;
inline constexpr float kBarrierTotalHeight = 0.48f;

struct SectionVertex {
    Vec2 position;
    double altitude;
};

// Closed seven-vertex cross section: two grass/stone shoulders, buried
// earth sides and a central bottom below the terrain. Extruding the
// whole ring closes the embankment instead of leaving green sheet surfaces.
using EmbankmentSection = std::array<SectionVertex, 7>;

inline EmbankmentSection embankmentSection(const Track& track,
                                          std::size_t index) {
    const double rise=std::max(0.0,track.bridgeRise(index));
    const double shoulder= kDeckHalfWidth + 35.0 + .35*rise;
    const double toe= kDeckHalfWidth + 75.0 + std::min(1.6*rise,430.0);

    const auto vertex=[&](double offset,double addedHeight) {
        const Vec2 xy=track.point(index,offset);
        return SectionVertex{xy,elevation(xy)+addedHeight};
    };
    return {{
        vertex(-toe,-23.0),
        vertex(-shoulder,.59*rise-9.0),
        vertex(-kDeckHalfWidth,rise-5.0),
        vertex(kDeckHalfWidth,rise-5.0),
        vertex(shoulder,.59*rise-9.0),
        vertex(toe,-23.0),
        vertex(0.0,-33.0)
    }};
}

inline bool isFilledEmbankment(double firstRise,double secondRise) {
    const double maximum=std::max(firstRise,secondRise);
    return maximum>kFillMinRise && maximum<kFillMaxRise;
}

} // namespace karting::bridge
