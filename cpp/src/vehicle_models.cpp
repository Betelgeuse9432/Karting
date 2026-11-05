#include "karting/game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace karting {
namespace {

// The front of every model is +X. Coordinates are in renderer metres.
// All bodies are lightweight procedural geometry; no external meshes.
Color tone(Color c,float k) {
    const auto channel=[k](unsigned char v) {
        return static_cast<unsigned char>(std::clamp(float(v)*k,0.f,255.f));
    };
    return {channel(c.r),channel(c.g),channel(c.b),c.a};
}

void box(float x,float y,float z,float sx,float sy,float sz,Color color) {
    DrawCubeV({x,y,z},{sx,sy,sz},color);
}

void rod(Vector3 a,Vector3 b,float width,Color color) {
    DrawCylinderEx(a,b,width,width,7,color);
}

void panel(Vector3 a,Vector3 b,Vector3 c,Vector3 d,Color color) {
    // Double sided so that front and rear chase cameras agree on visibility.
    DrawTriangle3D(a,b,c,color);
    DrawTriangle3D(a,c,d,color);
    DrawTriangle3D(c,b,a,color);
    DrawTriangle3D(d,c,a,color);
}

struct Section {
    float x;
    float bottom;
    float top;
    float halfWidth;
};

void bodyShell(std::initializer_list<Section> points,Color top,Color sides) {
    if (points.size()<2) return;
    auto it=points.begin();
    auto previous=*it++;
    for(;it!=points.end();++it) {
        const auto current=*it;
        panel({previous.x,previous.top,-previous.halfWidth},
              {current.x,current.top,-current.halfWidth},
              {current.x,current.top,current.halfWidth},
              {previous.x,previous.top,previous.halfWidth},top);
        panel({previous.x,previous.bottom,-previous.halfWidth},
              {current.x,current.bottom,-current.halfWidth},
              {current.x,current.top,-current.halfWidth},
              {previous.x,previous.top,-previous.halfWidth},sides);
        panel({previous.x,previous.top,previous.halfWidth},
              {current.x,current.top,current.halfWidth},
              {current.x,current.bottom,current.halfWidth},
              {previous.x,previous.bottom,previous.halfWidth},sides);
        previous=current;
    }
    const auto a=*points.begin(),b=previous;
    panel({a.x,a.bottom,-a.halfWidth},{a.x,a.bottom,a.halfWidth},
          {a.x,a.top,a.halfWidth},{a.x,a.top,-a.halfWidth},sides);
    panel({b.x,b.bottom,b.halfWidth},{b.x,b.bottom,-b.halfWidth},
          {b.x,b.top,-b.halfWidth},{b.x,b.top,b.halfWidth},sides);
}

void cockpit(float front,float rear,float roofFront,float roofRear,
             float bottom,float roof,float halfWidth,float roofHalfWidth,
             Color window,Color roofColor) {
    const Vector3 fl{front,bottom,-halfWidth},fr{front,bottom,halfWidth};
    const Vector3 bl{rear,bottom,-halfWidth},br{rear,bottom,halfWidth};
    const Vector3 ftl{roofFront,roof,-roofHalfWidth};
    const Vector3 ftr{roofFront,roof,roofHalfWidth};
    const Vector3 btl{roofRear,roof,-roofHalfWidth};
    const Vector3 btr{roofRear,roof,roofHalfWidth};
    panel(fl,fr,ftr,ftl,tone(window,1.16f));
    panel(br,bl,btl,btr,tone(window,.75f));
    panel(bl,fl,ftl,btl,window);
    panel(fr,br,btr,ftr,window);
    panel(btl,ftl,ftr,btr,roofColor);
    for (float side:{-1.f,1.f}) {
        rod({front,bottom,side*halfWidth},
            {roofFront,roof,side*roofHalfWidth},.012f,roofColor);
        rod({rear,bottom,side*halfWidth},
            {roofRear,roof,side*roofHalfWidth},.012f,roofColor);
    }
}

// Restrained rear wing: almost a line from behind, not a black fence.
void rearWing(float x,float y,float width,float chord,Color color) {
    box(x,y,0,chord,.027f,width,color);
    box(x-chord*.33f,y+.018f,0,chord*.20f,.012f,width*.92f,
        tone(color,1.25f));
    for(float side:{-1.f,1.f})
        box(x,y,side*width*.5f,chord+.015f,.055f,.019f,tone(color,.82f));
}

void lamps(float x,float height,float side,Color color,float width=.14f) {
    for(float z:{-side,side})
        box(x,height,z,.045f,.065f,width,color);
}

void kart(Color paint) {
    const Color steel{67,72,78,255},rubber{35,37,44,255};
    bodyShell({{-.77f,.12f,.18f,.25f},
               {-.37f,.12f,.21f,.34f},
               { .26f,.11f,.19f,.30f},
               { .75f,.10f,.13f,.17f}},paint,tone(paint,.74f));
    for(float side:{-1.f,1.f}) {
        box(-.04f,.17f,side*.36f,.48f,.11f,.15f,paint);
        rod({-.76f,.14f,side*.33f},{.69f,.14f,side*.36f},
            .018f,steel);
    }
    box(-.17f,.35f,0,.32f,.30f,.29f,rubber); // seat
    box(-.34f,.49f,0,.10f,.34f,.30f,rubber); // backrest
    DrawSphere({-.16f,.76f,0},.18f,{238,187,54,255}); // driver
    box(-.02f,.74f,.11f,.14f,.055f,.05f,{39,61,87,255}); // visor
    rod({.15f,.22f,0},{.15f,.43f,0},.02f,steel);
    rod({.15f,.43f,-.11f},{.15f,.43f,.11f},.027f,rubber);
    for(float side:{-1.f,1.f})
        rod({-.13f,.49f,side*.12f},{.16f,.37f,side*.09f},
            .042f,{56,67,79,255});
    box(-.59f,.29f,0,.28f,.24f,.25f,{114,115,119,255}); // engine
    for(int i=0;i<4;i++)
        box(-.69f+.059f*i,.38f,0,.017f,.20f,.29f,steel);
    rod({-.68f,.25f,.17f},{-.94f,.35f,.21f},.034f,
        {179,179,174,255}); // exhaust
    box(.76f,.14f,0,.068f,.10f,.76f,rubber); // bumper
    box(-.77f,.14f,0,.068f,.10f,.73f,rubber);
}

void offRoad(Color paint,Color glass,Color dark,bool braking) {
    // A boxy SUV is intentional, but the front hood and rear body taper.
    bodyShell({{-1.10f,.36f,.76f,.45f},
               {-.73f,.35f,.88f,.55f},
               { .36f,.35f,.83f,.53f},
               { .79f,.34f,.78f,.52f},
               {1.13f,.34f,.66f,.45f}},
              paint,tone(paint,.74f));
    cockpit(.27f,-.86f,.06f,-.73f,.86f,1.47f,.47f,.40f,
            glass,tone(paint,.86f));
    for(float side:{-1.f,1.f}) {
        for(float x:{-.63f,.57f})
            box(x,.54f,side*.53f,.43f,.15f,.15f,dark); // arches
        box(-.12f,.36f,side*.54f,1.65f,.13f,.09f,dark); // running boards
        box(.43f,1.14f,side*.52f,.12f,.12f,.08f,dark); // mirror
        rod({-.82f,1.55f,side*.36f},
            {.41f,1.55f,side*.36f},.028f,dark); // roof rack
        box(.12f,.77f,side*.56f,.16f,.035f,.05f,
            {175,179,179,255}); // handle
    }
    box(1.14f,.43f,0,.16f,.19f,1.02f,dark);
    box(1.23f,.60f,0,.032f,.24f,.61f,{45,51,59,255}); // grille
    lamps(1.22f,.69f,.36f,{228,231,190,255},.17f);
    lamps(-1.13f,.70f,.36f,braking?RED:Color{139,30,32,255},.16f);
    rod({-1.16f,.87f,-.10f},{-1.28f,.87f,-.10f},
        .30f,{40,42,46,255}); // spare
    rod({-1.30f,.87f,-.10f},{-1.32f,.87f,-.10f},
        .13f,{163,166,172,255});
}

void gt3(Color paint,Color glass,Color dark,bool braking) {
    // Coupe with a distinctly long front hood and a low sloping cabin.
    bodyShell({{-1.11f,.14f,.39f,.42f},
               {-.80f,.15f,.50f,.53f},
               {-.22f,.15f,.51f,.50f},
               { .55f,.14f,.38f,.47f},
               {1.13f,.11f,.21f,.31f}},paint,tone(paint,.72f));
    cockpit(.34f,-.79f,-.01f,-.54f,.50f,.88f,.38f,.27f,
            glass,tone(paint,.79f));
    for(float side:{-1.f,1.f}) {
        const float z=side*.50f;
        box(-.13f,.18f,z,.80f,.085f,.12f,dark); // skirts
        box(-.74f,.51f,side*.40f,.47f,.07f,.19f,
            tone(paint,.90f)); // haunches
        box(-.57f,.33f,side*.52f,.28f,.053f,.016f,
            {36,43,50,255}); // side intake
        box(.37f,.78f,side*.34f,.10f,.065f,.07f,dark); // mirrors
        box(-1.10f,.36f,side*.34f,.04f,.07f,.14f,
            braking?RED:Color{139,25,29,255});
        box(.99f,.31f,side*.30f,.09f,.057f,.17f,
            {230,236,206,255});
        rod({-1.01f,.45f,side*.30f},
            {-1.01f,.61f,side*.30f},.018f,dark);
    }
    box(1.14f,.13f,0,.21f,.06f,1.01f,dark); // splitter
    box(-1.12f,.13f,0,.18f,.08f,.92f,dark); // diffuser
    rearWing(-1.08f,.64f,.97f,.14f,dark);
    for(float z:{-.23f,0.f,.23f})
        box(.72f,.37f,z,.21f,.018f,.045f,dark); // hood vents
}

void wec(Color paint,Color glass,Color dark,bool braking) {
    // Long, very low prototype. The rear wing must NOT eclipse the road.
    bodyShell({{-1.43f,.10f,.24f,.36f},
               {-1.09f,.11f,.32f,.45f},
               {-.52f,.12f,.34f,.41f},
               { .15f,.10f,.29f,.39f},
               { .92f,.09f,.20f,.36f},
               {1.46f,.075f,.11f,.23f}},paint,tone(paint,.72f));
    // Independent raised shoulders make the silhouette read as a prototype.
    for(float side:{-1.f,1.f}) {
        rlPushMatrix();
        rlTranslatef(0,0,side*.37f);
        bodyShell({{-1.29f,.11f,.31f,.16f},
                   {-.94f,.11f,.40f,.18f},
                   {-.31f,.11f,.26f,.16f},
                   {.64f,.09f,.25f,.15f},
                   {1.31f,.08f,.13f,.08f}},
                  tone(paint,1.06f),tone(paint,.78f));
        rlPopMatrix();
        box(-.78f,.30f,side*.44f,.40f,.045f,.14f,dark);
        box(-.64f,.15f,side*.44f,.68f,.07f,.12f,dark);
        box(1.40f,.18f,side*.30f,.055f,.054f,.20f,
            {237,237,209,255});
        box(-1.42f,.25f,side*.31f,.06f,.048f,.18f,
            braking?RED:Color{126,21,26,255});
        rod({-1.37f,.30f,side*.33f},
            {-1.37f,.49f,side*.33f},.018f,dark);
    }
    cockpit(.29f,-.77f,-.07f,-.56f,.37f,.75f,.29f,.18f,
            glass,tone(paint,.78f));
    // Shallow dorsal fin, with no full-height vertical panel.
    panel({-.91f,.43f,-.014f},{-.20f,.44f,-.014f},
          {-.60f,.63f,-.014f},{-.90f,.57f,-.014f},dark);
    panel({-.91f,.43f,.014f},{-.20f,.44f,.014f},
          {-.60f,.63f,.014f},{-.90f,.57f,.014f},dark);
    rearWing(-1.43f,.53f,.95f,.13f,dark);
    box(1.45f,.10f,0,.22f,.05f,.95f,dark); // floor
    box(-1.44f,.11f,0,.18f,.07f,.89f,dark);
}

void wheels(const Car& car,const VehicleProfile& p,int index) {
    const float front=float(p.frontAxle*kWorldScale);
    const float rear=float(p.rearAxle*kWorldScale);
    constexpr std::array<float,4> sizes{{.155f,.265f,.205f,.195f}};
    constexpr std::array<float,4> widths{{.19f,.26f,.22f,.21f}};
    constexpr std::array<float,4> tracks{{.45f,.65f,.60f,.58f}};
    const std::size_t slot=std::size_t(std::clamp(index,0,3));
    const float r=sizes[slot],width=widths[slot];
    const float track=std::max(float(p.halfWheelTrack*kWorldScale),tracks[slot]);

    for(float x:{rear,front}) for(float side:{-1.f,1.f}) {
        rlPushMatrix();
        rlTranslatef(x,r+.015f,side*track);
        if(x==front) rlRotatef(float(-car.steering*RAD2DEG),0,1,0);
        DrawCylinderEx({0,0,-width*.5f},{0,0,width*.5f},r,r,12,
                       {27,30,36,255});
        const float z=side*(width*.5f+.008f);
        rod({0,0,z},{0,0,z+side*.017f},
            r*.52f,{90,96,107,255});
        rod({0,0,z+side*.020f},{0,0,z+side*.024f},
            r*.17f,{188,191,199,255});
        // The wheel rim rotates using the real wheel rotation state.
        for(int spoke=0;spoke<5;++spoke) {
            const float a=float(car.wheelRotation)+spoke*1.2566371f;
            DrawLine3D({std::cos(a)*r*.2f,std::sin(a)*r*.2f,z+side*.028f},
                       {std::cos(a)*r*.48f,std::sin(a)*r*.48f,z+side*.028f},
                       {180,185,193,255});
        }
        rlPopMatrix();
    }
}

} // namespace

void VehicleRenderer::draw(const Car& car,const VehicleProfile& profile,
                           int index,int colorIndex,bool night) const {
    const Vector3 p=worldPoint(car.position);
    const Color paint=palette(colorIndex);
    const Color dark{31,34,42,255};
    const Color glass=night?Color{29,48,70,255}:Color{58,104,139,255};
    rlPushMatrix();
    rlTranslatef(p.x,p.y,p.z);
    rlRotatef(float(-car.heading*RAD2DEG),0,1,0);
    rlPushMatrix();
    rlRotatef(float(car.roll*RAD2DEG),1,0,0);
    rlRotatef(float(car.pitch*RAD2DEG),0,0,1);
    switch(index) {
    case 0: kart(paint);break;
    case 1: offRoad(paint,glass,dark,car.brake>.1);break;
    case 2: gt3(paint,glass,dark,car.brake>.1);break;
    case 3: wec(paint,glass,dark,car.brake>.1);break;
    default: gt3(paint,glass,dark,car.brake>.1);break;
    }
    rlPopMatrix();
    wheels(car,profile,index);
    rlPopMatrix();
}

} // namespace karting
