#include "karting/game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace karting {
namespace {

Color shade(Color c,float value) {
    auto channel=[value](unsigned char v) -> unsigned char {
        return static_cast<unsigned char>(std::clamp(v*value,0.f,255.f));
    };
    return {channel(c.r),channel(c.g),channel(c.b),c.a};
}
void cube(float x,float y,float z,float w,float h,float d,Color c) {
    DrawCubeV({x,y,z},{w,h,d},c);
}
void beam(Vector3 a,Vector3 b,float r,Color c) {
    DrawCylinderEx(a,b,r,r,6,c);
}
void poly(Vector3 a,Vector3 b,Vector3 c,Color color) {
    DrawTriangle3D(a,b,c,color);
    DrawTriangle3D(c,b,a,color);
}
float seedNoise(float x,float y) {
    return std::sin(x*.027f+y*.018f)*.42f+
           std::sin(y*.051f-x*.022f)*.27f+
           std::sin(x*.115f+y*.098f)*.12f;
}
float groundY(double x,double z) {
    // Same height function as road and physics; soil is just below asphalt.
    return float(elevation({x,z})*kWorldScale-.19);
}

void cedar(float scale,float night,int color) {
    const Color bark=shade({102,79,55,255},night);
    const Color needles=shade(color%2?Color{44,99,66,255}:Color{38,121,65,255},night);
    beam({0,-.10f,0},{0,1.65f*scale,0},.095f*scale,bark);
    for (int i=0;i<4;++i) {
        float low=(.40f+.39f*i)*scale;
        float hi=(1.57f+.25f*i)*scale;
        float radius=(.85f-.145f*i)*scale;
        DrawCylinderEx({0,low,0},{0,hi,0},radius,0,7,
                       shade(needles, .82f+.085f*i));
    }
}

void oak(float scale,float night,int color) {
    beam({0,-.1f,0},{0,1.85f*scale,0},.16f*scale,
         shade({111,80,55,255},night));
    for(float side:{-1.f,1.f}) {
        beam({0,1.4f*scale,0},{side*.48f*scale,2.13f*scale,.12f*scale},
             .075f*scale,shade({109,83,55,255},night));
    }
    Color leaf=shade(color%2?Color{79,141,60,255}:Color{55,123,58,255},night);
    DrawSphereEx({0,2.14f*scale,0},.93f*scale,7,10,leaf);
    DrawSphereEx({-.53f*scale,1.94f*scale,.1f*scale},.59f*scale,6,8,
                 shade(leaf,.88f));
    DrawSphereEx({.48f*scale,2.3f*scale,.19f*scale},.62f*scale,6,9,
                 shade(leaf,1.10f));
}

// Far objects keep their silhouette and colour but skip branches,
// individual leaves and the crowd's hundreds of primitive draw calls.
void cedarFar(float size,float night,int color) {
    const Color green=shade(color%2?Color{44,99,66,255}:
                                    Color{38,121,65,255},night);
    beam({0,0,0},{0,1.55f*size,0},.09f*size,
         shade({102,79,55,255},night));
    DrawCylinderEx({0,.42f*size,0},{0,2.15f*size,0},
                   .76f*size,0,6,green);
}

void oakFar(float size,float night,int color) {
    beam({0,0,0},{0,1.55f*size,0},.14f*size,
         shade({111,80,55,255},night));
    DrawSphereEx({0,2.02f*size,0},.91f*size,5,7,
                 shade(color%2?Color{79,141,60,255}:
                                Color{55,123,58,255},night));
}

void standFar(float night,int variant) {
    const Color concrete=shade({156,159,161,255},night);
    const Color accent=shade(variant%2?Color{43,94,159,255}:
                                      Color{205,63,61,255},night);
    cube(0,.20f,0,9.3f,.36f,4.2f,shade({99,105,107,255},night));
    for(int row=0;row<6;++row) {
        const float z=-1.57f+row*.57f;
        const float y=.46f+row*.29f;
        cube(0,y,z,9.f,.20f,.68f,concrete);
        cube(0,y+.26f,z,8.5f,.24f,.15f,accent);
    }
    cube(0,3.75f,0,10.2f,.18f,5.3f,
         shade({48,57,73,255},night));
    cube(-4.67f,1.52f,0,.15f,2.8f,4.35f,
         shade({104,116,127,255},night));
}

void boulders(float size,float night) {
    Color rock=shade({127,128,125,255},night);
    DrawSphereEx({0,.22f*size,0},.56f*size,4,7,rock);
    DrawSphereEx({.47f*size,.12f*size,.24f*size},.35f*size,4,6,
                 shade(rock,.78f));
    DrawSphereEx({-.37f*size,.09f*size,-.15f*size},.30f*size,4,6,
                 shade(rock,1.14f));
}

void brush(float size,float night) {
    Color leaves=shade({65,120,47,255},night);
    for(float z:{-.32f,0.f,.32f})
        DrawSphereEx({.1f*size,.27f*size,z*size},
                     .43f*size,5,7,shade(leaves,1.f+z*.2f));
}

void canopy(float width,float depth,float base,float peak,Color color) {
    const float hx=width*.5f,hz=depth*.5f;
    const Vector3 a{-hx,base,-hz},b{hx,base,-hz};
    const Vector3 c{hx,base,hz},d{-hx,base,hz};
    const Vector3 e{0,peak,0};
    poly(a,b,e,color);poly(b,c,e,color);
    poly(c,d,e,color);poly(d,a,e,color);
}

void grandstand(float night,int variant) {
    Color metal=shade({104,116,127,255},night);
    Color concrete=shade({156,159,161,255},night);
    cube(0,.20f,0,9.3f,.36f,4.2f,shade({99,105,107,255},night));
    for(int row=0;row<6;++row) {
        const float z=-1.57f+row*.57f;
        const float y=.46f+row*.29f;
        cube(0,y,z,9.0f,.20f,.68f,concrete);
        cube(0,y+.16f,z+.21f,8.8f,.13f,.15f,
             shade(variant%2?Color{43,94,159,255}:Color{205,63,61,255},night));
        for (int i=0;i<20;++i) {
            // Small silhouettes instead of giant coloured blocks.
            const float x=-4.17f+i*.44f;
            const Color shirt[5]{{220,57,50,255},{55,120,196,255},
                                 {245,219,90,255},{225,228,221,255},
                                 {46,159,118,255}};
            cube(x,y+.38f,z,.19f,.29f,.19f,
                 shade(shirt[(i*3+row+variant)%5],night));
            DrawSphere({x,y+.60f,z},.11f,shade({204,165,133,255},night));
        }
    }
    for(float x:{-4.f,4.f})for(float z:{-1.7f,1.65f})
        beam({x,0,z},{x,3.78f,z},.065f,metal);
    cube(0,3.75f,0,10.2f,.18f,5.3f,
         shade({48,57,73,255},night));
    cube(0,3.83f,-2.5f,10.5f,.20f,.12f,
         shade({204,57,53,255},night));
    // Transparent-looking rear facade, stairway side.
    cube(-4.67f,1.52f,0,.15f,2.8f,4.35f,metal);
}

void pitBuilding(float night,int variant) {
    Color wall=shade({196,203,208,255},night);
    Color dark=shade({52,65,80,255},night);
    cube(0,1.35f,0,9.0f,2.7f,4.4f,wall);
    cube(0,2.78f,0,9.55f,.22f,4.85f,dark);
    cube(0,3.43f,-.10f,8.5f,1.03f,3.95f,
         shade({85,114,139,255},night));
    cube(0,4.01f,-.1f,9.f,.15f,4.3f,dark);
    // Bays with roller shutters, structural columns and team colour.
    for(int bay=0;bay<5;++bay) {
        float x=-3.58f+bay*1.79f;
        cube(x,1.12f,2.23f,1.44f,1.86f,.055f,
             shade({64,84,102,255},night));
        for(int slit=0;slit<5;++slit)
            cube(x,.35f+slit*.31f,2.27f,1.35f,.025f,.028f,
                 shade({132,153,165,255},night));
        const Color teams[4]{{214,57,57,255},{42,109,206,255},
                             {225,189,70,255},{43,159,116,255}};
        cube(x,2.49f,2.28f,1.60f,.20f,.07f,
             shade(teams[(bay+variant)%4],night));
    }
    // Glass command tower and rooftop rail.
    for(int i=0;i<6;++i)
        cube(-3.45f+i*1.38f,3.38f,1.91f,1.18f,.76f,.045f,
             shade({35,88,118,255},night));
    cube(0,4.22f,0,9.1f,.045f,4.5f,
         shade({162,170,177,255},night));
}

void paddockTent(float night,int color) {
    Color fabric=shade(color%2?Color{226,223,210,255}:
                         Color{204,58,56,255},night);
    const float w=3.5f,d=3.f;
    cube(0,.09f,0,w,.14f,d,shade({177,170,149,255},night));
    for(float x:{-1.62f,1.62f})for(float z:{-1.36f,1.36f})
        beam({x,0,z},{x,1.67f,z},.045f,
             shade({154,155,152,255},night));
    canopy(w,d,1.62f,2.45f,fabric);
    // Side walls intentionally open to show camping equipment.
    for(float x:{-1.0f,0.f,1.0f})
        cube(x,.40f,0,.65f,.70f,.65f,
             shade({102,112,117,255},night));
    cube(0,.79f,0,2.65f,.10f,1.20f,
         shade({201,192,170,255},night));
}

void sponsor(float night,int variant) {
    const Color accents[4]{{207,55,57,255},{36,111,203,255},
                           {239,179,48,255},{39,151,111,255}};
    Color metal=shade({121,128,135,255},night);
    for(float x:{-1.75f,1.75f})
        beam({x,0,0},{x,2.23f,0},.075f,metal);
    cube(0,2.25f,0,4.35f,1.27f,.14f,shade(accents[variant%4],night));
    cube(0,2.39f,.10f,3.37f,.18f,.07f,shade(WHITE,night));
    cube(-.52f,2.06f,.10f,2.17f,.11f,.07f,shade(WHITE,night));
}

void towerLight(float night) {
    Color metal=shade({134,143,154,255},night);
    beam({0,0,0},{0,5.15f,0},.065f,metal);
    beam({0,5.15f,0},{0,5.15f,.87f},.06f,metal);
    cube(0,5.18f,.9f,.67f,.24f,.36f,
         night>.7f?Color{255,227,153,255}:
                    shade({208,217,220,255},night));
}

void windmill(double seconds,float night) {
    Color metal=shade({214,214,216,255},night);
    DrawCylinderEx({0,0,0},{0,9.2f,0},.21f,.10f,8,metal);
    DrawSphere({0,9.3f,0},.26f,metal);
    for(int i=0;i<3;++i) {
        const float t=float(seconds*.9+i*2.09439510239);
        beam({0,9.3f,.16f},
             {3.0f*std::sin(t),9.3f+3.0f*std::cos(t),.20f},
             .045f,metal);
    }
}

} // namespace

