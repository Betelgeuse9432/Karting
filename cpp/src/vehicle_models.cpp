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

void ellipsoid(float x,float y,float z,float sx,float sy,float sz,Color color) {
    rlPushMatrix();
    rlTranslatef(x,y,z);
    rlScalef(sx,sy,sz);
    DrawSphere({0,0,0},1.0f,color);
    rlPopMatrix();
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
    // Porsche 963-inspired Hypercar proportions. Front is +X, rear is -X.
    // This is a procedural model: all geometry stays independent from physics.
    const Color carbon{22,25,31,255};
    const Color carbonHi{43,48,57,255};
    const Color pearl{232,235,229,255};
    const Color accent{38,190,207,255};
    const Color lamp{224,246,247,255};
    const Color tail=braking?Color{255,35,28,255}:Color{128,20,27,255};

    // Main monocoque: low nose, broad front shoulders, pinched waist,
    // high rear haunches and a tapered tail.
    bodyShell({{-1.52f,.075f,.20f,.31f},
               {-1.24f,.085f,.27f,.43f},
               {-.82f,.095f,.31f,.46f},
               {-.40f,.090f,.29f,.42f},
               { .18f,.085f,.25f,.39f},
               { .78f,.075f,.20f,.35f},
               {1.23f,.065f,.145f,.29f},
               {1.53f,.055f,.095f,.20f}},
              paint,tone(paint,.70f));

    // Separate sculpted fenders create the wide 963 shoulders.
    for(float side:{-1.f,1.f}) {
        rlPushMatrix();
        rlTranslatef(0,0,side*.34f);
        bodyShell({{-1.30f,.09f,.29f,.12f},
                   {-.98f,.10f,.43f,.19f},
                   {-.70f,.10f,.40f,.20f},
                   {-.38f,.09f,.28f,.17f},
                   { .45f,.08f,.25f,.15f},
                   { .91f,.07f,.22f,.12f},
                   {1.31f,.06f,.13f,.07f}},
                  tone(paint,1.08f),tone(paint,.76f));
        rlPopMatrix();

        // Sidepod lower edge and carbon floor.
        box(-.12f,.115f,side*.405f,.98f,.075f,.105f,carbon);
        box(-.17f,.205f,side*.447f,.70f,.10f,.018f,
            tone(paint,.82f));

        // Deep sidepod cooling inlet, with a bright leading lip.
        box(-.43f,.285f,side*.456f,.34f,.105f,.018f,carbon);
        box(-.43f,.337f,side*.468f,.29f,.014f,.012f,accent);
        for(int vent=0;vent<4;++vent)
            box(-.54f+vent*.067f,.286f,side*.470f,
                .018f,.068f,.012f,carbonHi);

        // Front wheel arch crown and rear haunch highlight.
        box(.79f,.205f,side*.355f,.47f,.045f,.13f,
            tone(paint,1.13f));
        box(-.88f,.205f,side*.395f,.43f,.045f,.13f,
            tone(paint,1.10f));

        // Mirrors on slim stalks.
        rod({.18f,.43f,side*.29f},{.29f,.49f,side*.49f},
            .014f,carbon);
        box(.31f,.50f,side*.50f,.16f,.075f,.105f,carbon);
        box(.33f,.52f,side*.552f,.075f,.018f,.012f,accent);

        // Front corner dive planes and small canards.
        panel({1.05f,.19f,side*.34f},
              {1.38f,.13f,side*.31f},
              {1.20f,.24f,side*.48f},
              {1.03f,.23f,side*.43f},carbon);
        panel({1.19f,.18f,side*.43f},
              {1.43f,.12f,side*.36f},
              {1.31f,.18f,side*.49f},
              {1.20f,.21f,side*.48f},tone(paint,.78f));

        // Headlight signature: two compact LED blades per corner.
        box(1.405f,.175f,side*.205f,.055f,.036f,.135f,carbon);
        box(1.438f,.183f,side*.205f,.018f,.014f,.105f,lamp);
        box(1.405f,.145f,side*.205f,.025f,.012f,.085f,accent);

        // Rear lamp strip and rear corner aero.
        box(-1.48f,.205f,side*.205f,.035f,.035f,.17f,tail);
        box(-1.34f,.17f,side*.35f,.20f,.045f,.12f,carbon);
    }

    // Low, wide front splitter with end fences.
    box(1.48f,.065f,0,.30f,.035f,.91f,carbon);
    box(1.58f,.082f,0,.10f,.018f,.83f,carbonHi);
    for(float side:{-1.f,1.f}) {
        box(1.42f,.11f,side*.405f,.19f,.12f,.025f,carbon);
        rod({1.37f,.10f,side*.43f},{1.23f,.18f,side*.43f},
            .012f,carbonHi);
    }

    // Rounded upper body volumes: soften the flat panel transitions and
    // give the hood and rear haunches a more sculpted prototype silhouette.
    ellipsoid(.83f,.205f,0,.62f,.105f,.285f,tone(paint,1.035f));
    ellipsoid(-1.00f,.205f,0,.46f,.105f,.355f,tone(paint,.96f));
    for(float side:{-1.f,1.f}) {
        ellipsoid(.76f,.235f,side*.355f,.48f,.145f,.175f,
                  tone(paint,1.08f));
        ellipsoid(-.91f,.235f,side*.385f,.43f,.145f,.17f,
                  tone(paint,1.045f));
    }

    // Contrasting custom-livery centre stripe, broken around the cockpit.
    box(.69f,.205f,0,.88f,.012f,.105f,pearl);
    box(1.20f,.145f,0,.54f,.012f,.095f,pearl);
    box(.98f,.218f,0,.23f,.014f,.025f,accent);

    // Cockpit canopy, dark panoramic glass and raised roof.
    cockpit(.34f,-.79f,-.08f,-.58f,.335f,.675f,.285f,.185f,
            tone(glass,1.12f),tone(paint,.83f));
    box(-.13f,.686f,0,.36f,.025f,.245f,tone(paint,1.12f));
    // Windscreen lower frame and roof centre accent.
    rod({.34f,.34f,-.285f},{.34f,.34f,.285f},.012f,carbonHi);
    box(-.13f,.704f,0,.42f,.012f,.025f,accent);

    // Roof air scoop, with dark opening and two cooling slots.
    box(-.47f,.675f,0,.30f,.085f,.145f,carbon);
    box(-.32f,.722f,0,.12f,.012f,.105f,carbonHi);
    for(int slot=0;slot<3;++slot)
        box(-.52f+slot*.055f,.723f,0,.018f,.008f,.075f,accent);

    // Central dorsal fin: a long, tapered blade rather than a flat wall.
    panel({-.30f,.30f,-.018f},{-1.30f,.27f,-.018f},
          {-1.12f,.57f,-.018f},{-.55f,.48f,-.018f},carbon);
    panel({-.30f,.30f,.018f},{-1.30f,.27f,.018f},
          {-1.12f,.57f,.018f},{-.55f,.48f,.018f},carbon);
    rod({-.38f,.31f,0},{-1.22f,.29f,0},.012f,accent);

    // Rear deck, engine-cover vents and a full-width diffuser.
    box(-1.08f,.155f,0,.60f,.045f,.56f,tone(paint,.83f));
    for(int vent=0;vent<7;++vent) {
        const float z=-.24f+vent*.08f;
        box(-.99f,.184f,z,.31f,.012f,.018f,carbon);
    }
    box(-1.48f,.075f,0,.30f,.045f,.92f,carbon);
    for(int fin=0;fin<7;++fin) {
        const float z=-.39f+fin*.13f;
        panel({-1.57f,.045f,z},
              {-1.18f,.045f,z},
              {-1.20f,.16f,z+.025f},
              {-1.53f,.14f,z+.025f},carbonHi);
    }
    box(-1.53f,.205f,0,.055f,.025f,.72f,tail);

    // Rear wing: two swan-neck supports, broad blade and endplates.
    for(float side:{-1.f,1.f}) {
        rod({-.98f,.30f,side*.29f},{-1.12f,.66f,side*.31f},
            .025f,carbon);
        rod({-1.18f,.30f,side*.29f},{-1.12f,.66f,side*.31f},
            .022f,carbonHi);
        box(-1.13f,.55f,side*.49f,.24f,.34f,.025f,carbon);
        box(-1.13f,.55f,side*.505f,.12f,.19f,.009f,accent);
    }
    rearWing(-1.13f,.69f,1.12f,.24f,carbon);
    box(-1.13f,.715f,0,.17f,.012f,.72f,carbonHi);

    // Small identification panels and tow points complete the prototype look.
    box(.88f,.235f,0,.22f,.012f,.055f,accent);
    box(-1.38f,.13f,0,.10f,.018f,.055f,pearl);
    for(float side:{-1.f,1.f})
        box(1.30f,.095f,side*.16f,.075f,.018f,.035f,pearl);
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
    case 3:
        // Raise only the WEC bodywork; wheels stay at their original ground contact.
        rlPushMatrix();
        rlTranslatef(0,.12f,0);
        wec(paint,glass,dark,car.brake>.1);
        rlPopMatrix();
        break;
    default: gt3(paint,glass,dark,car.brake>.1);break;
    }
    rlPopMatrix();
    wheels(car,profile,index);
    rlPopMatrix();
}

} // namespace karting
