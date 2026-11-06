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
    DrawCylinderEx(a,b,width,width,10,color);
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
    // BMW M4 GT3: broad shoulders, long bonnet, low greenhouse and planted stance.
    const Color carbon{22,25,31,255};
    const Color grille{7,10,15,255};
    const Color intake{12,15,20,255};
    const Color headlamp{225,238,245,255};

    // Broad M4-style bonnet: stays wide and nearly level all the way to the nose.
    // The front is blunt and muscular, not pinched down like a single-seater.
    bodyShell({{-1.30f,.13f,.34f,.39f},
               {-1.12f,.14f,.43f,.51f},
               {-.78f,.15f,.47f,.57f},
               {-.25f,.15f,.48f,.57f},
               { .35f,.14f,.45f,.56f},
               { .78f,.13f,.44f,.56f},
               {1.08f,.12f,.41f,.53f},
               {1.25f,.11f,.37f,.47f}},paint,tone(paint,.70f));

    // A lower, wider cabin. The long windscreen flows into a compact roof.
    cockpit(.40f,-.79f,.10f,-.58f,.47f,.79f,.43f,.33f,
            glass,tone(paint,.78f));

    // Fine clear-coat highlights and panel seams on the broad bonnet.
    for(float side:{-1.f,1.f}) {
        DrawLine3D({.94f,.426f,side*.43f},{.30f,.475f,side*.48f},
                   {255,255,255,42});
        DrawLine3D({-.18f,.481f,side*.555f},{.72f,.448f,side*.555f},
                   {12,15,20,150});
    }

    // Twin bonnet extraction vents inspired by the Mercedes-AMG GT3.
    // They sit flush on the flat bonnet, one on each side of the centreline.
    for(float side:{-1.f,1.f}) {
        const float ventZ=side*.365f;
        box(.63f,.451f,ventZ,.31f,.012f,.145f,{12,15,20,255});
        // Four slim longitudinal louvres inside each dark vent opening.
        for(int louvre=0;louvre<4;++louvre) {
            const float z=ventZ+(louvre-1.5f)*.031f;
            box(.63f,.460f,z,.265f,.009f,.012f,{78,84,92,255});
        }
        // Small painted edges make the vents read as recessed bonnet details.
        box(.63f,.458f,ventZ-side*.076f,.30f,.006f,.008f,
            tone(paint,.68f));
        box(.63f,.458f,ventZ+side*.076f,.30f,.006f,.008f,
            tone(paint,.68f));
    }

    // Front bumper, deep lower intake and the twin BMW kidney grilles.
    box(1.245f,.245f,0,.055f,.25f,.72f,carbon);
    box(1.278f,.265f,-.145f,.025f,.205f,.145f,grille);
    box(1.278f,.265f,.145f,.025f,.205f,.145f,grille);
    for(int bar=0;bar<5;++bar) {
        const float y=.185f+bar*.039f;
        box(1.292f,y,0,.012f,.010f,.57f,{70,77,86,255});
    }
    box(1.245f,.115f,0,.24f,.045f,1.18f,carbon); // wide splitter
    box(1.18f,.205f,0,.10f,.105f,.84f,intake); // lower air intake

    for(float side:{-1.f,1.f}) {
        // Long, narrow LED signatures rather than square headlights.
        box(1.19f,.405f,side*.365f,.075f,.055f,.20f,headlamp);
        box(1.20f,.365f,side*.365f,.04f,.018f,.22f,{65,165,220,255});

        // Recessed wheel openings: dark inner liners and a curved arch lip.
        // The tyres sit inside the body width instead of sticking out of the arches.
        for(float axle:{-.864f,.90f}) {
            const float archZ=side*.555f;
            for(int segment=0;segment<12;++segment) {
                const float a0=3.14159265f*segment/12.0f;
                const float a1=3.14159265f*(segment+1)/12.0f;
                const Vector3 p0{axle+std::cos(a0)*.235f,
                                 .22f+std::sin(a0)*.235f,archZ};
                const Vector3 p1{axle+std::cos(a1)*.235f,
                                 .22f+std::sin(a1)*.235f,archZ};
                rod(p0,p1,.010f,carbon);
            }
            box(axle,.205f,side*.535f,.34f,.12f,.018f,carbon);
        }

        // A single low side skirt, tucked between the wheel openings.
        box(.02f,.15f,side*.535f,.48f,.045f,.025f,carbon);

        // Rear light bar and lower bumper details.
        box(-1.285f,.365f,side*.365f,.025f,.065f,.20f,
            braking?RED:Color{155,24,31,255});
        box(-1.25f,.205f,side*.32f,.14f,.10f,.19f,carbon);
        // Exhaust tips sit low and outboard at the rear.
        rod({-1.26f,.255f,side*.245f},{-1.37f,.255f,side*.245f},
            .052f,{135,141,149,255});
        rod({-1.37f,.255f,side*.245f},{-1.385f,.255f,side*.245f},
            .037f,{31,35,42,255});
    }

    // A proper M4 GT3 rear wing, with two supports visibly attached to the rear deck.
    for(float side:{-1.f,1.f})
        rod({-.94f,.365f,side*.285f},
            {-1.13f,.685f,side*.325f},.032f,carbon);
    rearWing(-1.19f,.705f,1.12f,.245f,carbon);
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


