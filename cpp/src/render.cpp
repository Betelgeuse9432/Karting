#include "karting/game.hpp"
#include "karting/bridge_geometry.hpp"
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

static Vector3 raised(Vector3 v,float amount) {
    v.y+=amount;
    return v;
}

// An actual closed, two-sided solid with thickness, not a floating quad.
// Its lower, taller face is darkened to give the barrier readable depth.
static void solidGuardSegment(Vector3 innerA,Vector3 innerB,
                              Vector3 outerA,Vector3 outerB,
                              float bottom,float top,
                              Color inside,Color outside,Color crown) {
    const Vector3 ai=raised(innerA,bottom),bi=raised(innerB,bottom);
    const Vector3 ao=raised(outerA,bottom),bo=raised(outerB,bottom);
    const Vector3 ci=raised(innerA,top),di=raised(innerB,top);
    const Vector3 co=raised(outerA,top),dO=raised(outerB,top);
    quad(ai,bi,di,ci,inside);
    quad(ao,bo,dO,co,outside);
    quad(ci,di,dO,co,crown);
    quad(ai,ao,bo,bi,outside);
    quad(ai,ci,co,ao,inside);
    quad(bi,bo,dO,di,outside);
}

static Vector3 vertex3(bridge::SectionVertex point) {
    return {float(point.position.x*kWorldScale),
            float(point.altitude*kWorldScale),
            float(point.position.y*kWorldScale)};
}

static void solidEmbankment(const Track& track,std::size_t i,std::size_t j,
                            float night) {
    const auto first=bridge::embankmentSection(track,i);
    const auto second=bridge::embankmentSection(track,j);
    const Color sod=shaded({78,116,59,255},1-.46f*night);
    const Color packed=shaded({126,116,86,255},1-.40f*night);
    const Color exposed=shaded({118,92,67,255},1-.43f*night);
    const Color rock=shaded({99,92,81,255},1-.43f*night);
    const Color deep=shaded({72,66,59,255},1-.43f*night);
    const float variation=0.94f+float((i/5)%7)*.015f;
    const std::array<Color,7> strata{{
        shaded(exposed,variation),shaded(sod,variation),
        packed,shaded(sod,variation),shaded(exposed,variation),
        deep,shaded(rock,variation)
    }};
    // Sweep EVERY edge of the cross section, including the buried
    // underside. The top lies just below the asphalt and shoulder.
    for(std::size_t k=0;k<first.size();++k) {
        const auto next=(k+1)%first.size();
        quad(vertex3(first[k]),vertex3(second[k]),
             vertex3(second[next]),vertex3(first[next]),strata[k]);
    }
    // Explicitly seal both ends, including LOD boundaries. A side view
    // through the ramp now shows packed soil, never an empty green sheet.
    for(const auto& end:{std::pair<std::size_t,bridge::EmbankmentSection>{i,first},
                        {j,second}}) {
        const Vec2 xy=track.point(end.first);
        const Vector3 center{
            float(xy.x*kWorldScale),
            float((elevation(xy)+track.bridgeRise(end.first)*.42-18)*kWorldScale),
            float(xy.y*kWorldScale)};
        for(std::size_t k=0;k<end.second.size();++k) {
            const auto next=(k+1)%end.second.size();
            const auto a=vertex3(end.second[k]);
            const auto b=vertex3(end.second[next]);
            DrawTriangle3D(center,a,b,shaded(packed,variation*.82f));
            DrawTriangle3D(center,b,a,shaded(packed,variation*.82f));
        }
    }
}