void WorldRenderer::initialize(const Track& track) {
    scenery_.clear();
    std::mt19937 rng(72951);
    std::uniform_real_distribution<float> variation(.72f,1.38f);
    std::uniform_real_distribution<float> wobble(-55.f,55.f);
    const double lap=track.lapLength();
    scenery_.reserve(std::size_t(lap/145.0)*10);

    // All zones and density are specified in world-distance, NOT sample
    // indices. A circuit can gain/remove spline points without erasing forests.
    const auto service=[](double f) {
        return f<.055 || f>.972;
    };
    const auto spectator=[](double f) {
        return (f>.135 && f<.265) ||
               (f>.38 && f<.49) ||
               (f>.72 && f<.83);
    };
    const auto rocky=[](double f) {
        return (f>.29 && f<.38) || (f>.58 && f<.68);
    };
    const auto place=[&](double along,int side,double lateral,
                         int kind,float size,int color) {
        const std::size_t i=track.indexAtDistance(along);
        const Vec2 p=track.point(i,side*lateral);
        if(p.x<120 || p.y<120 ||
           p.x>Track::worldWidth-120 ||
           p.y>Track::worldHeight-120) return;
        const double clearance=(kind==0||kind==1||kind==2||kind==9)?
                               25.0:120.0;
        // Avoid putting a building or a tree on another nearby section
        // of track, particularly between the two arms of the hairpin.
        if(track.info(p).distance<Track::wall+clearance) return;
        const std::size_t n=track.size();
        const Vec2 before=track.center()[(i+n-3)%n];
        const Vec2 after=track.center()[(i+3)%n];
        const float yaw=-std::atan2(float(after.y-before.y),
                                    float(after.x-before.x))*RAD2DEG;
        const Vector3 ground{float(p.x*kWorldScale),
                             groundY(p.x,p.y),float(p.y*kWorldScale)};
        scenery_.push_back({p,size,yaw,kind,color,ground});
    };

    // Roadsides: grandstands cluster around interesting corners,
    // the pits retain the garages and tents, other sectors get sparse
    // technical infrastructure. No index-dependent missing zones.
    for(double s=0;s<lap;s+=570.) {
        const double fraction=s/lap;
        const int section=int(s/570.);
        if(service(fraction)) {
            place(s,1,Track::wall+430,4,1.f,section);
            if(section%2==0)
                place(s+220,1,Track::wall+760,5,1.f,section);
            if(section%3==0)
                place(s,-1,Track::wall+350,3,1.f,section);
            place(s+110,1,Track::wall+140,7,1.f,section);
        } else if(spectator(fraction)) {
            if(section%2==0)
                place(s,-1,Track::wall+370,3,1.f,section);
            if(section%3==0)
                place(s,1,Track::wall+475,3,1.f,section);
            place(s+170,-1,Track::wall+155,6,1.f,section);
            if(section%3==1)
                place(s,1,Track::wall+230,7,1.f,section);
        } else {
            if(section%2==0)
                place(s,(section%4==0)?1:-1,
                      Track::wall+160,6,1.f,section);
            if(section%10==0)
                place(s,1,Track::wall+960,8,1.f,section);
        }
    }

    // Forest density expressed in trees per world-unit of lap.
    // Five staggered rows per side build actual wooded landscapes;
    // service/spectator zones keep enough clear space for buildings.
    for(double s=0;s<lap;s+=160.) {
        const double fraction=s/lap;
        const bool facility=service(fraction)||spectator(fraction);
        for(int side:{-1,1}) {
            for(int row=0;row<5;++row) {
                if(facility && row<2) continue;
                if((rng()%100)<(facility?25:10)) continue;
                const double lateral=Track::wall+115+row*170+wobble(rng);
                const int pick=int(rng()%12);
                const int kind=rocky(fraction)?
                     (pick<5?2:pick<9?1:0):
                     (pick<5?0:pick<10?1:pick==10?2:9);
                const double along=s+double(rng()%90);
                place(along,side,lateral,kind,
                      variation(rng)*(row<2?.83f:1.15f),
                      int(rng()%6));
                if(row>=2 && rng()%4==0)
                    place(along+65,side,lateral+55,9,
                          .75f+variation(rng)*.20f,int(rng()%6));
            }
        }
    }
}

