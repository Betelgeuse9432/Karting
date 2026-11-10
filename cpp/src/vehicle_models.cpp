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

void gt3Monster(Color glass,bool braking) {
    const Color black{10,13,17,255};
    const Color green{105,220,35,255};
    const Color greenDark{48,126,24,255};
    const Color carbon{18,22,27,255};
    // Start with the aggressive wide-body GT3 silhouette, then apply a fixed
    // black-and-neon-green Monster-style racing livery.
    gt3(black,glass,carbon,braking);
    // Sharp green bonnet blades, aligned with the bonnet surface.
    for(float side:{-1.f,1.f}) {
        panel({1.08f,.435f,side*.12f},{.30f,.482f,side*.19f},
              {.12f,.484f,side*.255f},{.93f,.438f,side*.235f},green);
        // Neon shoulder stripe continues from front arch to rear quarter.
        box(.02f,.337f,side*.558f,.78f,.025f,.014f,green);
        box(-.62f,.352f,side*.545f,.34f,.025f,.018f,greenDark);
        // Three claw-like diagonal marks on each door/side panel.
        for(int claw=0;claw<3;++claw) {
            const float x=.12f-claw*.17f;
            panel({x+.13f,.39f,side*.571f},
                  {x-.03f,.39f,side*.571f},
                  {x-.20f,.255f,side*.571f},
                  {x-.07f,.255f,side*.571f},green);
        }
        // Green accents on the front canard and rear wing endplate.
        panel({1.22f,.19f,side*.47f},{.98f,.29f,side*.57f},
              {.91f,.25f,side*.57f},{1.16f,.15f,side*.47f},green);
        box(-1.20f,.70f,side*.555f,.22f,.018f,.014f,green);
    }
    // Roof stripe and rear deck flashes make the livery visible from above.
    box(-.20f,.795f,0,.78f,.012f,.035f,green);
    box(-.91f,.452f,0,.38f,.012f,.08f,greenDark);
    // Deep front splitter with a bright centre blade.
    box(1.31f,.105f,0,.18f,.018f,.72f,green);
    // Small roof number panel, a clean motorsport detail.
    box(-.20f,.808f,0,.25f,.008f,.20f,{235,238,230,255});
}
void gt3RedBull(Color glass,bool braking) {
    const Color blue{5,24,67,255};
    const Color red{218,25,42,255};
    const Color yellow{255,204,0,255};
    const Color carbon{16,21,31,255};
    gt3(blue,glass,carbon,braking);
    // Red Bull-inspired colours and abstract wing graphics, not a copied logo.
    for(float side:{-1.f,1.f}) {
        panel({1.10f,.431f,side*.16f},{.43f,.468f,side*.22f},
              {.08f,.478f,side*.30f},{.80f,.438f,side*.31f},red);
        panel({.64f,.405f,side*.572f},{.28f,.405f,side*.572f},
              {-.12f,.255f,side*.572f},{.18f,.255f,side*.572f},red);
        panel({.12f,.39f,side*.575f},{-.18f,.39f,side*.575f},
              {-.48f,.29f,side*.575f},{-.27f,.29f,side*.575f},yellow);
        panel({.42f,.355f,side*.582f},{.12f,.405f,side*.582f},
              {-.04f,.365f,side*.582f},{.23f,.325f,side*.582f},yellow);
        panel({.14f,.345f,side*.584f},{-.16f,.39f,side*.584f},
              {-.30f,.345f,side*.584f},{-.03f,.305f,side*.584f},yellow);
        box(-.67f,.354f,side*.548f,.38f,.025f,.016f,red);
        box(-1.20f,.70f,side*.555f,.22f,.018f,.014f,yellow);
    }
    box(.83f,.454f,0,.55f,.012f,.085f,red);
    box(1.31f,.108f,0,.18f,.018f,.72f,yellow);
    box(-.20f,.795f,0,.76f,.012f,.035f,red);
    box(-.91f,.452f,0,.36f,.012f,.075f,yellow);
}

