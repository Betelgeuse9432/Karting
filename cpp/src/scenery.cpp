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
    std::uniform_real_distribution<float> wobble(-95.f,95.f);
    std::uniform_real_distribution<float> facing(-180.f,180.f);

    const auto add=[&](std::size_t i,int side,double distance,int kind,
                       float size,int variant) {
        const auto n=track.size();
        const Vec2 p=track.point(i%n,side*distance);
        if(p.x<100||p.y<100||p.x>Track::worldWidth-100||
           p.y>Track::worldHeight-100) return;
        const Vec2 before=track.center()[(i+n-3)%n];
        const Vec2 after=track.center()[(i+3)%n];
        const float yaw=-std::atan2(float(after.y-before.y),
                                    float(after.x-before.x))*RAD2DEG;
        scenery_.push_back({p,size,yaw,kind,variant});
    };

    const std::size_t n=track.size();
    // Infrastructure in contiguous zones rather than sprinkling garages
    // randomly among forests. The pit sector is around the start/finish.
    for(std::size_t i=0;i<n;++i) {
        const bool service=(i<85||i>n-50);
        const bool spectator=(i>=100&&i<315)||(i>=1120&&i<1280);
        if(service) {
            if(i%26==0) add(i,1,560,4,1.f,int(i/26));
            if(i%31==0) add(i,1,860,5,1.f,int(i/31));
            if(i%45==0) add(i,-1,650,3,1.f,int(i/45));
            if(i%20==0) add(i,1,320,7,1.f,0);
        } else if(spectator) {
            if(i%34==0) add(i,-1,550,3,1.f,int(i/34));
            if(i%47==0) add(i,1,650,3,1.f,int(i/47));
            if(i%37==0) add(i,-1,290,6,1.f,int(i/37));
            if(i%45==0) add(i,1,280,7,1.f,0);
        } else {
            if(i%90==0) add(i,(i/90)%2?1:-1,650,8,1.f,0);
            if(i%35==0) add(i,(i/35)%2?1:-1,325,6,1.f,int(i/35));
        }
    }

    // Vegetation is clustered along actual sectors, with a clear verge and
    // multiple rows of trees. No random isolated structures on the horizon.
    for(std::size_t i=0;i<n;i+=3) {
        const bool facilities=(i<115||i>n-55||
                               (i>=100&&i<315)||(i>=1120&&i<1280));
        for(int side:{-1,1}) {
            for(int row=0;row<4;++row) {
                if(facilities&&row<3) continue;
                if((rng()%100)<(facilities?42:13)) continue;
                const double distance=Track::wall+160+row*180+wobble(rng);
                int kind=int(rng()%8)<5?int(rng()%2):2;
                add(i+std::size_t(rng()%3),side,distance,kind,
                    variation(rng)*(row==0?.85f:1.18f),int(rng()%6));
                if(row>0&&(rng()%5==0))
                    add(i+1,side,distance+70,9,.8f,int(rng()%6));
            }
        }
    }
    // Distinct rocky areas in the central and southern bends.
    for(std::size_t i=420;i<650&&i<n;i+=4)
        add(i,-1,510+int(i%5)*90,2,1.1f,int(i%6));
}

void WorldRenderer::drawTerrain(const Weather& weather,int quality,
                                 Vector3 cameraPosition) const {
    const float rain=float(weather.rainLevel);
    const float night=float(weather.nightLevel);
    // Camera-centred terrain follows the SAME elevation as the circuit.
    // Snapping avoids sub-frame movement of the mesh under the vehicle.
    const double spacing=quality==0?250.0:quality==1?190.0:160.0;
    const int half=quality==0?21:quality==1?28:38;
    const double cx=std::floor(double(cameraPosition.x)/kWorldScale/spacing)*spacing;
    const double cz=std::floor(double(cameraPosition.z)/kWorldScale/spacing)*spacing;
    const auto vertex=[&](double x,double z) {
        return Vector3{float(x*kWorldScale),groundY(x,z),
                       float(z*kWorldScale)};
    };
    const auto color=[&](double x,double z) {
        const float noise=seedNoise(float(x),float(z));
        const float damp=.99f-.27f*night-.04f*rain;
        Color c=noise>.24f?Color{99,141,66,255}:
                noise<-.23f?Color{63,128,60,255}:
                            Color{67,144,63,255};
        return shade(c,damp);
    };
    for(int i=-half;i<half;++i)for(int j=-half;j<half;++j) {
        const double x=cx+i*spacing,z=cz+j*spacing;
        const auto a=vertex(x,z),b=vertex(x+spacing,z);
        const auto c=vertex(x+spacing,z+spacing);
        const auto d=vertex(x,z+spacing);
        const Color first=color(x+spacing*.25,z+spacing*.25);
        const Color second=color(x+spacing*.75,z+spacing*.75);
        poly(a,b,c,first);
        poly(a,c,d,second);
    }
}

void WorldRenderer::drawScenery(const Weather& weather,int quality,
                                 Vector3 cameraPosition,double time) const {
    const float darkness=1.f-.48f*float(weather.nightLevel);
    const float distance=quality==0?55.f:quality==1?82.f:120.f;
    const float range2=distance*distance;
    for(const SceneryProp& obj:scenery_) {
        const Vector3 origin=worldPoint(obj.position);
        const float dx=origin.x-cameraPosition.x,dz=origin.z-cameraPosition.z;
        const float d2=dx*dx+dz*dz;
        if(d2>range2)continue;
        if(quality==0 && (obj.kind==3||obj.kind==4||obj.kind==8)
           &&d2>38.f*38.f)continue;
        rlPushMatrix();
        // Terrain contact: props' bases are 0.19m below the former road level.
        rlTranslatef(origin.x,groundY(obj.position.x,obj.position.y),
                     origin.z);
        rlRotatef(obj.yaw,0,1,0);
        const float scale=obj.size;
        switch(obj.kind) {
        case 0: oak(scale,darkness,obj.color);break;
        case 1: cedar(scale,darkness,obj.color);break;
        case 2: boulders(scale,darkness);break;
        case 3: grandstand(darkness,obj.color);break;
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