void WorldRenderer::drawTerrain(const Weather& weather,int quality,
                                 Vector3 cameraPosition) const {
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    const double spacing=quality==0?250.0:quality==1?190.0:160.0;
    const int half=quality==0?21:quality==1?28:38;
    const int side=2*half+1;
    const double cx=std::floor(double(cameraPosition.x)/kWorldScale/spacing)*spacing;
    const double cz=std::floor(double(cameraPosition.z)/kWorldScale/spacing)*spacing;
    const float damp=.99f-.27f*night-.04f*rain;

    // Cache height once per grid vertex, rather than four times per quad.
    // Keep the old geometry and colours: only submission is optimized.
    std::vector<Vector3> vertices(std::size_t(side)*std::size_t(side));
    for(int i=0;i<side;++i)for(int j=0;j<side;++j) {
        const double x=cx+(i-half)*spacing;
        const double z=cz+(j-half)*spacing;
        vertices[std::size_t(i)*side+j]=
            {float(x*kWorldScale),groundY(x,z),float(z*kWorldScale)};
    }
    const auto emit=[](Vector3 v) {
        rlVertex3f(v.x,v.y,v.z);
    };
    const auto triangle=[&](Vector3 a,Vector3 b,Vector3 c) {
        emit(a);emit(b);emit(c);
        emit(c);emit(b);emit(a);
    };

    // Old code issued up to 12,000+ separate DrawTriangle3D calls
    // per frame. One rlgl immediate-mode batch removes that CPU overhead.
    // rlgl automatically flushes when its internal vertex buffer fills.
    rlBegin(RL_TRIANGLES);
    for(int i=0;i<2*half;++i)for(int j=0;j<2*half;++j) {
        const double x=cx+(i-half)*spacing,z=cz+(j-half)*spacing;
        const Vector3 a=vertices[std::size_t(i)*side+j];
        const Vector3 b=vertices[std::size_t(i+1)*side+j];
        const Vector3 c=vertices[std::size_t(i+1)*side+j+1];
        const Vector3 d=vertices[std::size_t(i)*side+j+1];
        const float noise=seedNoise(float(x+spacing*.5),float(z+spacing*.5));
        const Color grass=noise>.24f?Color{99,141,66,255}:
                          noise<-.23f?Color{63,128,60,255}:
                                      Color{67,144,63,255};
        const Color color=shade(grass,damp);
        rlColor4ub(color.r,color.g,color.b,color.a);
        triangle(a,b,c);
        triangle(a,c,d);
    }
    rlEnd();
}