void rally(Color paint,Color glass,Color dark,bool braking) {
    // Compact rallycross hatch: raised suspension, short overhangs and roof scoop.
    const Color carbon{24,28,34,255};
    const Color lamp{238,242,224,255};
    bodyShell({{-1.12f,.30f,.72f,.43f},
               {-.82f,.29f,.83f,.51f},
               {-.38f,.29f,.86f,.53f},
               { .42f,.29f,.82f,.52f},
               { .82f,.28f,.74f,.48f},
               {1.08f,.27f,.62f,.40f}},
              paint,tone(paint,.72f));
    cockpit(.38f,-.78f,.05f,-.65f,.82f,1.42f,.44f,.36f,
            glass,tone(paint,.82f));
    for(float side:{-1.f,1.f}) {
        // Flared arches, rally mud flaps and sturdy side skirts.
        for(float x:{-.68f,.64f}) {
            box(x,.48f,side*.51f,.48f,.18f,.13f,carbon);
            box(x-.03f,.29f,side*.55f,.19f,.25f,.035f,
                {35,38,43,255});
        }
        box(-.02f,.31f,side*.52f,.72f,.10f,.09f,carbon);
        box(.34f,1.08f,side*.46f,.18f,.10f,.09f,dark);
        // Roof rails and a small intake scoop.
        rod({-.62f,1.47f,side*.29f},{.45f,1.47f,side*.29f},
            .025f,carbon);
        box(-.18f,1.49f,0,.34f,.13f,.23f,carbon);
        // Door number panel and sill accent.
        box(-.05f,.77f,side*.536f,.35f,.25f,.018f,
            tone(WHITE,.92f));
        box(-.05f,.77f,side*.55f,.23f,.18f,.012f,paint);
    }
    // Rally-style front lamps and broad protective bumpers.
    box(1.08f,.36f,0,.13f,.22f,.82f,carbon);
    box(1.15f,.20f,0,.20f,.045f,1.12f,carbon);
    for(float side:{-1.f,1.f}) {
        box(1.13f,.62f,side*.27f,.05f,.10f,.17f,lamp);
        box(1.14f,.48f,side*.37f,.04f,.08f,.12f,
            {35,42,49,255});
        box(-1.10f,.58f,side*.31f,.035f,.13f,.16f,
            braking?RED:Color{150,27,31,255});
    }
    // Compact rear wing with two upright supports.
    for(float side:{-1.f,1.f})
        rod({-.72f,.78f,side*.30f},{-.91f,1.18f,side*.34f},
            .035f,carbon);
    rearWing(-.99f,1.20f,.92f,.19f,carbon);
    // Visible underbody skid plate.
    box(.25f,.18f,0,.72f,.045f,.72f,{92,98,105,255});
}


