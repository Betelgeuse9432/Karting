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

// Closed nine-vertex cross section: turf, exposed compacted soil,
// buried stone toes and a central bottom below the terrain. Sweeping
// the ENTIRE ring closes the embankment instead of using green sheets.
using EmbankmentSection = std::array<SectionVertex, 9>;

inline EmbankmentSection embankmentSection(const Track& track,
                                          std::size_t index) {
    const double rise=std::max(0.0,track.bridgeRise(index));
    const double upper= kDeckHalfWidth + 28.0 + .24*rise;
    const double lower= kDeckHalfWidth + 64.0 + .92*rise;
    const double toe= kDeckHalfWidth + 94.0 + std::min(1.85*rise,480.0);

    const auto vertex=[&](double offset,double addedHeight) {
        // Deterministic, low-frequency irregularity along the track keeps
        // the two banks from looking like perfectly planar green ramps.
        const double jitter=6.0*std::sin(index*.11+offset*.0018) +
                            3.0*std::sin(index*.047-offset*.003);
        const Vec2 xy=track.point(index,offset);
        return SectionVertex{xy,elevation(xy)+addedHeight+jitter};
    };
    // The roadway crown must remain geometrically accurate: no jitter
    // where the earth touches the slab's underside.
    const auto roadEdge=[&](double offset) {
        const Vec2 xy=track.point(index,offset);
        return SectionVertex{xy,elevation(xy)+rise-5.0};
    };
    return {{
        vertex(-toe,-23.0),
        vertex(-lower,.36*rise-17.0),
        vertex(-upper,.78*rise-9.0),
        roadEdge(-kDeckHalfWidth),
        roadEdge(kDeckHalfWidth),
        vertex(upper,.78*rise-9.0),
        vertex(lower,.36*rise-17.0),
        vertex(toe,-23.0),
        vertex(0.0,-40.0)
    }};
}

inline bool isFilledEmbankment(double firstRise,double secondRise) {
    const double maximum=std::max(firstRise,secondRise);
    return maximum>kFillMinRise && maximum<kFillMaxRise;
}

} // namespace karting::bridge