void wec(Color paint,Color glass,Color dark,bool braking) {
    // Modern WEC Hypercar: low closed-cockpit prototype with sculpted fenders,
    // deep aero floor, front dive planes, cooling channels and a tall rear wing.
    // Front is +X, rear is -X.
    const Color carbon{21,25,31,255};
    const Color aero{34,40,48,255};
    const Color glassDark=tone(glass,.62f);
    const Color lamp{231,242,238,255};
    const Color tail=braking?Color{255,35,28,255}:Color{154,24,31,255};
    const Color vent{9,13,18,255};

    // Wide, low central monocoque with a long nose and tapered rear deck.
    bodyShell({{-1.48f,.085f,.19f,.32f},
               {-1.28f,.095f,.27f,.43f},
               {-.94f,.105f,.31f,.47f},
               {-.48f,.105f,.32f,.45f},
               { .08f,.095f,.29f,.42f},
               { .55f,.085f,.24f,.39f},
               { .98f,.075f,.20f,.34f},
               {1.30f,.065f,.15f,.27f},
               {1.52f,.055f,.105f,.18f}},
              paint,tone(paint,.70f));

    // Closed cockpit canopy: broad windscreen, compact roof and sloped rear glass.
    cockpit(.38f,-.72f,-.10f,-.52f,.315f,.70f,.285f,.205f,
            glassDark,tone(paint,.82f));
    // Windscreen base and roof edge give the canopy a proper prototype outline.
    rod({.38f,.315f,-.285f},{-.10f,.70f,-.205f},.018f,carbon);
    rod({.38f,.315f,.285f},{-.10f,.70f,.205f},.018f,carbon);

    // Deep front splitter, central intake and two separated brake-cooling ducts.
    box(1.47f,.075f,0,.34f,.035f,1.12f,carbon);
    box(1.475f,.19f,0,.045f,.16f,.46f,vent);
    for(float side:{-1.f,1.f}) {
        box(1.405f,.185f,side*.315f,.10f,.105f,.19f,vent);
        box(1.455f,.13f,side*.49f,.22f,.10f,.055f,aero);
        // Front dive planes are mounted directly to the outer bumper corners.
        panel({1.27f,.20f,side*.43f},{1.03f,.30f,side*.57f},
              {.99f,.17f,side*.57f},{1.30f,.13f,side*.43f},carbon);
        // Narrow headlight strips integrated into the front fender shoulders.
        box(1.365f,.275f,side*.255f,.055f,.045f,.20f,lamp);
        box(1.39f,.245f,side*.255f,.025f,.018f,.16f,
            {70,155,180,255});
    }

    // Raised front and rear wheel pods, joined to the main body by broad shoulders.
    for(float side:{-1.f,1.f}) {
        const float z=side*.455f;
        // Raised fender crowns over both axles, with visible wheel clearance.
        for(float axle:{.91f,-.91f}) {
            const float archZ=side*.535f;
            for(int segment=0;segment<14;++segment) {
                const float a0=3.14159265f*float(segment)/14.0f;
                const float a1=3.14159265f*float(segment+1)/14.0f;
                const Vector3 p0{axle+std::cos(a0)*.255f,
                                 .205f+std::sin(a0)*.255f,archZ};
                const Vector3 p1{axle+std::cos(a1)*.255f,
                                 .205f+std::sin(a1)*.255f,archZ};
                rod(p0,p1,.018f,carbon);
            }
            // Solid fender shoulders connect the arch to the central body shell.
            box(axle,.315f,side*.435f,.53f,.105f,.16f,paint);
            box(axle,.245f,side*.545f,.43f,.035f,.025f,aero);
            // Three extraction slots sit on top of each wheel pod.
            for(int slot=0;slot<3;++slot)
                box(axle-.015f,.374f,side*(.36f+slot*.045f),
                    .19f,.012f,.018f,carbon);
        }

        // Sidepod: broad shoulder, deep undercut and recessed cooling inlet.
        box(-.08f,.205f,z,.92f,.19f,.22f,paint);
        box(-.03f,.285f,side*.555f,.47f,.085f,.025f,aero);
        box(-.12f,.295f,side*.572f,.31f,.065f,.012f,vent);
        for(int l=0;l<4;++l)
            box(-.12f,.302f,side*(.535f+l*.023f),
                .25f,.012f,.009f,{105,113,123,255});
        // Deep side skirt and floor edge, visibly attached between the axles.
        box(-.02f,.105f,side*.535f,.92f,.085f,.075f,carbon);
        box(-.03f,.075f,side*.49f,.98f,.035f,.12f,aero);

        // Rear brake light and vertical rear corner aero fences.
        box(-1.455f,.235f,side*.255f,.025f,.055f,.12f,tail);
        box(-1.28f,.20f,side*.43f,.20f,.14f,.055f,carbon);
        panel({-1.30f,.16f,side*.48f},{-1.02f,.38f,side*.52f},
              {-.88f,.16f,side*.52f},{-1.15f,.12f,side*.48f},aero);
    }

    // Large dorsal fin, blended into the engine cover and ending at the wing mount.
    panel({-.25f,.30f,-.018f},{-1.27f,.255f,-.018f},
          {-1.10f,.49f,-.018f},{-.52f,.47f,-.018f},carbon);
    panel({-.25f,.30f,.018f},{-1.27f,.255f,.018f},
          {-1.10f,.49f,.018f},{-.52f,.47f,.018f},carbon);
    // Engine-cover cooling louvres, laid flat on the rear deck.
    for(int i=0;i<5;++i) {
        const float x=-.72f-i*.085f;
        box(x,.325f,0,.035f,.012f,.43f,carbon);
    }

    // Rear diffuser with three tunnels and vertical strakes.
    box(-1.34f,.085f,0,.30f,.045f,.94f,carbon);
    for(float side:{-1.f,0.f,1.f}) {
        box(-1.43f,.105f,side*.29f,.34f,.07f,.025f,aero);
        box(-1.25f,.105f,side*.29f,.28f,.09f,.018f,carbon);
    }

    // High, wide endurance-racing rear wing on two substantial rear-deck pylons.
    for(float side:{-1.f,1.f}) {
        rod({-.91f,.30f,side*.245f},{-1.16f,.66f,side*.30f},
            .035f,carbon);
        rod({-.91f,.30f,side*.245f},{-1.16f,.66f,side*.30f},
            .018f,{105,113,123,255});
    }
    rearWing(-1.22f,.69f,1.20f,.22f,carbon);
    // Small endplates make the wing read as a single connected aero assembly.
    for(float side:{-1.f,1.f})
        box(-1.22f,.665f,side*.615f,.24f,.15f,.025f,aero);
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
    const Color cage{39,45,52,255};
    const Color rail{61,69,77,255};
    const Color alloy{151,161,171,255};
    const Color dark{24,29,35,255};
    const Color lamp=braking?Color{255,42,35,255}:Color{235,226,184,255};
    const float half=second?1.02f:.96f;

    // Lower spaceframe: two long chassis rails, cross-members and triangulation.
    for(float side:{-1.f,1.f}) {
        rod({-1.48f,.55f,side*.58f},{1.48f,.55f,side*.58f},.060f,rail);
        rod({-1.30f,.42f,side*.88f},{1.34f,.42f,side*.88f},.052f,cage);
        rod({-1.30f,.42f,side*.88f},{-.92f,.82f,side*.58f},.045f,cage);
        rod({1.34f,.42f,side*.88f},{.98f,.82f,side*.58f},.045f,cage);
        // Main cabin cage and side impact structure.
        rod({-.92f,.72f,side*.58f},{-.52f,1.62f,side*.46f},.052f,cage);
        rod({-.52f,1.62f,side*.46f},{.50f,1.62f,side*.46f},.052f,cage);
        rod({.50f,1.62f,side*.46f},{.88f,.72f,side*.58f},.052f,cage);
        rod({-.92f,.72f,side*.58f},{.88f,.72f,side*.58f},.040f,rail);
        rod({-.78f,.58f,side*.60f},{.66f,1.45f,side*.47f},.034f,alloy);
        rod({-.78f,1.43f,side*.47f},{.66f,.58f,side*.60f},.034f,alloy);
        // Front and rear shock towers tied into the cage.
        rod({.98f,.55f,side*.58f},{.78f,1.18f,side*.50f},.052f,cage);
        rod({-1.02f,.55f,side*.58f},{-.82f,1.16f,side*.50f},.052f,cage);
    }
    for(float x:{-1.42f,-.82f,-.30f,.48f,1.30f}) {
        rod({x,.55f,-.58f},{x,.55f,.58f},.047f,rail);
    }
    // Roof, firewall and rear bulkhead cross-bracing.
    for(float x:{-.52f,.50f}) rod({x,1.62f,-.46f},{x,1.62f,.46f},.040f,cage);
    rod({-.92f,.72f,-.58f},{.88f,.72f,.58f},.034f,alloy);
    rod({-.92f,.72f,.58f},{.88f,.72f,-.58f},.034f,alloy);
    // Additional triangulated lower frame: the suspension towers are tied
    // into the main rails instead of appearing as isolated uprights.
    for(float side:{-1.f,1.f}) {
        rod({1.28f,.55f,side*.58f},{.72f,.92f,side*.58f},.046f,cage);
        rod({.72f,.92f,side*.58f},{.38f,.55f,side*.58f},.046f,cage);
        rod({-1.28f,.55f,side*.58f},{-.72f,.92f,side*.58f},.046f,cage);
        rod({-.72f,.92f,side*.58f},{-.36f,.55f,side*.58f},.046f,cage);
    }
    // A central tunnel and engine cradle complete the visible spaceframe.
    rod({-1.28f,.57f,0},{1.28f,.57f,0},.052f,rail);
    for(float x:{-.98f,-.48f,.18f,.78f})
        rod({x,.57f,-.38f},{x,.57f,.38f},.034f,alloy);

    // Reinforced front suspension pickup towers and rear trailing-arm mounts.
    for(float side:{-1.f,1.f}) {
        for(float x:{.62f,1.02f}) {
            rod({x,.50f,side*.58f},{x,.78f,side*.68f},.055f,alloy);
        }
        for(float x:{-1.30f,-.92f}) {
            rod({x,.48f,side*.58f},{x,.70f,side*.70f},.055f,alloy);
        }
    }

    // Visible mechanical package: engine, transmission, seat, dash and skid plates.    box(-.88f,.86f,0,.58f,.48f,.68f,{77,83,90,255});
    for(int fin=0;fin<6;++fin)
        box(-1.13f+fin*.09f,1.12f,0,.025f,.12f,.62f,alloy);
    box(-.10f,.91f,0,.36f,.52f,.44f,dark);
    box(.48f,.90f,0,.30f,.045f,.86f,rail);
    box(.12f,.66f,0,.32f,.06f,.72f,dark);
    box(.30f,.72f,0,.10f,.24f,.08f,alloy);
    // Protective front/rear bumpers and side rock sliders.
    rod({1.48f,.53f,-.86f},{1.48f,.53f,.86f},.075f,alloy);
    rod({-1.48f,.53f,-.86f},{-1.48f,.53f,.86f},.075f,alloy);
    for(float side:{-1.f,1.f})
        rod({-1.05f,.40f,side*.91f},{1.05f,.40f,side*.91f},.045f,cage);
    box(1.46f,.80f,0,.035f,.08f,.62f,lamp);
    // Spare wheel mounted behind the cabin.
    rod({-1.00f,1.17f,-.12f},{-1.30f,1.17f,-.12f},.30f,dark);
    rod({-1.30f,1.17f,-.12f},{-1.32f,1.17f,-.12f},.12f,alloy);
    (void)paint;(void)glass;(void)half;
}

