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
    // Modern GT3: broad stance, recognisable wheel haunches, low roof and
    // separate aerodynamic details. All decoration remains procedural.
    const Color shadow=tone(paint,.70f);
    const Color highlight=tone(paint,1.13f);
    const Color carbon{24,27,33,255};
    const Color grille{12,16,20,255};
    const Color metal{126,132,140,255};
    const Color headlight{239,245,223,255};
    const Color tail=braking?Color{255,46,39,255}:Color{155,27,34,255};
    const Color crease=tone(paint,.58f);
    const Color silver{180,187,196,255};

    // Front faces +X. Keep the same footprint as the original GT3 so the
    // existing wheels, camera and physics remain compatible.
    // Flatten only the GT3 body (15% lower), pivoting around its floor.
    // Wheels are drawn afterwards in their own transform, unchanged.
    rlPushMatrix();
    rlTranslatef(0.f,.14f,0.f);
    rlScalef(1.f,.85f,1.f);
    rlTranslatef(0.f,-.14f,0.f);
    bodyShell({{-1.18f,.15f,.42f,.48f},
               {-.95f,.15f,.52f,.57f},
               {-.54f,.16f,.55f,.56f},
               {-.13f,.16f,.54f,.52f},
               { .39f,.15f,.49f,.54f},
               { .83f,.14f,.42f,.55f},
               {1.18f,.13f,.35f,.48f}},paint,shadow);
    cockpit(.30f,-.81f,-.02f,-.60f,.54f,.94f,.405f,.272f,
            glass,tone(paint,.83f));

    // Define the side windows, B-pillars and roof without thick black slabs.
    for(float side:{-1.f,1.f}) {
        rod({-.41f,.945f,side*.277f},
            {-.41f,.543f,side*.407f},.010f,dark);
        rod({-.59f,.949f,side*.273f},
            {-.03f,.949f,side*.273f},.008f,shadow);
    }
    box(-.32f,.951f,0,.28f,.014f,.12f,shadow);
    // Thin race sunstrip and integrated roof rails; neither increases
    // the height of the already lowered cockpit.
    panel({.040f,.917f,-.262f},{.040f,.917f,.262f},
          {-.045f,.952f,.272f},{-.045f,.952f,-.272f},carbon);
    for(float side:{-1.f,1.f}) {
        rod({-.53f,.947f,side*.257f},
            {-.03f,.947f,side*.257f},.008f,highlight);
    }

    // Raised wheel-arch shoulders give the sides some shape rather than
    // looking like one rectangular block. They do not move the wheels.
    for(float side:{-1.f,1.f}) {
        for(float axle:{-.40f,.40f}) {
            panel({axle-.35f,.40f,side*.53f},
                  {axle-.21f,.56f,side*.615f},
                  {axle+.20f,.55f,side*.615f},
                  {axle+.34f,.40f,side*.53f},highlight);
            rod({axle-.15f,.562f,side*.620f},
                {axle+.14f,.558f,side*.620f},.006f,
                tone(paint,1.28f));
        }
        // Wheel-arch lip follows the tyre rather than being a straight box.
        // Wheel centres and tyres themselves remain untouched.
        for(float axle:{-.396f,.396f}) {
            for(int segment=0;segment<9;++segment) {
                const float a0=3.14159265f*float(segment)/9.f;
                const float a1=3.14159265f*float(segment+1)/9.f;
                const auto point=[&](float a) {
                    return Vector3{axle+.234f*std::cos(a),
                                   .222f+.234f*std::sin(a),
                                   side*.704f};
                };
                rod(point(a0),point(a1),.008f,crease);
            }
        }
        // Carbon side skirt and front canard, with a rear brake-cooling duct.
        box(-.04f,.17f,side*.58f,1.69f,.08f,.15f,carbon);
        box(-.04f,.221f,side*.661f,1.32f,.012f,.018f,shadow);
        box(-.78f,.34f,side*.59f,.24f,.08f,.020f,grille);
        // Deep rear brake duct and the sculpted channel leading into it.
        panel({-.88f,.29f,side*.585f},
              {-.64f,.29f,side*.612f},
              {-.54f,.42f,side*.604f},
              {-.77f,.39f,side*.582f},carbon);
        rod({-.78f,.345f,side*.619f},
            {-.60f,.353f,side*.624f},.008f,silver);
        // Pronounced sideblade, tapered towards the front wheel.
        panel({-.41f,.27f,side*.598f},
              {.60f,.27f,side*.593f},
              {.80f,.32f,side*.549f},
              {-.23f,.33f,side*.573f},shadow);
        panel({.93f,.24f,side*.49f},{1.18f,.18f,side*.60f},
              {1.08f,.22f,side*.60f},{.87f,.30f,side*.52f},carbon);

        // Door outline and number panel. Deliberately logo-free.
        DrawLine3D({-.17f,.30f,side*.563f},
                   {-.17f,.51f,side*.563f},tone(paint,.52f));
        DrawLine3D({.29f,.36f,side*.551f},
                   {.29f,.49f,side*.551f},crease);
        rod({-.15f,.47f,side*.583f},{.21f,.47f,side*.565f},
            .006f,highlight);
        box(-.06f,.402f,side*.578f,.27f,.17f,.018f,
            {231,232,225,255});
        // A small raised race number "7" rather than a generic black square.
        rod({-.119f,.459f,side*.597f},
            {.003f,.459f,side*.597f},.009f,grille);
        rod({.003f,.459f,side*.597f},
            {-.073f,.346f,side*.597f},.009f,grille);
        box(.26f,.74f,side*.45f,.14f,.075f,.12f,dark); // mirror
        rod({.20f,.71f,side*.39f},{.26f,.74f,side*.45f},.018f,dark);

        // Low, sharp front lamp with separate white daylight signature.
        box(1.183f,.365f,side*.395f,.020f,.082f,.205f,grille);
        box(1.198f,.385f,side*.405f,.018f,.027f,.177f,headlight);
        box(1.195f,.344f,side*.462f,.018f,.021f,.063f,
            {164,198,218,255});
        // Angular DRL signature and a tiny amber corner indicator.
        rod({1.214f,.400f,side*.323f},
            {1.215f,.365f,side*.484f},.009f,headlight);
        box(1.210f,.331f,side*.476f,.014f,.014f,.032f,
            {232,155,65,255});

        // Rear light clusters are bright when the brake is applied.
        box(-1.193f,.424f,side*.405f,.021f,.055f,.20f,grille);
        box(-1.209f,.430f,side*.406f,.016f,.024f,.171f,tail);
    }

    // Deep front fascia: central mesh intake, two side intakes and splitter.
    box(1.191f,.246f,0,.026f,.142f,.60f,grille);
    for(int i=-4;i<=4;++i)
        box(1.209f,.245f,float(i)*.058f,.012f,.113f,.012f,
            {56,62,70,255});
    for(int i=0;i<3;++i)
        box(1.218f,.208f+float(i)*.036f,0.f,
            .009f,.006f,.57f,{72,79,89,255});
    // Central towing eye is visible but stays behind the splitter.
    rod({1.218f,.193f,.20f},{1.230f,.193f,.20f},.026f,silver);
    for(float side:{-1.f,1.f})
        box(1.192f,.253f,side*.393f,.028f,.104f,.171f,carbon);
    box(1.205f,.139f,0,.275f,.047f,1.17f,carbon);
    box(1.285f,.154f,0,.053f,.018f,1.10f,tone(carbon,1.45f));

    // Hood contours and extractor vents follow the actual bonnet slope.
    for(float side:{-1.f,1.f}) {
        rod({.49f,.487f,side*.40f},
            {1.06f,.387f,side*.40f},.006f,highlight);
        panel({.60f,.483f,side*.16f},
              {.89f,.435f,side*.16f},
              {.89f,.437f,side*.31f},
              {.60f,.493f,side*.31f},grille);
        rod({.65f,.493f,side*.24f},
            {.85f,.456f,side*.24f},.008f,metal);
        // Long bonnet power line follows the existing low hood.
        rod({.32f,.508f,side*.355f},
            {.79f,.433f,side*.385f},.008f,crease);
    }
    // Low front bonnet stripes to break up the flat top surface.
    panel({.33f,.507f,-.078f},{.33f,.507f,.078f},
          {.82f,.431f,.078f},{.82f,.431f,-.078f},
          tone(paint,.90f));

    // Vented rear deck: small surface panels, not a roof-height air scoop.
    for(float side:{-1.f,1.f}) {
        panel({-1.07f,.487f,side*.13f},
              {-.87f,.538f,side*.13f},
              {-.87f,.538f,side*.33f},
              {-1.07f,.487f,side*.33f},grille);
        for(int i=0;i<3;++i) {
            const float x=-1.04f+float(i)*.066f;
            rod({x,.50f+float(i)*.017f,side*.15f},
                {x,.50f+float(i)*.017f,side*.31f},
                .006f,metal);
        }
    }

    // Rear bumper, diffuser channels and discreet dual exhausts.
    box(-1.196f,.237f,0,.050f,.17f,1.03f,carbon);
    box(-1.216f,.426f,0,.018f,.019f,.38f,tail);
    // Centre rear cooling grille with three visible horizontal slats.
    box(-1.211f,.331f,0.f,.018f,.11f,.48f,grille);
    for(int i=0;i<3;++i)
        box(-1.227f,.290f+float(i)*.040f,0.f,
            .010f,.009f,.44f,{66,73,82,255});
    box(-1.236f,.125f,0,.29f,.06f,1.13f,carbon);
    for(float z:{-.44f,-.22f,0.f,.22f,.44f})
        box(-1.256f,.155f,z,.245f,.100f,.025f,grille);
    for(float side:{-1.f,1.f})
        rod({-1.240f,.254f,side*.25f},
            {-1.273f,.254f,side*.25f},.060f,metal);
    // Rear bumper outline gives the chase camera a readable GT3 shape.
    rod({-1.221f,.48f,-.465f},
        {-1.221f,.48f,.465f},.010f,highlight);

    // Twin pylons and a wide, thin GT wing with clear end plates.
    for(float side:{-1.f,1.f})
        rod({-1.018f,.46f,side*.34f},
            {-1.100f,.81f,side*.36f},.024f,carbon);
    rearWing(-1.105f,.84f,1.23f,.23f,carbon);
    // Subtle Gurney flap, not a second tall wing.
    box(-1.190f,.865f,0.f,.013f,.023f,1.17f,grille);
    rlPopMatrix();
}

