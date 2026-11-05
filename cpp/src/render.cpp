#include "karting/game.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

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

static Texture2D makeSurfaceTexture(bool asphalt) {
    constexpr int size=512;
    Image image=GenImageColor(size,size,asphalt?Color{78,80,86,255}:Color{65,128,53,255});
    std::mt19937 rng(asphalt?173u:947u);
    std::uniform_int_distribution<int> grain(-13,13);
    std::uniform_int_distribution<int> fleck(0,99);
    for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
        const int n=grain(rng);
        Color c{};
        if(asphalt) {
            const int value=std::clamp(79+n+(fleck(rng)<5?-12:0),35,112);
            c={static_cast<unsigned char>(value),
               static_cast<unsigned char>(std::clamp(value+grain(rng)/3,0,255)),
               static_cast<unsigned char>(std::clamp(value+grain(rng)/2,0,255)),255};
        } else {
            const int value=std::clamp(116+n*2,65,165);
            c={static_cast<unsigned char>(std::clamp(value/2+n/3,0,255)),
               static_cast<unsigned char>(value),
               static_cast<unsigned char>(std::clamp(value/2+n/2,0,255)),255};
        }
        ImageDrawPixel(&image,x,y,c);
    }
    // Mipmaps keep the fine grain stable at a distance and reduce shimmer.
    ImageMipmaps(&image);
    Texture2D texture=LoadTextureFromImage(image);
    UnloadImage(image);
    if(texture.id) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture,TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture,TEXTURE_WRAP_REPEAT);
    }
    return texture;
}

static void texturedQuad(Vector3 a,Vector3 b,Vector3 c,Vector3 d,
                         Texture2D texture,float uRepeat,float vRepeat) {
    if(!texture.id) return;
    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255,255,255,255);
    rlTexCoord2f(0,0);             rlVertex3f(a.x,a.y,a.z);
    rlTexCoord2f(uRepeat,0);       rlVertex3f(b.x,b.y,b.z);
    rlTexCoord2f(uRepeat,vRepeat); rlVertex3f(c.x,c.y,c.z);
    rlTexCoord2f(0,vRepeat);       rlVertex3f(d.x,d.y,d.z);
    rlEnd();
    rlSetTexture(0);
}

namespace {
void drawMountain(Vector3 base, float radius, float height, Color color,
                  int slices) {
    const Vector3 peak{base.x,base.y+height,base.z};
    DrawCylinderEx(base,peak,radius,.12f,slices,color);
    // A broad lower shoulder makes the silhouette less like a cone.
    const Vector3 shoulder{base.x,base.y+height*.48f,base.z};
    DrawCylinderEx(base,shoulder,radius*.82f,radius*.48f,slices,
                   Fade(color,.92f));
}
void draw3DLandscape(Vector3 camera, int quality, float night, double time) {
    const Color far{83,105,126,255};
    const Color mid{68,91,76,255};
    const std::array<Vector3,8> mountainBases{{
        {-45,0,-36},{12,0,-52},{72,0,-48},{125,0,-34},
        {190,0,8},{168,0,92},{105,0,145},{5,0,130}
    }};
    const float visibility=quality==0?0.78f:1.0f;
    for(std::size_t i=0;i<mountainBases.size();++i) {
        const auto b=mountainBases[i];
        const float distance=Vector3Distance(camera,b);
        if(distance>(quality==0?260.0f:360.0f)) continue;
        const float radius= i%3==0?23.0f:17.0f;
        const float height= i%3==0?25.0f:17.0f;
        Color c=i%2==0?far:mid;
        c=Fade(c,visibility);
        drawMountain(b,radius,height,c,quality>=3?16:10);
        if(quality>=2 && i%2==0) {
            const Vector3 flank{b.x+radius*.72f,b.y,b.z+radius*.28f};
            drawMountain(flank,radius*.55f,height*.66f,
                         Fade(c,.82f),quality>=3?12:8);
        }
    }

    // Fixed 3D cloud banks float over the circuit. Their world positions
    // make parallax and camera movement visible, unlike a screen overlay.
    const std::array<Vector3,7> cloudCenters{{
        {-24,38,-12},{22,45,-35},{70,41,-22},{116,48,12},
        {158,39,48},{120,44,100},{38,42,92}
    }};
    const float drift=float(std::fmod(time*.12,18.0));
    for(std::size_t i=0;i<cloudCenters.size();++i) {
        Vector3 p=cloudCenters[i];
        p.x+=drift;
        if(Vector3Distance(camera,p)>(quality==0?95.0f:145.0f)) continue;
        const float scale= i%3==0?1.0f:.78f;
        const Color cloud=Fade({224,231,239,255},(1.0f-night*.68f)*.88f);
        DrawSphere({p.x,p.y,p.z},4.2f*scale,cloud);
        DrawSphere({p.x+3.1f*scale,p.y+.8f*scale,p.z+.3f},
                   3.3f*scale,cloud);
        DrawSphere({p.x-2.8f*scale,p.y+.25f*scale,p.z-.2f},
                   2.8f*scale,cloud);
    }
}
void drawTrackLandmarks(const Track& track, Vector3 camera, int quality,
                        float night) {
    const auto place=[&](std::size_t index,double offset) {
        return worldPoint(track.point(index,offset),.2f);
    };
    const auto visible=[&](Vector3 p,float range) {
        return Vector3Distance(p,camera)<range;
    };

    // Signature pedestrian bridge over the long back straight.
    if(quality>0) {
        const std::size_t i=std::min<std::size_t>(track.size()-1,track.size()*0.29);
        const auto left=place(i,-Track::halfWidth-17);
        const auto right=place(i,Track::halfWidth+17);
        if(visible(left,78)) {
            const Color concrete=Fade({154,161,171,255},1-night*.48f);
            DrawCylinderEx({left.x,left.y,left.z},{left.x,left.y+4.7f,left.z},
                           .20f,.20f,8,concrete);
            DrawCylinderEx({right.x,right.y,right.z},{right.x,right.y+4.7f,right.z},
                           .20f,.20f,8,concrete);
            DrawCubeV({(left.x+right.x)*.5f,left.y+4.55f,(left.z+right.z)*.5f},
                      {2.0f,.35f,Vector3Distance(left,right)},concrete);
            DrawCubeV({(left.x+right.x)*.5f,left.y+4.9f,(left.z+right.z)*.5f},
                      {1.9f,.18f,Vector3Distance(left,right)},BLUE);
        }
    }

    // A control tower beside the paddock and a tall timing pylon near sector 2.
    const std::array<std::pair<std::size_t,double>,2> towers{{
        {track.size()*0.965,Track::halfWidth+Track::pitWidth+65},
        {track.size()*0.53,-Track::wall-75}
    }};
    for(std::size_t n=0;n<towers.size();++n) {
        const auto p=place(towers[n].first,towers[n].second);
        if(!visible(p,quality==0?35.f:58.f)) continue;
        const Color wall=Fade({177,184,190,255},1-night*.45f);
        DrawCubeV({p.x,p.y+1.7f,p.z},{2.0f,3.4f,1.8f},wall);
        DrawCubeV({p.x,p.y+3.55f,p.z},{2.2f,.35f,2.0f},
                  Fade({48,57,70,255},1-night*.4f));
        DrawCubeV({p.x,p.y+2.35f,p.z+0.92f},{1.25f,.55f,.06f},
                  Fade(n>.5?Color{255,185,75,255}:Color{65,155,220,255},
                       1-night*.25f));
        DrawCylinderEx({p.x+1.0f,p.y+3.6f,p.z},
                       {p.x+1.0f,p.y+5.1f,p.z},.045f,.025f,6,wall);
    }

    // Three distinct corner markers: braking boards and coloured kerb-side pylons.
    for(const std::size_t i:{track.size()*0.16,track.size()*0.43,
                             track.size()*0.72}) {
        const auto p=place(i,-Track::wall-12);
        if(!visible(p,quality==0?26.f:43.f)) continue;
        for(int board=0;board<3;++board) {
            const auto q=worldPoint(track.point(i,-Track::wall-12-board*1.7),.2f);
            DrawCubeV({q.x,q.y+.72f,q.z},{.12f,1.35f,1.1f},
                      board==0?RED:Fade(WHITE,1-night*.35f));
        }
    }
}
} // namespace

