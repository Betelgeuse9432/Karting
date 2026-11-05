#include "karting/game.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace karting {

Vector3 worldPoint(Vec2 p, float heightOffset) {
    return {float(p.x*kWorldScale),
            float((elevation(p)+heightOffset)*kWorldScale),
            float(p.y*kWorldScale)};
}

Color palette(int index) {
    static constexpr std::array<Color,6> colors{{
        {220,30,30,255},{40,90,220,255},{240,200,30,255},
        {240,240,245,255},{40,170,90,255},{235,120,20,255}
    }};
    return colors[std::size_t(std::clamp(index,0,5))];
}

static Color shaded(Color c, float multiplier) {
    return {static_cast<unsigned char>(std::clamp(c.r*multiplier,0.0f,255.0f)),
            static_cast<unsigned char>(std::clamp(c.g*multiplier,0.0f,255.0f)),
            static_cast<unsigned char>(std::clamp(c.b*multiplier,0.0f,255.0f)),255};
}

static void quad(Vector3 a,Vector3 b,Vector3 c,Vector3 d,Color color) {
    DrawTriangle3D(a,b,c,color);
    DrawTriangle3D(a,c,d,color);
    // Both faces visible even around elevation changes and sharp turns.
    DrawTriangle3D(c,b,a,color);
    DrawTriangle3D(d,c,a,color);
}

void WorldRenderer::draw(const Track& track,const Weather& weather,int quality,
                         Vector3 cameraPosition,double time) const {
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    const auto asphalt=shaded({84,86,95,255},1-0.35f*rain-0.52f*night);
    const auto kerbRed=shaded({223,37,37,255},1-0.25f*night);
    const auto kerbWhite=shaded({230,228,218,255},1-0.4f*night);
    const auto grass=shaded({53,153,57,255},1-0.1f*rain-0.55f*night);
    // A distant lower base hides the far edge of the camera-local terrain.
    DrawPlane({float(Track::worldWidth*kWorldScale*.5f),-4.f,
               float(Track::worldHeight*kWorldScale*.5f)},
              {float(Track::worldWidth*kWorldScale),
               float(Track::worldHeight*kWorldScale)},
              shaded(grass,.68f));
    // Near and mid-distance ground follows the same relief as the asphalt.
    drawTerrain(weather,quality,cameraPosition);

    const auto count=track.size();
    const float farDistance=quality==0?47.f:quality==1?63.f:81.f;
    const auto& center=track.center();
    for (std::size_t i=0;i<count;) {
        const Vector3 mid=worldPoint(center[i]);
        const float dx=mid.x-cameraPosition.x, dz=mid.z-cameraPosition.z;
        const float d2=dx*dx+dz*dz;
        if (d2>(farDistance+12)*(farDistance+12)) { ++i; continue; }
        const std::size_t step=(d2<28*28?1:quality==0?5:quality==1?3:2);
        const auto j=(i+step)%count;
        const auto l=[&](std::size_t k,double off) {
            return worldPoint(track.point(k,off),1);
        };
        const auto paint=[&](double left,double right,Color color) {
            quad(l(i,left),l(j,left),l(j,right),l(i,right),color);
        };
        paint(-Track::halfWidth,Track::halfWidth,
              shaded(asphalt,int(i/9)%2==0?1.f:.90f));
        const auto kerb=int(i/5)%2?kerbWhite:kerbRed;
        paint(-Track::halfWidth-Track::kerb,-Track::halfWidth,kerb);
        paint(Track::halfWidth,Track::halfWidth+Track::kerb,kerb);
        if (i%18<9) paint(-2,2,shaded(WHITE,1-0.55f*night));

        if (track.isPitIndex(i) && track.isPitIndex(j)) {
            paint(Track::halfWidth+Track::kerb+5,
                  Track::halfWidth+Track::kerb+Track::pitWidth-5,
                  shaded({110,111,119,255},1-.3f*rain-.55f*night));
            paint(Track::halfWidth+Track::kerb+5,
                  Track::halfWidth+Track::kerb+7,YELLOW);
        }
        if (quality>0 && d2<38*38) {
            for (int side:{-1,1}) {
                if (side>0 && track.isPitIndex(i)) continue;
                const auto a=worldPoint(track.point(i,side*Track::wall),18);
                const auto b=worldPoint(track.point(j,side*Track::wall),18);
                const auto a2=worldPoint(track.point(i,side*Track::wall),25);
                const auto b2=worldPoint(track.point(j,side*Track::wall),25);
                DrawLine3D(a,b,shaded(LIGHTGRAY,1-.5f*night));
                DrawLine3D(a2,b2,shaded(GRAY,1-.5f*night));
            }
        }
        i+=step;
    }

    // Close roadside reflectors give passing-speed cues without fake blur.
    // One pair about every 25-30 metres, culled before any draw calls.
    if (quality>0) {
        for (std::size_t i=0;i<count;i+=42) {
            const Vector3 centerPoint=worldPoint(center[i]);
            const float dx=centerPoint.x-cameraPosition.x;
            const float dz=centerPoint.z-cameraPosition.z;
            if (dx*dx+dz*dz>65.f*65.f) continue;
            for (int side:{-1,1}) {
                if (side>0 && track.isPitIndex(i)) continue;
                const auto base=worldPoint(
                    track.point(i,side*(Track::halfWidth+Track::kerb+34)),0);
                const Color pillar=shaded({228,231,224,255},1.f-.45f*night);
                DrawCylinderEx(base,{base.x,base.y+.78f,base.z},
                               .045f,.043f,6,pillar);
                DrawCubeV({base.x,base.y+.57f,base.z},
                          {.12f,.14f,.12f},
                          side>0?shaded({225,62,48,255},1.f-.35f*night):
                                 shaded({242,204,72,255},1.f-.35f*night));
            }
        }
    }

    // Start/finish gantry is part of the track infrastructure.
    if (quality>0) {
        const auto a=worldPoint(track.point(0,-Track::halfWidth-25));
        const auto b=worldPoint(track.point(0,Track::halfWidth+25));
        if (Vector3Distance(a,cameraPosition)<65.f) {
            DrawCylinderEx(a,{a.x,a.y+3.8f,a.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx(b,{b.x,b.y+3.8f,b.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx({a.x,a.y+3.8f,a.z},
                           {b.x,b.y+3.8f,b.z},.23f,.23f,6,
                           shaded(WHITE,1-.5f*night));
        }
    }

    // Static scenery is generated once and culled before drawing.
    drawScenery(weather,quality,cameraPosition,time);
}

} // namespace karting