void wec(Color paint,Color glass,Color dark,bool braking) {
    // Simple WEC Hypercar silhouette. Front is +X, rear is -X.
    const Color carbon{24,28,34,255};
    const Color glassDark=tone(glass,.72f);
    const Color headlamp{235,243,232,255};
    const Color tail=braking?Color{255,38,32,255}:Color{135,22,28,255};

    // One continuous, gently rounded body profile.
    bodyShell({{-1.43f,.085f,.19f,.30f},
               {-1.20f,.09f,.26f,.39f},
               {-.82f,.10f,.31f,.43f},
               {-.38f,.105f,.32f,.42f},
               { .18f,.10f,.28f,.39f},
               { .67f,.09f,.23f,.36f},
               {1.08f,.075f,.18f,.31f},
               {1.38f,.065f,.13f,.23f},
               {1.48f,.06f,.105f,.16f}},
              paint,tone(paint,.76f));

    // Compact integrated canopy: lower roof, short windscreen and rear glass.
    cockpit(.31f,-.73f,-.045f,-.54f,.315f,.66f,.275f,.185f,
            glassDark,tone(paint,.88f));

    // Small front splitter and simple flat floor.
    box(1.43f,.075f,0,.25f,.035f,.88f,carbon);
    box(-1.40f,.075f,0,.18f,.045f,.84f,carbon);

    // Two subtle side skirts, kept as straight elements.
    for(float side:{-1.f,1.f}) {
        box(-.08f,.115f,side*.405f,1.30f,.055f,.055f,carbon);
        // Minimal front and rear light units.
        box(1.405f,.16f,side*.205f,.025f,.035f,.13f,headlamp);
        box(-1.435f,.195f,side*.265f,.025f,.035f,.14f,tail);
    }

    // One low central fin, with no oversized roof scoop or extra decoration.
    panel({-.34f,.30f,-.012f},{-1.22f,.27f,-.012f},
          {-1.04f,.43f,-.012f},{-.55f,.40f,-.012f},carbon);
    panel({-.34f,.30f,.012f},{-1.22f,.27f,.012f},
          {-1.04f,.43f,.012f},{-.55f,.40f,.012f},carbon);

    // Low twin supports and a thin, restrained rear wing.
    for(float side:{-1.f,1.f})
        rod({-1.02f,.27f,side*.27f},
            {-1.18f,.48f,side*.30f},.018f,carbon);
    rearWing(-1.25f,.50f,.98f,.14f,carbon);
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
        // GT3 gets a denser motorsport rim and a visible red brake caliper.
        // Wheel rotation still comes from the simulation.
        if(index==2)
            box(-.045f,.087f,z+side*.031f,.052f,.080f,.018f,
                {186,48,38,255});
        const int spokes=index==2?10:5;
        for(int spoke=0;spoke<spokes;++spoke) {
            const float step=index==2?.62831853f:1.2566371f;
            const float a=float(car.wheelRotation)+spoke*step;
            DrawLine3D({std::cos(a)*r*.2f,std::sin(a)*r*.2f,z+side*.028f},
                       {std::cos(a)*r*.48f,std::sin(a)*r*.48f,z+side*.028f},
                       index==2?Color{201,206,211,255}:Color{180,185,193,255});
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
        // Nathan's adjusted hypercar body: wheels and driving physics
        // retain their original reference positions.
        rlPushMatrix();
        rlTranslatef(0,.035f,0);
        rlScalef(.96f,.96f,1.0f);
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