void formulaOne(Color paint,Color glass,bool braking) {
    const Color carbon{22,26,32,255};
    const Color blue{25,115,255,255};

    // Narrow pointed nose, raised chassis and sculpted sidepods.
    bodyShell({{-1.62f,.19f,.28f,.25f},
               {-1.28f,.18f,.38f,.34f},
               {-.78f,.18f,.43f,.39f},
               {-.25f,.19f,.39f,.38f},
               { .42f,.18f,.36f,.40f},
               {1.17f,.17f,.31f,.38f},
               {1.59f,.15f,.27f,.32f}},
              paint,tone(paint,.70f));
    // Open cockpit with a visible driver and halo.
    box(-.28f,.40f,0,.70f,.12f,.38f,carbon);
    DrawSphere({-.02f,.66f,0},.19f,{238,190,62,255});
    DrawSphere({.015f,.69f,0},.145f,glass);
    rod({-.35f,.60f,-.23f},{.22f,.77f,-.19f},.022f,carbon);
    rod({-.35f,.60f,.23f},{.22f,.77f,.19f},.022f,carbon);
    rod({.22f,.77f,-.19f},{.22f,.77f,.19f},.022f,carbon);
    // Sidepods and cooling inlets.
    for(float side:{-1.f,1.f}) {
        box(-.02f,.31f,side*.40f,.90f,.22f,.28f,paint);
        box(.42f,.37f,side*.535f,.30f,.12f,.018f,carbon);
        box(-.48f,.27f,side*.43f,.42f,.045f,.025f,blue);
        // Front wing: layered flaps and endplates.
        box(1.73f,.15f,side*.48f,.36f,.045f,.12f,carbon);
        box(1.78f,.20f,side*.48f,.27f,.025f,.11f,paint);
        box(1.67f,.17f,side*.69f,.32f,.31f,.035f,carbon);
        // Rear wing endplates and support pylons.
        box(-1.48f,.68f,side*.55f,.30f,.52f,.035f,carbon);
        rod({-1.12f,.30f,side*.18f},{-1.40f,.66f,side*.18f},.035f,carbon);
        // Clean sidepod silhouette without detached decorative bars.
    }
    // Central exhaust outlet integrated into the rear diffuser.
    box(-1.60f,.245f,0,.16f,.11f,.22f,carbon);
    rod({-1.66f,.245f,0},{-1.78f,.245f,0},.075f,{145,151,160,255});
    rod({-1.78f,.245f,0},{-1.795f,.245f,0},.055f,{28,32,39,255});
    // Rear diffuser and a broad, two-element rear wing.
    box(-1.43f,.15f,0,.35f,.08f,.88f,carbon);
    box(-1.57f,.83f,0,.30f,.045f,1.28f,carbon);
    box(-1.53f,.89f,0,.25f,.025f,1.22f,paint);
    // Small front suspension fairing and nose tip.
    box(1.80f,.22f,0,.18f,.07f,.20f,carbon);
    box(1.87f,.25f,0,.09f,.035f,.12f,paint);
}

void trophyTruck(Color paint,Color glass,bool braking,bool second) {
    const Color cage{34,39,44,255};
    const Color chassis{53,59,65,255};
    const Color alloy{155,163,170,255};
    const Color lamp=braking?Color{255,42,35,255}:Color{235,226,184,255};
    const float width=second?1.02f:.96f;
    // Tall, wide-fendered body with short overhangs and a raised belly.
    bodyShell({{-1.22f,.43f,.91f,.67f},
               {-.92f,.43f,1.05f,.83f},
               {-.48f,.43f,1.08f,.87f},
               {.28f,.43f,1.02f,.86f},
               {.82f,.43f,.96f,.80f},
               {1.18f,.43f,.86f,.70f},
               {1.38f,.43f,.72f,.58f}},
              paint,tone(paint,.68f));
    // Open tubular cabin and high-strength roll cage.
    cockpit(.38f,-.76f,.12f,-.48f,.91f,1.48f,.52f,.43f,
            glass,tone(paint,.86f));
    for(float side:{-1.f,1.f}) {
        rod({-.86f, .82f,side*.48f},{-.48f,1.58f,side*.43f},.045f,cage);
        rod({-.48f,1.58f,side*.43f},{ .47f,1.58f,side*.43f},.045f,cage);
        rod({ .47f,1.58f,side*.43f},{ .83f,.82f,side*.48f},.045f,cage);
        rod({-.86f,.82f,side*.48f},{ .83f,.82f,side*.48f},.035f,cage);
        rod({-.45f,1.55f,side*.42f},{ .48f,.94f,side*.52f},.032f,cage);
        box(.05f,.56f,side*.57f,.82f,.12f,.055f,chassis);
        // Front and rear fender flares.
        for(float x:{-.78f,.88f})
            box(x,.70f,side*.66f,.58f,.18f,.13f,tone(paint,.72f));
        box(1.34f,.53f,side*.48f,.13f,.22f,.18f,cage);
        box(-1.22f,.54f,side*.48f,.13f,.20f,.18f,cage);
        // Exposed long-travel suspension arms and coil-over dampers.
        for(float x:{-.94f,.94f}) {
            const Vector3 hub{x,.43f,side*.88f};
            rod({x-.30f,.35f,side*.35f},hub,.035f,alloy);
            rod({x+.30f,.35f,side*.35f},hub,.035f,alloy);
            rod({x,.88f,side*.38f},{x,.59f,side*.78f},.025f,{65,133,205,255});
            const Vector3 springTop{x,.91f,side*.42f};
            const Vector3 springBottom{x,.57f,side*.79f};
            for(int coil=0;coil<18;++coil) {
                const float t0=float(coil)/18.f,t1=float(coil+1)/18.f;
                const float a0=t0*6.0f*2*PI,a1=t1*6.0f*2*PI;
                const Vector3 p0{springTop.x+std::cos(a0)*.045f,
                    springTop.y+(springBottom.y-springTop.y)*t0,
                    springTop.z+(springBottom.z-springTop.z)*t0+std::sin(a0)*.045f};
                const Vector3 p1{springTop.x+std::cos(a1)*.045f,
                    springTop.y+(springBottom.y-springTop.y)*t1,
                    springTop.z+(springBottom.z-springTop.z)*t1+std::sin(a1)*.045f};
                DrawLine3D(p0,p1,{25,115,255,255});
            }
        }
    }
    // Front grille, LED bar, skid plate, rear spare and roof-mounted intake.
    box(1.37f,.59f,0,.12f,.25f,.72f,cage);
    box(1.44f,.69f,0,.025f,.07f,.58f,lamp);
    box(1.02f,.36f,0,.65f,.12f,1.08f,alloy);
    box(-.15f,1.50f,0,.36f,.10f,.27f,cage);
    rod({-.98f,.91f,-.10f},{-1.27f,.91f,-.10f},.34f,{29,32,37,255});
    box(-1.27f,.91f,0,.10f,.38f,.39f,alloy);
    box(-1.31f,.57f,0,.12f,.16f,1.02f,cage);
    for(float side:{-1.f,1.f}) {
        box(-1.25f,.75f,side*.35f,.05f,.12f,.20f,braking?RED:Color{170,30,28,255});
        box(1.38f,.76f,side*.32f,.035f,.10f,.18f,lamp);
    }
    (void)width;
}

