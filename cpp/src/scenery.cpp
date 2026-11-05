#include "karting/game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace karting {
namespace {

Color dim(Color c,float factor) {
    const auto v=[factor](unsigned char channel) -> unsigned char {
        return static_cast<unsigned char>(
            std::clamp(float(channel)*factor,0.f,255.f));
    };
    return {v(c.r),v(c.g),v(c.b),c.a};
}

void cube(float x,float y,float z,float w,float h,float d,Color color) {
    DrawCubeV({x,y,z},{w,h,d},color);
}

void beam(Vector3 a,Vector3 b,float radius,Color c) {
    DrawCylinderEx(a,b,radius,radius,6,c);
}

void roof(Color c,float width,float length,float y) {
    const float x=length*.5f,z=width*.5f;
    // Two sloped roof faces.
    DrawTriangle3D({-x,y,-z},{x,y,-z},{0,y+.55f,0},c);
    DrawTriangle3D({x,y,-z},{x,y,z},{0,y+.55f,0},c);
    DrawTriangle3D({x,y,z},{-x,y,z},{0,y+.55f,0},c);
    DrawTriangle3D({-x,y,z},{-x,y,-z},{0,y+.55f,0},c);
}

void pine(float size,float darkness) {
    const Color bark=dim({106,77,51,255},darkness);
    const Color green=dim({45,110,57,255},darkness);
    beam({0,0,0},{0,.78f*size,0},.10f*size,bark);
    DrawCylinderEx({0,.45f*size,0},{0,1.75f*size,0},
                   .61f*size,0,7,green);
    DrawCylinderEx({0,1.18f*size,0},{0,2.2f*size,0},
                   .41f*size,0,7,dim(green,1.12f));
}

void broadleaf(float size,float darkness) {
    beam({0,0,0},{0,1.05f*size,0},.12f*size,
         dim({102,75,54,255},darkness));
    DrawSphereEx({0,1.41f*size,0},.66f*size,6,9,
                 dim({62,138,59,255},darkness));
    DrawSphereEx({-.28f*size,1.71f*size,0},.49f*size,5,8,
                 dim({85,158,65,255},darkness));
}

void rocks(float size,float darkness) {
    DrawSphereEx({0,.22f*size,0},.47f*size,4,7,
                 dim({121,126,130,255},darkness));
    DrawSphereEx({.44f*size,.15f*size,.24f*size},.31f*size,4,6,
                 dim({103,110,116,255},darkness));
    DrawSphereEx({-.32f*size,.12f*size,-.18f*size},.25f*size,4,6,
                 dim({141,143,135,255},darkness));
}

void stand(float scale,float darkness) {
    const Color steel=dim({113,119,127,255},darkness);
    const Color shade=dim({43,50,62,255},darkness);
    for(int row=0;row<4;++row) {
        const float h=.35f+row*.30f;
        cube(0,h,-.65f+row*.32f,4.2f,.12f,.65f,steel);
        for(int i=0;i<9;++i) {
            const int id=(i+row*3)%4;
            const Color shirts[4]{{220,65,57,255},{226,206,80,255},
                                  {53,120,198,255},{235,237,232,255}};
            cube(-1.7f+i*.42f,h+.18f,-.65f+row*.32f,
                 .20f,.30f,.21f,dim(shirts[id],darkness));
        }
    }
    cube(0,1.80f,-.07f,4.9f,.12f,2.4f,shade);
    for(float x:{-2.f,2.f})for(float z:{-.9f,.75f})
        beam({x,0,z},{x,1.8f,z},.04f,steel);
    (void)scale;
}

void paddock(float darkness) {
    const Color wall=dim({200,207,205,255},darkness);
    cube(0,.85f,0,3.8f,1.7f,2.4f,wall);
    cube(0,1.8f,0,4.3f,.22f,2.7f,dim({58,65,77,255},darkness));
    for(float x:{-1.18f,0.f,1.18f}) {
        cube(x,.78f,1.23f,.88f,1.12f,.045f,
             dim({59,82,103,255},darkness));
        cube(x,1.38f,1.26f,.90f,.11f,.07f,
             dim({87,116,155,255},darkness));
    }
    cube(0,1.84f,1.35f,2.5f,.33f,.12f,
         dim({217,73,57,255},darkness));
}

void tent(float darkness) {
    cube(0,.17f,0,2.5f,.18f,2.2f,dim({190,185,160,255},darkness));
    roof(dim({230,218,174,255},darkness),2.4f,2.6f,1.20f);
    for(float x:{-1.0f,1.0f})for(float z:{-1.0f,1.0f})
        beam({x,.18f,z},{x,1.25f,z},.035f,
             dim({122,128,121,255},darkness));
}

void billboard(int color,float darkness) {
    const Color backgrounds[4]{{218,62,58,255},{38,95,195,255},
                               {230,187,55,255},{36,149,116,255}};
    const Color c=dim(backgrounds[color%4],darkness);
    for(float x:{-1.4f,1.4f})
        beam({x,0,0},{x,2.5f,0},.09f,
             dim({127,137,145,255},darkness));
    cube(0,2.5f,0,3.6f,1.35f,.16f,c);
    cube(0,2.55f,.10f,2.4f,.13f,.04f,dim(WHITE,darkness));
    cube(0,2.24f,.10f,1.35f,.09f,.04f,dim(WHITE,darkness));
}

void turbine(float time,float darkness) {
    const Color steel=dim({206,209,214,255},darkness);
    beam({0,0,0},{0,5.2f,0},.095f,steel);
    DrawSphere({0,5.3f,0},.18f,dim({180,188,202,255},darkness));
    for(int i=0;i<3;++i) {
        const float angle=time*1.5f+i*2.0943951f;
        beam({0,5.3f,0},
             {0,5.3f+2.0f*std::cos(angle),2.f*std::sin(angle)},
             .037f,steel);
    }
}

void lighting(bool night,float darkness) {
    const Color post=dim({132,137,145,255},darkness);
    beam({0,0,0},{0,4.4f,0},.054f,post);
    beam({0,4.4f,0},{0,4.5f,.72f},.045f,post);
    DrawSphere({0,4.5f,.72f},.23f,
               night?Color{255,231,156,255}:dim(LIGHTGRAY,darkness));
}

void shrub(float size,float darkness) {
    for(float z:{-.32f,0.f,.32f})
        DrawSphereEx({0,.25f*size,z*size},.42f*size,4,7,
                     dim({61,124,55,255},darkness));
}

} // namespace