void wheels(const Car& car,const VehicleProfile& p,int index,bool desert=false) {
    const Color carbon{22,26,32,255};
    // GT3 wheel centres sit close to the actual front and rear wheel arches.
    const float axleVisualScale=index==3?1.12f:1.0f;
    const float front=float(p.frontAxle*kWorldScale*axleVisualScale);
    const float rear=float(p.rearAxle*kWorldScale*axleVisualScale);
    constexpr std::array<float,11> sizes{{.155f,.265f,.205f,.195f,.180f,.220f,.205f,.315f,.325f,.335f,.205f}};
    constexpr std::array<float,11> widths{{.19f,.26f,.22f,.21f,.21f,.245f,.22f,.34f,.36f,.37f,.22f}};
    constexpr std::array<float,11> tracks{{.45f,.65f,.48f,.58f,.58f,.63f,.48f,.88f,.92f,.94f,.48f}};
    const std::size_t slot=std::size_t(std::clamp(index,0,10));
    const float r=sizes[slot],width=widths[slot];
    const float track=std::max(float(p.halfWheelTrack*kWorldScale),tracks[slot]);

    const bool baja=index>=7 && index<=9;
    const Vec2 forward{std::cos(car.heading),std::sin(car.heading)};
    const Vec2 lateral{-forward.y,forward.x};
    const float visualWheelRotation=float(car.wheelRotation)*(desert?4.0f:1.0f);
    for(float x:{rear,front}) for(float side:{-1.f,1.f}) {
        // Keep the original wheel track and tyre width. Only limit vertical
        // suspension travel on the GT3 so terrain height cannot push tyres
        // up through the front bodywork when steering.
        const Vec2 wheelWorld=car.position+
            forward*(x/kWorldScale)+lateral*(side*track/kWorldScale);
        float wheelHeight=float(
            ((desert?0.0:(baja?bajaElevation(wheelWorld):elevation(wheelWorld)))-
             (desert?0.0:(baja?bajaElevation(car.position):elevation(car.position))))*kWorldScale
            - (baja?car.jumpHeight*kWorldScale:0.0));
        if(slot==2) wheelHeight=std::clamp(wheelHeight,-.045f,.045f);
        // Trophy Truck long-travel suspension: deliberately generous wheel
        // movement while keeping the tyres beneath the chassis footprint.
        if(baja) wheelHeight=std::clamp(wheelHeight,-.78f,.48f);
        // In the flat Desert world, braking/acceleration still articulate the
        // suspension: the front compresses under braking, the rear under power.
        if(desert) {
            const float axleSign=x>0.0f?1.0f:-1.0f;
            wheelHeight-=float(car.jumpHeight*kWorldScale);
            wheelHeight+=std::clamp(-float(car.pitch)*axleSign*.72f,-.16f,.16f);
        }
        if(baja) {
            const float wheelY=r+.015f+wheelHeight;
            const float railZ=side*.62f;
            const float outerZ=side*(track-.015f);
            const Vector3 upperHub{x,wheelY+.17f,outerZ};
            const Vector3 lowerHub{x,wheelY-.17f,outerZ};
            const Color arm{133,145,157,255};
            const Color joint{198,205,211,255};
            const Color mount{72,82,94,255};

            if(desert && x==rear) {
                // Rear trailing arm: two longitudinal rails from a shared,
                // chassis-mounted pivot crossmember to the moving wheel upright.
                const Vector3 pivotA{x+.62f,.62f,side*.48f};
                const Vector3 pivotB{x+.62f,.62f,side*.72f};
                const Vector3 hubA{x-.48f,wheelY-.12f,outerZ-.10f*side};
                const Vector3 hubB{x-.48f,wheelY-.12f,outerZ+.10f*side};
                rod(pivotA,hubA,.065f,arm);
                rod(pivotB,hubB,.065f,arm);
                rod(hubA,hubB,.055f,arm);
                rod(upperHub,lowerHub,.050f,mount);
                // Diagonal braces close the trailing arm into a rigid triangle.
                rod(pivotA,hubB,.038f,{105,117,130,255});
                rod(pivotB,hubA,.038f,{105,117,130,255});
                DrawSphere(pivotA,.052f,joint);
                DrawSphere(pivotB,.052f,joint);
                DrawSphere(hubA,.045f,joint);
                DrawSphere(hubB,.045f,joint);
            } else {
                // Front double wishbones. Both inner pivots of each arm sit
                // on reinforced chassis brackets; both outer ends meet the upright.
                const Vector3 upperA{x-.48f,.91f,railZ};
                const Vector3 upperB{x+.48f,.91f,railZ};
                const Vector3 lowerA{x-.55f,.63f,railZ};
                const Vector3 lowerB{x+.55f,.63f,railZ};
                // Mounting brackets physically connect each pivot to the side rail.
                rod({x-.48f,.55f,side*.58f},upperA,.055f,mount);
                rod({x+.48f,.55f,side*.58f},upperB,.055f,mount);
                rod({x-.55f,.55f,side*.58f},lowerA,.055f,mount);
                rod({x+.55f,.55f,side*.58f},lowerB,.055f,mount);
                rod(upperA,upperHub,.050f,arm);
                rod(upperB,upperHub,.050f,arm);
                rod(lowerA,lowerHub,.062f,arm);
                rod(lowerB,lowerHub,.062f,arm);
                rod(upperHub,lowerHub,.052f,mount);
                for(const Vector3 pvt:{upperA,upperB,lowerA,lowerB})
                    DrawSphere(pvt,.047f,joint);
                // Steering link joins the upright to a rack mounted on the frame.
                const Vector3 steeringArm{x-.10f,wheelY-.02f,outerZ};
                const Vector3 rack{x-.34f,.57f,side*.28f};
                rod(lowerHub,steeringArm,.032f,mount);
                rod(steeringArm,rack,.026f,{194,158,91,255});
                DrawSphere(steeringArm,.030f,joint);
                DrawSphere(rack,.030f,joint);
            }
            DrawSphere(upperHub,.040f,joint);
            DrawSphere(lowerHub,.044f,joint);

            // Long-travel coilover. Both eyes are attached: upper to tower,
            // lower to the moving arm/upright assembly.
            const Vector3 shockTop{x-.22f,1.20f,side*.58f};
            const Vector3 shockBottom{x+.20f,wheelY-.12f,side*(track-.18f)};
            const Vector3 axis{shockBottom.x-shockTop.x,
                               shockBottom.y-shockTop.y,
                               shockBottom.z-shockTop.z};
            const float len=std::max(.001f,std::sqrt(axis.x*axis.x+
                                  axis.y*axis.y+axis.z*axis.z));
            const Vector3 n{axis.x/len,axis.y/len,axis.z/len};
            Vector3 u{n.y,-n.x,0};
            const float ul=std::sqrt(u.x*u.x+u.y*u.y);
            if(ul>.001f){u.x/=ul;u.y/=ul;} else u={1,0,0};
            rod(shockTop,shockBottom,.032f,{65,77,92,255});
            for(int i=0;i<64;++i) {
                const float t0=float(i)/64.f,t1=float(i+1)/64.f;
                const float a0=t0*9.f*2*PI,a1=t1*9.f*2*PI;
                const auto spring=[&](float t,float a) {
                    return Vector3{
                        shockTop.x+axis.x*t+u.x*std::cos(a)*.052f,
                        shockTop.y+axis.y*t+u.y*std::cos(a)*.052f,
                        shockTop.z+axis.z*t+std::sin(a)*.052f};
                };
                DrawLine3D(spring(t0,a0),spring(t1,a1),{25,115,255,255});
            }
            DrawSphere(shockTop,.060f,joint);
            DrawSphere(shockBottom,.055f,joint);

            if(desert) {
                // Bypass damper mounted alongside the coilover, with both ends
                // attached to the same chassis tower and moving suspension arm.
                const Vector3 bypassTop{x-.43f,1.10f,side*.58f};
                const Vector3 bypassBottom{x+.34f,wheelY-.20f,side*(track-.08f)};
                rod(bypassTop,bypassBottom,.040f,{185,133,54,255});
                DrawSphere(bypassTop,.047f,joint);
                DrawSphere(bypassBottom,.043f,joint);
                // Reservoir fixed to the chassis tower and hose linked to bypass.
                const Vector3 reservoir{x-.62f,1.02f,side*.70f};
                rod({reservoir.x,reservoir.y-.15f,reservoir.z},
                    {reservoir.x,reservoir.y+.15f,reservoir.z},.052f,
                    {104,116,130,255});
                rod(bypassTop,reservoir,.018f,{45,53,63,255});
                // Bump stop is attached to the tower; limit strap joins tower
                // and moving arm, so neither detail floats independently.
                const Vector3 bumpTop{x-.18f,.96f,side*.58f};
                const Vector3 bumpBottom{x-.18f,.78f,side*.58f};
                rod(bumpTop,bumpBottom,.040f,{215,177,73,255});
                rod({x+.12f,.96f,side*.60f},
                    {x+.28f,wheelY-.08f,side*(track-.04f)},.014f,
                    {125,91,58,255});
            }
        }
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
        // The tyre centre follows the same long-travel position as its upright.
        rlTranslatef(x,r+.015f+wheelHeight,side*track);
        if(x==front) rlRotatef(float(-car.steering*RAD2DEG),0,1,0);
        DrawCylinderEx({0,0,-width*.5f},{0,0,width*.5f},r,r,slot==2||slot==3?24:16,
                       {27,30,36,255});
        // Fine circumferential grooves and angled tread cuts add rubber detail.
        for(int band=0;band<5;++band) {
            const float treadZ=-width*.38f+float(band)*width*.19f;
            for(int segment=0;segment<32;++segment) {
                const float a0=2*PI*float(segment)/32.f+visualWheelRotation;
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
            const float a=visualWheelRotation+spoke*1.2566371f;
            DrawLine3D({std::cos(a)*r*.2f,std::sin(a)*r*.2f,z+side*.028f},
                       {std::cos(a)*r*.48f,std::sin(a)*r*.48f,z+side*.028f},
                       {180,185,193,255});
        }
        rlPopMatrix();
    }
}

} // namespace