void wheels(const Car& car,const VehicleProfile& p,int index) {
    const Color carbon{22,26,32,255};
    // GT3 wheel centres sit close to the actual front and rear wheel arches.
    const float axleVisualScale=index==3?1.12f:1.0f;
    const float front=float(p.frontAxle*kWorldScale*axleVisualScale);
    const float rear=float(p.rearAxle*kWorldScale*axleVisualScale);
    constexpr std::array<float,8> sizes{{.155f,.265f,.205f,.195f,.180f,.220f,.315f,.325f}};
    constexpr std::array<float,8> widths{{.19f,.26f,.22f,.21f,.21f,.245f,.34f,.36f}};
    constexpr std::array<float,8> tracks{{.45f,.65f,.48f,.58f,.58f,.63f,.88f,.92f}};
    const std::size_t slot=std::size_t(std::clamp(index,0,5));
    const float r=sizes[slot],width=widths[slot];
    const float track=std::max(float(p.halfWheelTrack*kWorldScale),tracks[slot]);

    const Vec2 forward{std::cos(car.heading),std::sin(car.heading)};
    const Vec2 lateral{-forward.y,forward.x};
    for(float x:{rear,front}) for(float side:{-1.f,1.f}) {
        // Keep the original wheel track and tyre width. Only limit vertical
        // suspension travel on the GT3 so terrain height cannot push tyres
        // up through the front bodywork when steering.
        const Vec2 wheelWorld=car.position+
            forward*(x/kWorldScale)+lateral*(side*track/kWorldScale);
        float wheelHeight=float(
            (elevation(wheelWorld)-elevation(car.position))*kWorldScale);
        if(slot==2) wheelHeight=std::clamp(wheelHeight,-.045f,.045f);
        if(slot==5) {
            const Color carbon{22,26,32,255};
            wheelHeight=std::clamp(wheelHeight,-.075f,.075f);
            const float wheelY=r+.015f+wheelHeight;
            const float innerZ=side*(track-.34f);
            const Vector3 upperA{x-.22f,.28f,innerZ};
            const Vector3 upperB{x+.22f,.28f,innerZ};
            const Vector3 upperHub{x,wheelY+.105f,side*(track-.015f)};
            const Vector3 lowerA{x-.25f,.12f,innerZ};
            const Vector3 lowerB{x+.25f,.12f,innerZ};
            const Vector3 lowerHub{x,wheelY-.105f,side*(track-.015f)};
            // Two arms converge at each upright joint: proper triangular
            // upper and lower wishbones, with their inner pivots on brackets.
            rod(upperA,upperHub,.018f,{145,153,164,255});
            rod(upperB,upperHub,.018f,{145,153,164,255});
            rod(lowerA,lowerHub,.022f,{145,153,164,255});
            rod(lowerB,lowerHub,.022f,{145,153,164,255});
            // Short mounts visibly connect the wishbone pivots to the chassis.
            rod({x-.22f,.28f,side*.24f},upperA,.025f,carbon);
            rod({x+.22f,.28f,side*.24f},upperB,.025f,carbon);
            rod({x-.25f,.12f,side*.24f},lowerA,.027f,carbon);
            rod({x+.25f,.12f,side*.24f},lowerB,.027f,carbon);
            // The upright links both ball joints and moves with the wheel.
            rod(upperHub,lowerHub,.027f,{75,83,95,255});
            DrawSphere(upperHub,.025f,{185,193,203,255});
            DrawSphere(lowerHub,.027f,{185,193,203,255});
            // Blue coil-over: its lower end follows the wheel's vertical travel.
            const float springX=x+.12f;
            const float springZ=side*(track-.13f);
            const float springTop=.30f, springBottom=wheelY-.12f;
            for(int segment=0;segment<48;++segment) {
                const float t0=float(segment)/48.f,t1=float(segment+1)/48.f;
                const float a0=t0*6.f*2*PI,a1=t1*6.f*2*PI;
                const float y0=springTop+(springBottom-springTop)*t0;
                const float y1=springTop+(springBottom-springTop)*t1;
                DrawLine3D({springX+std::cos(a0)*.035f,y0,springZ+std::sin(a0)*.035f},
                           {springX+std::cos(a1)*.035f,y1,springZ+std::sin(a1)*.035f},
                           {25,115,255,255});
            }
            rod({springX,springTop,springZ},
                {springX,springBottom,springZ},.025f,{70,80,95,255});
        }
        rlPushMatrix();
        rlTranslatef(x,r+.015f+wheelHeight,side*track);
        if(x==front) rlRotatef(float(-car.steering*RAD2DEG),0,1,0);
        DrawCylinderEx({0,0,-width*.5f},{0,0,width*.5f},r,r,slot==2||slot==3?24:16,
                       {27,30,36,255});
        // Fine circumferential grooves and angled tread cuts add rubber detail.
        for(int band=0;band<5;++band) {
            const float treadZ=-width*.38f+float(band)*width*.19f;
            for(int segment=0;segment<32;++segment) {
                const float a0=2*PI*float(segment)/32.f+float(car.wheelRotation);
                const float a1=a0+.13f;
                const float r0=r*.985f,r1=r*.978f;
                DrawLine3D({std::cos(a0)*r0,std::sin(a0)*r0,treadZ},
                           {std::cos(a1)*r1,std::sin(a1)*r1,
                            treadZ+width*.055f},
                           {13,15,19,255});
            }
        }
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
        rlTranslatef(0,.035f,0);
        // Give the main body more vertical volume and a slightly shorter nose-to-tail
        // profile. Keep the wheelbase and tyre contact points unchanged.
        rlScalef(.96f,.96f,1.0f);
        wec(paint,glass,dark,car.brake>.1);
        rlPopMatrix();
        break;
    case 5:
        // Compact the F1 body around its centre; suspension and wheels retain
        // their own dimensions so the tyres stay planted on the track.
        rlPushMatrix();
        rlScalef(.88f,.90f,.90f);
        formulaOne(paint,glass,car.brake>.1);
        rlPopMatrix();
        break;
    case 6:
        trophyTruck(paint,glass,car.brake>.1,false);
        break;
    case 7:
        trophyTruck(paint,glass,car.brake>.1,true);
        break;
    case 4:
        // Scale the rally hatch to a compact real-world footprint.
        rlPushMatrix();
        rlScalef(.84f,.84f,.84f);
        rally(paint,glass,dark,car.brake>.1);
        rlPopMatrix();
        break;
    default: gt3(paint,glass,dark,car.brake>.1);break;
    }
    // Restrained clear-coat highlights help every vehicle read as painted metal.
    const float highlightY=index==1?.82f:index==2?.445f:index==3?.285f:index==4?.80f:index==5?.39f:index>=6?.94f:.205f;
    const float highlightZ=index==1?.47f:index==2?.49f:index==3?.34f:index==4?.48f:index==5?.36f:index>=6?.62f:.20f;
    for(float side:{-1.f,1.f}) {
        DrawLine3D({.62f,highlightY,side*highlightZ},
                   {-.48f,highlightY-.012f,side*highlightZ*.94f},
                   {255,255,255,36});
    }
    rlPopMatrix();
    wheels(car,profile,index);
    rlPopMatrix();
}

} // namespace karting