void WorldRenderer::drawScenery(const Weather& weather,int quality,
                                 Vector3 cameraPosition,double time) const {
    const float darkness=1.f-.48f*float(weather.nightLevel);
    const float distance=quality==0?55.f:quality==1?82.f:120.f;
    const float range2=distance*distance;
    for(const SceneryProp& obj:scenery_) {
        const Vector3 origin=obj.groundPosition;
        const float dx=origin.x-cameraPosition.x,dz=origin.z-cameraPosition.z;
        const float d2=dx*dx+dz*dz;
        if(d2>range2)continue;
        if(quality==0 && (obj.kind==3||obj.kind==4||obj.kind==8)
           &&d2>38.f*38.f)continue;
        rlPushMatrix();
        // Terrain contact: props' bases are 0.19m below the former road level.
        rlTranslatef(origin.x,origin.y,origin.z);
        rlRotatef(obj.yaw,0,1,0);
        const float scale=obj.size;
        const bool distant=d2>(obj.kind==3?27.f:obj.kind==4?48.f:32.f)*
                              (obj.kind==3?27.f:obj.kind==4?48.f:32.f);
        switch(obj.kind) {
        case 0: if(distant) oakFar(scale,darkness,obj.color);
                else oak(scale,darkness,obj.color);break;
        case 1: if(distant) cedarFar(scale,darkness,obj.color);
                else cedar(scale,darkness,obj.color);break;
        case 2: boulders(scale,darkness);break;
        case 3: if(distant) standFar(darkness,obj.color);
                else grandstand(darkness,obj.color);break;
        case 4: pitBuilding(darkness,obj.color);break;
        case 5: paddockTent(darkness,obj.color);break;
        case 6: sponsor(darkness,obj.color);break;
        case 7: towerLight(float(weather.nightLevel));break;
        case 8: windmill(time,darkness);break;
        default: brush(scale,darkness);break;
        }
        rlPopMatrix();
    }
}

} // namespace karting
