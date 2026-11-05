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
    for (int i=0;i<300;++i) {
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
    // Build the visible terrain from the same elevation field as the road.
    // This removes the flat green "table" underneath the hilly circuit.
    const double tile=quality>=3?210.0:420.0;
    const int baseX=int(std::floor((cameraPosition.x/kWorldScale)/tile));
    const int baseZ=int(std::floor((cameraPosition.z/kWorldScale)/tile));
    const int terrainRadius=quality==0?6:quality==1?8:quality==2?10:13;
    for(int gx=baseX-terrainRadius;gx<=baseX+terrainRadius;++gx) {
        for(int gz=baseZ-terrainRadius;gz<=baseZ+terrainRadius;++gz) {
            const double x0=std::max(0.0,gx*tile);
            const double z0=std::max(0.0,gz*tile);
            const double x1=std::min(9800.0,(gx+1)*tile);
            const double z1=std::min(8700.0,(gz+1)*tile);
            if(x1<=x0 || z1<=z0) continue;
            const auto a=worldPoint({x0,z0},-2.5f);
            const auto b=worldPoint({x1,z0},-2.5f);
            const auto c=worldPoint({x1,z1},-2.5f);
            const auto d=worldPoint({x0,z1},-2.5f);
            const int patch=(std::abs(gx*7+gz*11)%5);
            quad(a,b,c,d,shaded(grass,.94f+patch*.025f));
        }
    }

    const auto count=track.size();
    const float farDistance=quality==0?47.f:quality==1?63.f:quality==2?81.f:98.f;
    const auto& center=track.center();
    for (std::size_t i=0;i<count;) {
        const Vector3 mid=worldPoint(center[i]);
        const float dx=mid.x-cameraPosition.x, dz=mid.z-cameraPosition.z;
        const float d2=dx*dx+dz*dz;
        if (d2>(farDistance+12)*(farDistance+12)) { ++i; continue; }
        const std::size_t step=(d2<40*40?1:quality==0?5:quality==1?3:quality==2?2:1);
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
        // Barriers follow the geometry: only the outside of corners,
        // plus short sections directly in front of spectator grandstands.
        const Vec2 incoming=center[i]-center[(i+count-1)%count];
        const Vec2 outgoing=center[j]-center[i];
        const double directionScale=std::max(1e-8,length(incoming)*length(outgoing));
        const double turn= (incoming.x*outgoing.y-incoming.y*outgoing.x)/directionScale;
        const bool isCorner=std::abs(turn)>.002;
        const int outsideSide=turn>0?-1:1;

        bool publicZone=false;
        for(std::size_t stand=95;stand<330;stand+=20) {
            if(i>=stand && i<stand+10) { publicZone=true; break; }
        }

        // Selected corner exits use tyre walls instead of metal barriers.
        const double progress=double(i)/double(count);
        const bool tyreSection=
            (progress>.125 && progress<.145) ||
            (progress>.425 && progress<.445) ||
            (progress>.715 && progress<.735);

        if (d2<58*58) {
            for (int side:{-1,1}) {
                if (side>0 && track.isPitIndex(i)) continue;
                const bool outsideCorner=isCorner && side==outsideSide;
                const bool spectatorSide=publicZone && side==-1;
                if(!outsideCorner && !spectatorSide) continue;

                const bool useTyres=tyreSection && outsideCorner;
                if(useTyres) {
                    if(quality>0 && i%2==0) {
                        const auto p=worldPoint(track.point(i,side*(Track::wall-5)),1);
                        const float tireHeight=.28f;
                        const float radius=.34f;
                        const Color tire=shaded({25,27,31,255},1-.35f*night);
                        DrawCylinderEx(p,{p.x,p.y+tireHeight,p.z},
                                       radius,radius,quality>=3?20:12,tire);
                        DrawCylinderEx({p.x,p.y+tireHeight,p.z},
                                       {p.x,p.y+2*tireHeight,p.z},
                                       radius,radius,12,tire);
                    }
                } else {
                    const double offset=side*(Track::wall-5);
                    const auto baseA=worldPoint(track.point(i,offset),1);
                    const auto baseB=worldPoint(track.point(j,offset),1);
                    const auto topA=worldPoint(track.point(i,offset),1+.58f/kWorldScale);
                    const auto topB=worldPoint(track.point(j,offset),1+.58f/kWorldScale);
                    quad(baseA,baseB,topB,topA,
                         shaded({174,179,187,255},1-.48f*night));
                    DrawLine3D(topA,topB,shaded(WHITE,1-.5f*night));
                }
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

    // Tree positions were generated once, instead of walking the circuit
    // and calling nearest-point search on every rendered frame.
    for (const auto& p:trees_) {
        const auto origin=worldPoint(p);
        const float dx=origin.x-cameraPosition.x,dz=origin.z-cameraPosition.z;
        const float distance2=dx*dx+dz*dz;
        const float treeFar=quality==0?28.f:quality==1?39.f:quality==2?50.f:62.f;
        if (distance2>treeFar*treeFar) continue;
        const int variation=std::abs(int(p.x*.13+p.y*.27))%4;
        const float size=(variation==2?1.18f:variation==1?.72f:0.88f)+
                         float(std::abs(int(p.x+p.y))%9)/18.f;
        const auto foliage=shaded(
            variation==1?Color{67,145,62,255}:
            variation==2?Color{35,105,48,255}:Color{42,127,51,255},
            1-0.60f*night);
        DrawCylinderEx(origin,{origin.x,origin.y+size*.95f,origin.z},
                       .07f*size,.045f*size,5,
                       shaded({98,67,44,255},1-.5f*night));
        if(variation==0) {
            // Tall conifer.
            DrawCylinderEx({origin.x,origin.y+size*.45f,origin.z},
                           {origin.x,origin.y+size*2.0f,origin.z},
                           size*.43f,.015f,7,foliage);
        } else if(variation==3) {
            // Split crown, with two overlapping canopies.
            DrawSphere({origin.x-size*.20f,origin.y+size*1.30f,origin.z},
                       .42f*size,foliage);
            DrawSphere({origin.x+size*.20f,origin.y+size*1.48f,origin.z},
                       .38f*size,shaded(foliage,1.12f));
        } else {
            DrawSphere({origin.x,origin.y+size*1.35f,origin.z},
                       .48f*size,foliage);
            if(quality>0 && distance2<25*25)
                DrawSphere({origin.x-.21f*size,origin.y+size*1.65f,origin.z},
                           .35f*size,shaded(foliage,1.1f));
            if(quality>=3 && distance2<38*38) {
                DrawSphere({origin.x+size*.34f,origin.y+size*1.30f,origin.z+size*.18f},
                           .31f*size,shaded(foliage,1.08f));
                DrawSphere({origin.x-size*.28f,origin.y+size*1.42f,origin.z-size*.20f},
                           .29f*size,shaded(foliage,.92f));
            }
        }
    }
}

} // namespace karting