void WorldRenderer::initialize(const Track& track) {
    shutdown();
    asphaltTexture_=makeSurfaceTexture(true);
    grassTexture_=makeSurfaceTexture(false);
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

void WorldRenderer::shutdown() {
    if(asphaltTexture_.id) UnloadTexture(asphaltTexture_);
    if(grassTexture_.id) UnloadTexture(grassTexture_);
    asphaltTexture_={};
    grassTexture_={};
}

void WorldRenderer::draw(const Track& track,const Weather& weather,int quality,
                         Vector3 cameraPosition,double time) const {
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    draw3DLandscape(cameraPosition,quality,night,time);
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
            // Always draw a real terrain base first. The texture is a detail layer.
            quad(worldPoint({x0,z0},-2.65f),worldPoint({x1,z0},-2.65f),
                 worldPoint({x1,z1},-2.65f),worldPoint({x0,z1},-2.65f),
                 shaded(grass,.94f+patch*.025f));
            if(grassTexture_.id) {
                const auto ta=worldPoint({x0,z0},-2.45f);
                const auto tb=worldPoint({x1,z0},-2.45f);
                const auto tc=worldPoint({x1,z1},-2.45f);
                const auto td=worldPoint({x0,z1},-2.45f);
                const float u=float(x1-x0)/8.f,v=float(z1-z0)/8.f;
                texturedQuad(ta,tb,tc,td,grassTexture_,u,v);
                // Weather tint is a cheap overlay, not another texture pass.
                if(rain>.05f || night>.05f)
                    quad(a,b,c,d,Fade(shaded(grass,.94f+patch*.025f),.16f*rain+.22f*night));
            } else quad(a,b,c,d,shaded(grass,.94f+patch*.025f));
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
        const auto roadA=l(i,-Track::halfWidth),roadB=l(j,-Track::halfWidth);
        const auto roadC=l(j,Track::halfWidth),roadD=l(i,Track::halfWidth);
        // The asphalt colour pass is always present beneath the texture.
        quad(roadA,roadB,roadC,roadD,shaded(asphalt,int(i/9)%2==0?1.f:.90f));
        if(asphaltTexture_.id) {
            const float roadLength=Vector3Distance(roadA,roadB);
            const float roadWidth=Vector3Distance(roadA,roadD);
            const auto ra=worldPoint(track.point(i,-Track::halfWidth),1.08f);
            const auto rb=worldPoint(track.point(j,-Track::halfWidth),1.08f);
            const auto rc=worldPoint(track.point(j,Track::halfWidth),1.08f);
            const auto rd=worldPoint(track.point(i,Track::halfWidth),1.08f);
            texturedQuad(ra,rb,rc,rd,asphaltTexture_,
                         std::max(.05f,roadLength/4.f),roadWidth/4.f);
            // Subtle night/rain tint keeps the same material responsive to weather.
            if(rain>.05f || night>.05f)
                quad(roadA,roadB,roadC,roadD,
                     Fade(asphalt, .20f*rain+.28f*night));
        }
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

    drawTrackLandmarks(track,cameraPosition,quality,night);

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
