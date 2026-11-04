#include "karting/game.hpp"
#include "rlgl.h"
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

void WorldRenderer::initialize(const Track& track) {
    trees_.clear();
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> noise(-310,310);
    for (std::size_t i=0;i<track.size();i+=13) {
        for (int side:{-1,1}) {
            const auto p=track.point(i,side*(Track::wall+170)+noise(rng));
            if (p.x>100 && p.y>100 && p.x<9500 && p.y<8200 &&
                track.info(p).distance>Track::wall+35)
                trees_.push_back(p);
        }
    }
    std::uniform_real_distribution<double> x(0,9800), y(0,8700);
    for (int i=0;i<180;++i) {
        const Vec2 p{x(rng),y(rng)};
        if (track.info(p).distance>Track::wall+140) trees_.push_back(p);
    }
}

void WorldRenderer::draw(const Track& track,const Weather& weather,int quality,
                         Vector3 cameraPosition,double time) const {
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    const auto asphalt=shaded({84,86,95,255},1-0.35f*rain-0.52f*night);
    const auto kerbRed=shaded({223,37,37,255},1-0.25f*night);
    const auto kerbWhite=shaded({230,228,218,255},1-0.4f*night);
    const auto grass=shaded({53,153,57,255},1-0.1f*rain-0.55f*night);
    DrawPlane({88.2f,-2.6f,78.3f},{190,174},grass);

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

    // Tree positions were generated once, instead of walking the circuit
    // and calling nearest-point search on every rendered frame.
    for (const auto& p:trees_) {
        const auto origin=worldPoint(p);
        const float dx=origin.x-cameraPosition.x,dz=origin.z-cameraPosition.z;
        const float distance2=dx*dx+dz*dz;
        const float treeFar=quality==0?28.f:quality==1?39.f:50.f;
        if (distance2>treeFar*treeFar) continue;
        const float size=0.80f+float((int(p.x+p.y)%9))/15.f;
        const auto foliage=shaded({42,127,51,255},1-0.60f*night);
        DrawCylinderEx(origin,{origin.x,origin.y+size*.95f,origin.z},
                       .08f*size,.06f*size,5,
                       shaded({98,67,44,255},1-.5f*night));
        if (quality==0 || distance2>25*25) {
            DrawSphere({origin.x,origin.y+size*1.35f,origin.z},
                       .48f*size,foliage);
        } else {
            DrawSphere({origin.x,origin.y+size*1.25f,origin.z},
                       .5f*size,foliage);
            DrawSphere({origin.x-.21f*size,origin.y+size*1.65f,origin.z},
                       .35f*size,shaded(foliage,1.1f));
        }
    }
}

static void block(Vector3 pos,Vector3 size,Color color) {
    DrawCubeV(pos,size,color);
}

void VehicleRenderer::draw(const Car& car,const VehicleProfile& profile,
                           int index,int colorIndex,bool night) const {
    const auto p=worldPoint(car.position,0);
    const Color body=palette(colorIndex);
    const Color dark=shaded(body,.68f);
    const Color windows={44,90,125,255};
    const float s=kScale;
    rlPushMatrix();
    rlTranslatef(p.x,p.y,p.z);
    rlRotatef(float(-car.heading*RAD2DEG),0,1,0);

    // Four different low-poly silhouettes, using the original V6 dimensions.
    switch (index) {
    case 0: // Kart: open body, exposed driver and a small front bumper.
        block({0,.16f,0},{1.0f,.17f,.61f},body);
        block({-.10f,.37f,0},{.36f,.37f,.35f},dark);
        DrawSphere({-.05f,.70f,0},.21f,YELLOW);
        block({.55f,.11f,0},{.18f,.11f,.68f},dark);
        block({-.55f,.1f,0},{.18f,.09f,.65f},dark);
        break;
    case 1: // 4x4: tall cockpit and high ground clearance.
        block({0,.55f,0},{1.85f,.65f,1.1f},body);
        block({-.02f,1.02f,0},{1.2f,.55f,.92f},dark);
        block({.31f,1.08f,0},{.06f,.30f,.87f},windows);
        block({-.93f,.39f,0},{.18f,.30f,1.2f},GRAY);
        break;
    case 2: // GT3: low, long coupe and movable rear wing.
        block({0,.32f,0},{1.75f,.43f,.89f},body);
        block({-.12f,.66f,0},{.82f,.34f,.70f},dark);
        block({.32f,.65f,0},{.06f,.23f,.62f},windows);
        block({-.92f,.96f,0},{.22f,.13f,1.10f},BLACK);
        block({.90f,.20f,0},{.22f,.11f,1.05f},BLACK);
        break;
    default: // WEC: long, low hypercar with large aero surfaces.
        block({0,.27f,0},{2.35f,.32f,1.0f},body);
        block({-.13f,.54f,0},{1.03f,.30f,.69f},dark);
        block({.42f,.54f,0},{.08f,.23f,.57f},windows);
        block({-1.13f,.83f,0},{.25f,.13f,1.17f},BLACK);
        block({1.17f,.14f,0},{.30f,.10f,1.0f},BLACK);
        for (const float z:{-.43f,.43f})
            block({-.85f,.18f,z},{.75f,.18f,.19f},dark);
        break;
    }

    // Wheels rotate with speed and the front wheels with steering.
    const float front=float(profile.frontAxle)*s;
    const float rear=float(profile.rearAxle)*s;
    const float side=float(profile.halfWheelTrack)*s;
    const float wheelRadius=(index==1?.25f:index==0?.13f:.19f);
    for (const float x:{rear,front}) {
        for (const float z:{-side,side}) {
            const bool frontAxle=x==front;
            const float steer=frontAxle?float(car.steering):0;
            rlPushMatrix();
            rlTranslatef(x,wheelRadius,z);
            rlRotatef(float(-steer*RAD2DEG),0,1,0);
            DrawCylinderEx({0,0,-.12f},{0,0,.12f},wheelRadius,wheelRadius,10,
                           BLACK);
            DrawCylinderEx({0,0,z<0?-.14f:.14f},{0,0,z<0?-.15f:.15f},
                           wheelRadius*.46f,wheelRadius*.46f,10,
                           Color{170,175,185,255});
            rlPopMatrix();
        }
    }
    const float frontX=std::max(front+.24f,index==3?1.10f:.65f);
    const float backX=std::min(rear-.30f,index==3?-1.15f:-.62f);
    for (float z:{-.32f,.32f}) {
        DrawSphere({frontX,.35f,z},.075f,
                   night?Color{255,255,208,255}:Color{225,226,190,255});
        DrawSphere({backX,.35f,z},.070f,
                   car.brake>.1?RED:Color{130,25,25,255});
    }
    rlPopMatrix();
}

} // namespace karting