void WorldRenderer::draw(const Track& track,const Weather& weather,int quality,
                         const Camera3D& camera,double time) const {
    const Vector3 cameraPosition=camera.position;
    const float aspect=float(GetScreenWidth())/
                       float(std::max(1,GetScreenHeight()));
    const ViewFrustum frustum(camera,aspect,150.f);
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    const auto asphalt=shaded({84,86,95,255},1-0.35f*rain-0.52f*night);
    const auto kerbRed=shaded({223,37,37,255},1-0.25f*night);
    const auto kerbWhite=shaded({230,228,218,255},1-0.4f*night);
    const auto grass=shaded({53,153,57,255},1-0.1f*rain-0.55f*night);
    // A distant lower base hides the far edge of the camera-local terrain.
    // Keep the distant backdrop safely beneath the deeper valleys.
    DrawPlane({float(Track::worldWidth*kWorldScale*.5f),-18.f,
               float(Track::worldHeight*kWorldScale*.5f)},
              {float(Track::worldWidth*kWorldScale),
               float(Track::worldHeight*kWorldScale)},
              shaded(grass,.68f));
    // Near and mid-distance ground follows the same relief as the asphalt.
    drawTerrain(weather,quality,frustum,cameraPosition);

    const auto count=track.size();
    // The circuit's asphalt can now be elevated independently of terrain.
    const auto roadPoint=[&](std::size_t i,double offset,double lift=0.0) {
        const Vec2 p=track.point(i,offset);
        return Vector3{float(p.x*kWorldScale),
                       float((track.roadHeightAt(p,i)+lift)*kWorldScale),
                       float(p.y*kWorldScale)};
    };
    const float farDistance=quality==0?47.f:quality==1?63.f:81.f;
    const auto& center=track.center();
    for (std::size_t i=0;i<count;) {
        const Vector3 mid=roadPoint(i,0);
        const float dx=mid.x-cameraPosition.x, dz=mid.z-cameraPosition.z;
        const float d2=dx*dx+dz*dz;
        if (d2>(farDistance+12)*(farDistance+12)) { ++i; continue; }
        const std::size_t step=(d2<28*28?1:quality==0?5:quality==1?3:2);
        const auto j=(i+step)%count;
        const Vector3 next=roadPoint(j,0);
        const Vector3 segmentMid=Vector3Scale(Vector3Add(mid,next),.5f);
        const float segmentRadius=Vector3Distance(mid,next)*.5f+7.f;
        if(!frustum.visible(segmentMid,segmentRadius)) {
            i+=step;
            continue;
        }
        const auto l=[&](std::size_t k,double off) {
            return roadPoint(k,off,1);
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
        // Earth ramps, retaining abutments and structural deck.
        // On the low approaches, every cross section is a CLOSED soil volume.
        const double riseI=track.bridgeRise(i),riseJ=track.bridgeRise(j);
        const double riseHigh=std::max(riseI,riseJ);
        if(bridge::isFilledEmbankment(riseI,riseJ))
            solidEmbankment(track,i,j,night);

        const double deckWidth=bridge::kDeckHalfWidth;
        if(riseHigh>bridge::kFillMinRise) {
            // Render actual paved shoulders above the compacted fill.
            // On elevated portions these sit on the widened concrete slab.
            const Color shoulder=shaded({111,116,117,255},
                                        1-.22f*rain-.43f*night);
            paint(-deckWidth,-Track::halfWidth-Track::kerb,shoulder);
            paint(Track::halfWidth+Track::kerb,deckWidth,shoulder);
        }
        if(riseHigh>bridge::kDeckMinRise) {
            const Color face=shaded({192,194,198,255},1-.34f*night);
            const Color band=shaded({139,146,157,255},1-.38f*night);
            const Color underside=shaded({112,123,138,255},1-.43f*night);
            for(const int side:{-1,1}) {
                const double edge=side*deckWidth;
                quad(roadPoint(i,edge,-3),roadPoint(j,edge,-3),
                     roadPoint(j,edge,-32),roadPoint(i,edge,-32),face);
                quad(roadPoint(i,edge,-25),roadPoint(j,edge,-25),
                     roadPoint(j,edge,-32),roadPoint(i,edge,-32),band);
            }
            // Solid soffit and sealed abutment sections instead of a
            // thin roof open when viewed from the underpass.
            quad(roadPoint(i,-deckWidth,-32),roadPoint(j,-deckWidth,-32),
                 roadPoint(j,deckWidth,-32),roadPoint(i,deckWidth,-32),
                 underside);
            quad(roadPoint(i,-deckWidth,-3),roadPoint(i,deckWidth,-3),
                 roadPoint(i,deckWidth,-32),roadPoint(i,-deckWidth,-32),
                 face);
            quad(roadPoint(j,-deckWidth,-3),roadPoint(j,deckWidth,-3),
                 roadPoint(j,deckWidth,-32),roadPoint(j,-deckWidth,-32),
                 face);
        }
        // One continuous protection system, including the approach ramps.
        // Concrete lower toe + thick metallic beam, both closed solids.
        // The top is only 48 cm above the road instead of 84 cm.
        if((riseHigh>bridge::kFillMinRise || quality>0)
           && d2<(riseHigh>bridge::kFillMinRise?55.f*55.f:38.f*38.f)) {
            const Color foot=shaded({169,173,176,255},1-.42f*night);
            const Color beam=shaded({188,193,198,255},1-.40f*night);
            const Color outside=shaded({124,135,145,255},1-.43f*night);
            const Color railTop=shaded({227,230,232,255},1-.37f*night);
            for(const int side:{-1,1}) {
                if(side>0 && track.isPitIndex(i)) continue;
                const auto innerA=roadPoint(i,side*bridge::kBarrierInner,-1);
                const auto innerB=roadPoint(j,side*bridge::kBarrierInner,-1);
                const auto outerA=roadPoint(i,side*bridge::kBarrierOuter,-1);
                const auto outerB=roadPoint(j,side*bridge::kBarrierOuter,-1);
                solidGuardSegment(innerA,innerB,outerA,outerB,0.f,
                                  bridge::kBarrierFootHeight,
                                  foot,outside,railTop);
                solidGuardSegment(innerA,innerB,outerA,outerB,
                                  bridge::kBarrierRailBottom,
                                  bridge::kBarrierTotalHeight,
                                  beam,outside,railTop);
                // Reflective rail posts give a sense of thickness and speed.
                if(quality>0 && i%8<step && d2<33.f*33.f) {
                    const auto pos=roadPoint(i,side*(Track::wall-10),0);
                    DrawCubeV(raised(pos,.26f),
                              {.13f,.52f,.13f},
                              shaded({103,112,122,255},1-.43f*night));
                    DrawCubeV(raised(pos,.34f),
                              {.15f,.055f,.055f},
                              side>0?shaded({224,69,47,255},1-.38f*night):
                                     shaded({245,217,97,255},1-.38f*night));
                }
            }
        }
        i+=step;
    }

    // Modern viaduct supports: two restrained, rectangular concrete piers.
    // The crossover itself remains completely free for the lower road.
    // Unlike the former arch, nothing spans above the opening.
    if (quality>0) {
        const Color pier=shaded({183,186,192,255},1-.36f*night);
        const Color footing=shaded({124,130,138,255},1-.40f*night);
        const Color cap=shaded({214,215,216,255},1-.34f*night);
        // Locations on either side of the crossover, away from the lower
        // straight. Do not place any support under the crossing itself.
        for(const std::size_t i:{
                std::size_t(12*Track::subdivisions),
                std::size_t(17*Track::subdivisions)}) {
            if(i>=count || track.bridgeRise(i)<160) continue;
            const Vec2 xy=track.point(i);
            const Vector3 top=roadPoint(i,0,-31);
            const float dx=top.x-cameraPosition.x;
            const float dz=top.z-cameraPosition.z;
            if(dx*dx+dz*dz>62.f*62.f || !frustum.visible(top,9.f)) continue;
            // Protect the full width of the lower carriageway plus its
            // barriers, even if the track changes slightly in the future.
            bool nearUnderpass=false;
            for(std::size_t j=0;j<5*Track::subdivisions;j+=2) {
                if(length(xy-track.point(j))<Track::wall+135) {
                    nearUnderpass=true;
                    break;
                }
            }
            if(nearUnderpass) continue;
            const float ground=float(elevation(xy)*kWorldScale);
            const float available=top.y-ground;
            if(available<1.6f) continue;
            const Vector3 middle{top.x,(top.y+ground)*.5f,top.z};
            // Narrow square pier, oriented simply with the roadway.
            DrawCubeV(middle,{1.15f,available,1.15f},pier);
            DrawCubeV({top.x,ground+.10f,top.z},
                      {1.65f,.20f,1.65f},footing);
            DrawCubeV({top.x,top.y-.10f,top.z},
                      {1.60f,.20f,1.60f},cap);
        }
    }

    // Close roadside reflectors give passing-speed cues without fake blur.
    // One pair about every 25-30 metres, culled before any draw calls.
    if (quality>0) {
        for (std::size_t i=0;i<count;i+=42) {
            const Vector3 centerPoint=roadPoint(i,0);
            const float dx=centerPoint.x-cameraPosition.x;
            const float dz=centerPoint.z-cameraPosition.z;
            if (dx*dx+dz*dz>65.f*65.f ||
                !frustum.visible(centerPoint,5.f)) continue;
            for (int side:{-1,1}) {
                if (side>0 && track.isPitIndex(i)) continue;
                const auto base=roadPoint(
                    i,side*(Track::halfWidth+Track::kerb+34));
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
        const auto a=roadPoint(0,-Track::halfWidth-25);
        const auto b=roadPoint(0,Track::halfWidth+25);
        if (Vector3Distance(a,cameraPosition)<65.f &&
            (frustum.visible(a,5.f) || frustum.visible(b,5.f))) {
            DrawCylinderEx(a,{a.x,a.y+3.8f,a.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx(b,{b.x,b.y+3.8f,b.z},.13f,.13f,6,LIGHTGRAY);
            DrawCylinderEx({a.x,a.y+3.8f,a.z},
                           {b.x,b.y+3.8f,b.z},.23f,.23f,6,
                           shaded(WHITE,1-.5f*night));
        }
    }

    // Static scenery is generated once and culled before drawing.
    drawScenery(weather,quality,frustum,cameraPosition,time);
}

} // namespace karting
