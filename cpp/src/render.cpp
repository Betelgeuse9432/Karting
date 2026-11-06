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
        // A consistent three-part infrastructure: shaped earth ramps,
        // concrete approach walls, then a raised deck. No terrain edits.
        const double riseI=track.bridgeRise(i),riseJ=track.bridgeRise(j);
        const double riseHigh=std::max(riseI,riseJ);
        if(riseHigh>3 && riseHigh<225) {
            // Swept, two-slope embankment: broad foot / shallow shoulder /
            // narrow compacted berm. It fades into the native terrain.
            const Color earth=shaded({93,112,76,255},1-.32f*night);
            const Color compact=shaded({124,128,101,255},1-.30f*night);
            for(const int side:{-1,1}) {
                const double edge=side*(Track::halfWidth+Track::kerb);
                const double shoulder=side*(Track::halfWidth+Track::kerb+105);
                const double toe=side*(Track::halfWidth+Track::kerb+390);
                const auto topA=roadPoint(i,edge,-2),topB=roadPoint(j,edge,-2);
                const auto sidePoint=[&](std::size_t idx,double offset,double fraction) {
                    const Vec2 pos=track.point(idx,offset);
                    const double rise=track.bridgeRise(idx);
                    return Vector3{float(pos.x*kWorldScale),
                      float((elevation(pos)+fraction*rise-9)*kWorldScale),
                      float(pos.y*kWorldScale)};
                };
                const auto shelfA=sidePoint(i,shoulder,.74);
                const auto shelfB=sidePoint(j,shoulder,.74);
                const auto footA=sidePoint(i,toe,0);
                const auto footB=sidePoint(j,toe,0);
                quad(topA,topB,shelfB,shelfA,compact);
                quad(shelfA,shelfB,footB,footA,earth);
            }
        }
        // Stiff concrete beams with an inset shadow reveal rather than a
        // paper-thin dark slab. At the ends the ramp gradually hides them.
        if(riseHigh>165) {
            const Color face=shaded({155,160,167,255},1-.38f*night);
            const Color band=shaded({91,99,110,255},1-.40f*night);
            const Color under=shaded({72,78,89,255},1-.44f*night);
            for(const int side:{-1,1}) {
                const auto a=roadPoint(i,side*(Track::halfWidth+Track::kerb),-3);
                const auto b=roadPoint(j,side*(Track::halfWidth+Track::kerb),-3);
                const auto c=roadPoint(j,side*(Track::halfWidth+Track::kerb),-29);
                const auto d=roadPoint(i,side*(Track::halfWidth+Track::kerb),-29);
                quad(a,b,c,d,face);
                const auto e=roadPoint(i,side*(Track::halfWidth+Track::kerb),-23);
                const auto g=roadPoint(j,side*(Track::halfWidth+Track::kerb),-23);
                const auto h=roadPoint(j,side*(Track::halfWidth+Track::kerb),-29);
                const auto k=roadPoint(i,side*(Track::halfWidth+Track::kerb),-29);
                quad(e,g,h,k,band);
            }
            quad(roadPoint(i,-Track::halfWidth-Track::kerb,-29),
                 roadPoint(j,-Track::halfWidth-Track::kerb,-29),
                 roadPoint(j,Track::halfWidth+Track::kerb,-29),
                 roadPoint(i,Track::halfWidth+Track::kerb,-29),under);
        }
        // Continuous raised parapets make the bridge silhouette readable.
        // Placed outside the race surface, with no extra collision geometry.
        if(quality>0 && riseHigh>165 && d2<55.f*55.f) {
            const Color concrete=shaded({181,186,192,255},1-.35f*night);
            const Color cap=shaded({226,228,227,255},1-.32f*night);
            for(const int side:{-1,1}) {
                const double off=side*(Track::halfWidth+Track::kerb+14);
                const auto a=roadPoint(i,off,0),b=roadPoint(j,off,0);
                const auto up=[](Vector3 p,float d){p.y+=d;return p;};
                quad(a,b,up(b,.84f),up(a,.84f),concrete);
                DrawCylinderEx(up(a,.87f),up(b,.87f),.060f,.060f,5,cap);
            }
        }
        if (quality>0 && d2<38*38) {
            for (int side:{-1,1}) {
                if (side>0 && track.isPitIndex(i)) continue;
                const auto a=roadPoint(i,side*Track::wall,18);
                const auto b=roadPoint(j,side*Track::wall,18);
                const auto a2=roadPoint(i,side*Track::wall,25);
                const auto b2=roadPoint(j,side*Track::wall,25);
                DrawLine3D(a,b,shaded(LIGHTGRAY,1-.5f*night));
                DrawLine3D(a2,b2,shaded(GRAY,1-.5f*night));
            }
        }
        i+=step;
    }

    // One masonry-style arch centered over the lower approach road.
    // Its opening runs ALONG the elevated track; the lower road crosses
    // that opening perpendicular to the bridge. No columns in the tunnel.
    if(quality>0) {
        constexpr Vec2 crossing{14689,2401};
        constexpr int halfArchSamples=15;
        std::size_t crossingIndex=14*Track::subdivisions;
        double bestDistance=1e30;
        for(std::size_t k=12*Track::subdivisions;
            k<=16*Track::subdivisions && k<count;++k) {
            const Vec2 delta=track.point(k)-crossing;
            const double d2=dot(delta,delta);
            if(d2<bestDistance) {
                bestDistance=d2;
                crossingIndex=k;
            }
        }
        const Vector3 archCenter=roadPoint(crossingIndex,0);
        const float adx=archCenter.x-cameraPosition.x;
        const float adz=archCenter.z-cameraPosition.z;
        if(adx*adx+adz*adz<80.f*80.f &&
           frustum.visible(archCenter,23.f)) {
            // Keep the arch OPEN: no massive spandrel walls between
            // the curved ring and the bridge deck.
            const Color ring=shaded({186,189,190,255},1-.35f*night);
            const Color edge=shaded({118,126,135,255},1-.36f*night);
            const Color tunnel=shaded({68,75,83,255},1-.43f*night);
            constexpr double lip=Track::halfWidth+Track::kerb+24;
            constexpr double spring=8.0;
            // For each longitudinal slice, define the bottom of the arch.
            // At its centre the opening is close to the underside of the
            // deck; at the spring points it joins the natural ground.
            const auto archLevel=[&](std::size_t k,int offset) {
                const double u=double(offset)/halfArchSamples;
                const double crown=std::sqrt(std::max(0.0,1.0-u*u));
                const Vec2 center=track.point(k);
                const double ground=elevation(center);
                const double deck=track.roadHeightAt(center,k);
                return ground+spring+
                       std::max(0.0,deck-ground-80.0-spring)*crown;
            };
            const auto at=[&](std::size_t k,double lateral,double height) {
                const Vec2 p=track.point(k,lateral);
                return Vector3{float(p.x*kWorldScale),
                               float(height*kWorldScale),
                               float(p.y*kWorldScale)};
            };
            const auto slice=[&](std::size_t k,int offset,int side,
                                 double aboveArch) {
                const Vec2 p=track.point(k,side*lip);
                const double terrain=elevation(p);
                const double axisGround=elevation(track.point(k));
                // Follow the lateral ground undulation, rather than
                // creating a straight floating arch edge.
                const double local=archLevel(k,offset)+terrain-axisGround;
                return at(k,side*lip,local+aboveArch);
            };
            const std::size_t begin=crossingIndex-halfArchSamples;
            for(int j=-halfArchSamples;j<halfArchSamples;++j) {
                const std::size_t i=begin+std::size_t(j+halfArchSamples);
                const std::size_t next=i+1;
                const auto ceilingLeft=slice(i,j,-1,0);
                const auto ceilingRight=slice(i,j,1,0);
                const auto nextLeft=slice(next,j+1,-1,0);
                const auto nextRight=slice(next,j+1,1,0);
                // Curved vault visible from beneath. A separate roof
                // surface avoids a hollow-looking paper bridge.
                quad(ceilingLeft,nextLeft,nextRight,ceilingRight,tunnel);
                for(const int side:{-1,1}) {
                    const auto footA=slice(i,j,side,0);
                    const auto footB=slice(next,j+1,side,0);
                    // A narrow, continuous curved stone ring, not a
                    // full-height grey wall. Its depth stays restrained
                    // near the crown where the road deck is lowest.
                    const auto topA=roadPoint(i,side*lip,-29);
                    const auto topB=roadPoint(next,side*lip,-29);
                    auto trimA=slice(i,j,side,20);
                    auto trimB=slice(next,j+1,side,20);
                    trimA.y=std::min(trimA.y,topA.y-.04f);
                    trimB.y=std::min(trimB.y,topB.y-.04f);
                    if(trimA.y>footA.y && trimB.y>footB.y)
                        quad(footA,footB,trimB,trimA,ring);
                    // Thin under-deck edge keeps the two parts visually
                    // connected without closing the space above the arch.
                    const auto seamA=roadPoint(i,side*lip,-34);
                    const auto seamB=roadPoint(next,side*lip,-34);
                    quad(seamA,seamB,topB,topA,edge);
                }
            }
            // Slim diagonal struts transfer the arch ring into the deck.
            // They replace the old filled sidewalls, not the road opening.
            // At the crown the gap is already short; elsewhere the braces
            // follow the curve without extending down into the underpass.
            if(quality>0) {
                const Color brace=shaded({148,155,164,255},1-.36f*night);
                const Color node=shaded({207,210,211,255},1-.33f*night);
                for(const int relative:{-11,-6,-2,2,6,11}) {
                    const std::size_t k=std::size_t(
                        int(crossingIndex)+relative);
                    const std::size_t attach=std::size_t(
                        int(crossingIndex)+relative+(relative<0?3:-3));
                    for(const int side:{-1,1}) {
                        const Vector3 foot=slice(k,relative,side,20);
                        const Vector3 cap=roadPoint(attach,side*lip,-32);
                        // No visible strut if the concrete deck already
                        // meets the curved crown.
                        if(cap.y-foot.y<.20f)continue;
                        DrawCylinderEx(foot,cap,.14f,.115f,7,brace);
                        DrawSphereEx(foot,.19f,5,7,node);
                    }
                }
            }
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
