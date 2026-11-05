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
    DrawPlane({float(Track::worldWidth*kWorldScale*.5),-3.4f,
               float(Track::worldHeight*kWorldScale*.5)},
              {float(Track::worldWidth*kWorldScale),
               float(Track::worldHeight*kWorldScale)},grass);

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

    // The start gantry, pit garages and ads are inexpensive 3D primitives.
    if (quality>0) {
        const auto p=track.point(0,-Track::halfWidth-25);
        const auto q=track.point(0,Track::halfWidth+25);
        const auto a=worldPoint(p), b=worldPoint(q);
        if (Vector3Distance(a,cameraPosition)<50) {
            DrawCylinderEx(a,{a.x,a.y+3.8f,a.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx(b,{b.x,b.y+3.8f,b.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx({a.x,a.y+3.8f,a.z},{b.x,b.y+3.8f,b.z},
                           .23f,.23f,6,shaded(WHITE,1-.5f*night));
        }
        for (int i=0;i<26;i+=3) {
            const std::size_t segment=(count-1-std::size_t(i*5))%count;
            if (!track.isPitIndex(segment)) continue;
            const auto g=worldPoint(track.point(segment,Track::halfWidth+Track::pitWidth+85));
            if (Vector3Distance(g,cameraPosition)>52) continue;
            DrawCubeV({g.x,g.y+0.58f,g.z},{2.4f,1.2f,2.0f},
                      shaded({195,200,212,255},1-.5f*night));
            DrawCubeV({g.x,g.y+1.23f,g.z},{2.65f,.15f,2.2f},
                      shaded({70,73,87,255},1-.5f*night));
            DrawCubeV({g.x,g.y+.55f,g.z-1.02f},{1.5f,.8f,.07f},
                      palette((i/3)%6));
        }
    }

    // Grandstands reproduce the V6 crowd zones, without sprite processing.
    if (quality>0) {
        for (std::size_t i=95;i<330 && i<count;i+=20) {
            for (int row=0;row<4;++row) {
                const auto p=track.point(i,-Track::wall-85-row*36);
                const auto ground=worldPoint(p);
                if (Vector3Distance(ground,cameraPosition)>53) continue;
                const float high=.3f+row*.28f;
                DrawCubeV({ground.x,ground.y+high,ground.z},
                          {1.35f,.16f,1.25f},
                          shaded({120,125,134,255},1-.45f*night));
                for (int seat=0;seat<3;++seat) {
                    const Color crowdColor=palette((int(i/20)+row+seat)%6);
                    DrawCubeV({ground.x+seat*.3f-.4f,
                               ground.y+high+.22f,ground.z},
                              {.17f,.26f,.17f},
                              shaded(crowdColor,1-.4f*night));
                }
            }
        }
        // Advertising panels on selected outside barriers.
        for (std::size_t i=115;i<count;i+=145) {
            const auto p=track.point(i,(i/145)%2?Track::wall+35:-Track::wall-35);
            const auto ground=worldPoint(p);
            if (Vector3Distance(ground,cameraPosition)>59) continue;
            const Color ad=palette(int(i/145)%6);
            DrawCubeV({ground.x,ground.y+1.25f,ground.z},
                      {2.8f,1.0f,.16f},shaded(ad,1-.42f*night));
            DrawCubeV({ground.x,ground.y+1.25f,ground.z+.10f},
                      {2.0f,.17f,.09f},shaded(WHITE,1-.42f*night));
        }
        // Pit-lane lighting: lamps become brighter at night.
        for (std::size_t i=0;i<count;i+=90) {
            if (!track.isPitIndex(i)) continue;
            const auto base=worldPoint(track.point(i,Track::wall+40));
            if (Vector3Distance(base,cameraPosition)>48) continue;
            const Vector3 tip{base.x,base.y+4.0f,base.z};
            DrawCylinderEx(base,tip,.035f,.025f,6,
                           shaded(LIGHTGRAY,1-.45f*night));
            DrawSphere(tip,.16f,night>.3?Color{255,235,150,255}:LIGHTGRAY);
        }
    }

    // Static scenery is generated once and culled before drawing.
    drawScenery(weather,quality,cameraPosition,time);
}

} // namespace karting