void WorldRenderer::initialize(const Track& track) {
    scenery_.clear();
    // Reproducible placement: no procedural random work in the render loop.
    std::mt19937 rng(9267);
    std::uniform_real_distribution<float> scale(.68f,1.38f);
    std::uniform_real_distribution<float> offset(175.f,1100.f);
    std::uniform_real_distribution<float> facing(-3.14f,3.14f);
    std::uniform_int_distribution<int> vegetation(0,9);

    for(std::size_t i=0;i<track.size();i+=9) {
        for(int side:{-1,1}) {
            const Vec2 p=track.point(i,side*(Track::wall+offset(rng)));
            if (p.x<300 || p.y<300 ||
                p.x>Track::worldWidth-300 ||
                p.y>Track::worldHeight-300) continue;
            const int sample=vegetation(rng);
            int kind=sample<3?0:sample<5?1:sample<7?2:9;
            if(i%81==0) kind=3;     // grandstands
            else if(i%117==0) kind=4; // paddock / village
            else if(i%63==0) kind=6;  // sponsors
            else if(i%144==0) kind=7; // wind turbines
            else if(i%45==0) kind=8;  // lights
            else if(i%72==0) kind=5;  // tents
            scenery_.push_back({p,scale(rng),facing(rng),kind,
                                int((i/9)%6)});
        }
    }

    // Backdrop vegetation, sparse enough to maintain GPU headroom.
    std::uniform_real_distribution<double> x(200,Track::worldWidth-200);
    std::uniform_real_distribution<double> y(200,Track::worldHeight-200);
    for(int i=0;i<650;++i) {
        const Vec2 p{x(rng),y(rng)};
        if(track.info(p).distance<Track::wall+180) continue;
        const int kind=vegetation(rng)%4;
        scenery_.push_back({p,scale(rng)*1.35f,facing(rng),
                            kind==3?9:kind,int(i%6)});
    }
}

void WorldRenderer::drawScenery(const Weather& weather,int quality,
                                 Vector3 cameraPosition,double time) const {
    const float night=float(weather.nightLevel);
    const float dark=1.f-.48f*night;
    const float range=quality==0?31.f:quality==1?49.f:73.f;
    const float range2=range*range;
    for(const SceneryProp& prop:scenery_) {
        const Vector3 pos=worldPoint(prop.position);
        const float dx=pos.x-cameraPosition.x,dz=pos.z-cameraPosition.z;
        const float dist2=dx*dx+dz*dz;
        if(dist2>range2) continue;
        if(quality==0 && (prop.kind==3||prop.kind==4||
                          prop.kind==5||prop.kind==7)) continue;
        rlPushMatrix();
        rlTranslatef(pos.x,pos.y,pos.z);
        rlRotatef(prop.yaw*RAD2DEG,0,1,0);
        const float size=prop.size;
        switch(prop.kind) {
        case 0: broadleaf(size,dark);break;
        case 1: pine(size,dark);break;
        case 2: rocks(size,dark);break;
        case 3: rlScalef(size,size,size);stand(size,dark);break;
        case 4: rlScalef(size,size,size);paddock(dark);break;
        case 5: rlScalef(size,size,size);tent(dark);break;
        case 6: rlScalef(size,size,size);billboard(prop.color,dark);break;
        case 7: rlScalef(size,size,size);turbine(float(time),dark);break;
        case 8: rlScalef(size,size,size);lighting(night>.3f,dark);break;
        default: shrub(size,dark);break;
        }
        rlPopMatrix();
    }
}

} // namespace karting