void VehicleRenderer::draw(const Car& car,const VehicleProfile& profile,
                           int index,int colorIndex,bool night,bool desert) const {
    // Desert uses the Trophy Truck model and its articulated long-travel suspension.

    const bool baja=index>=7 && index<=9;
    const Vector3 p=worldPoint(car.position,float(car.jumpHeight),baja,false);
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
    case 6: gt3Monster(glass,car.brake>.1);break;
    case 10: gt3RedBull(glass,car.brake>.1);break;
    case 3:
        rlPushMatrix();
        rlTranslatef(0,.035f,0);
        rlScalef(.96f,.96f,1.0f);
        wec(paint,glass,dark,car.brake>.1);
        rlPopMatrix();
        break;
    case 5:
        rlPushMatrix();
        rlScalef(.88f,.90f,.90f);
        formulaOne(paint,glass,car.brake>.1);
        rlPopMatrix();
        break;
    case 9:
        trophyTruck(paint,glass,car.brake>.1,true);
        break;
    case 7:
        trophyTruck(paint,glass,car.brake>.1,true);
        break;
    case 8:
        trophyTruck(paint,glass,car.brake>.1,true);
        break;
    case 4:
        rlPushMatrix();
        rlScalef(.84f,.84f,.84f);
        rally(paint,glass,dark,car.brake>.1);
        rlPopMatrix();
        break;
    default: gt3(paint,glass,dark,car.brake>.1);break;
    }
    const float highlightY=index==1?.82f:index==2?.445f:index==3?.285f:index==4?.80f:index==5?.39f:index==6?.445f:index==10?.445f:index>=7?.94f:.205f;
    const float highlightZ=index==1?.47f:index==2?.49f:index==3?.34f:index==4?.48f:index==5?.36f:index==6?.49f:index==10?.49f:index>=7?.62f:.20f;
    for(float side:{-1.f,1.f}) {        DrawLine3D({.62f,highlightY,side*highlightZ},
                   {-.48f,highlightY-.012f,side*highlightZ*.94f},
                   {255,255,255,36});
    }
    wheels(car,profile,index,desert);
    rlPopMatrix();
    rlPopMatrix();
}

} // namespace karting