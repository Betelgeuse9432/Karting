#include "karting/game.hpp"
#include "karting/gt3_mesh.hpp"
#include "rlgl.h"
#include "raymath.h"
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

Color frontLight(Color color,bool enabled) {
    return enabled?surface(color,Surface::Headlamp):color;
}

void kart(Color paint,bool lights) {
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
    // Small auxiliary lights on the kart's front bumper.
    lamps(.808f,.19f,.25f,frontLight({255,220,160,255},lights),.10f);
    box(-.77f,.14f,0,.068f,.10f,.73f,rubber);
}

void offRoad(Color paint,Color glass,Color dark,bool braking,bool lights) {
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
    lamps(1.22f,.69f,.36f,frontLight({255,218,155,255},lights),.17f);
    lamps(-1.13f,.70f,.36f,braking?RED:Color{139,30,32,255},.16f);
    rod({-1.16f,.87f,-.10f},{-1.28f,.87f,-.10f},
        .30f,{40,42,46,255}); // spare
    rod({-1.30f,.87f,-.10f},{-1.32f,.87f,-.10f},
        .13f,{163,166,172,255});
}

void gt3(Color paint,Color glass,Color /*dark*/,bool braking,bool lights) {
    // Cached indexed mesh, submitted through the existing rlgl batch.
    // This keeps the world-space atmospheric shader, depth pass and
    // vertex-alpha material tags compatible with all other geometry.
    static const auto model=gt3model::build();
    using gt3model::Finish;
    const auto material=[&](Finish finish) -> Color {
        switch(finish) {
        case Finish::Paint: return paint;
        case Finish::Accent: return tone(paint,.78f);
        case Finish::Glass: return glass;
        case Finish::Carbon: return {27,30,36,255};
        case Finish::Grille: return {15,19,25,255};
        case Finish::Headlamp: return frontLight({255,226,174,255},lights);
        case Finish::Tail: return braking?Color{255,46,39,255}:Color{163,27,34,255};
        case Finish::Metal: return {145,153,161,255};
        }
        return paint;
    };
    // Reserve a full triangle at a time, preserving raylib batch limits.
    rlSetTexture(rlGetTextureIdDefault());
    for(const auto& face:model.faces) {
        rlCheckRenderBatchLimit(3);
        const auto color=material(face.finish);
        const auto a=model.vertices[face.index[0]],b=model.vertices[face.index[1]],
                   c=model.vertices[face.index[2]];
        const Vector3 normal=Vector3Normalize(Vector3CrossProduct(
            {b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z}));
        rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r,color.g,color.b,color.a);
        rlNormal3f(normal.x,normal.y,normal.z);
        rlTexCoord2f(0,0);
        for(const auto index:face.index) {
            const auto p=model.vertices[index];
            rlVertex3f(p.x,p.y,p.z);
        }
        rlEnd();
    }
}

void wec(Color paint,Color glass,Color /*dark*/,bool braking,bool lights) {
    // Simple WEC Hypercar silhouette. Front is +X, rear is -X.
    const Color carbon{24,28,34,255};
    const Color glassDark=tone(glass,.72f);
    const Color headlamp=frontLight({255,228,180,255},lights);
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
    const float front=index==2?gt3model::axle:float(p.frontAxle*kWorldScale);
    const float rear=index==2?-gt3model::axle:float(p.rearAxle*kWorldScale);
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

struct RoadPose { Vector3 origin; float pitch,roll; };
RoadPose roadPose(const Car& car,const VehicleProfile& profile,int index,
                  bool followRoad,const Track* track) {
    const Vec2 forward{std::cos(car.heading),std::sin(car.heading)};
    const Vec2 side{-forward.y,forward.x};
    // Sample the four wheel contact locations. Compute the road plane in
    // local car coordinates (X forward, Z right), not just the height under
    // the vehicle centre. The whole car, including wheels, follows it.
    double originHeight=track?car.height:elevation(car.position);
    float roadPitch=0.f,roadRoll=0.f;
    if (followRoad) {
        const double wheelTrack=std::max(profile.halfWheelTrack,
                                        (index==2?.60:index==1?.65:index==3?.58:.45)
                                            /kWorldScale);
        const auto contact=[&](double axle,double lateral) {
            const Vec2 point=car.position+forward*axle+side*lateral;
            return track?track->roadHeightAt(point,car.roadIndex):elevation(point);
        };
        // Sample the visual wheelbase for the visual road plane only.
        // Arcade physics continues to use the untouched profile axles.
        const double frontAxle=index==2?gt3model::axle/kWorldScale:profile.frontAxle;
        const double rearAxle=index==2?-gt3model::axle/kWorldScale:profile.rearAxle;
        const double front=(contact(frontAxle,-wheelTrack)+contact(frontAxle,wheelTrack))*.5;
        const double rear=(contact(rearAxle,-wheelTrack)+contact(rearAxle,wheelTrack))*.5;
        const double left=(contact(frontAxle,-wheelTrack)+contact(rearAxle,-wheelTrack))*.5;
        const double right=(contact(frontAxle,wheelTrack)+contact(rearAxle,wheelTrack))*.5;
        const double longitudinalGrade=(front-rear)/
                              (frontAxle-rearAxle);
        const double transverseGrade=(right-left)/(2*wheelTrack);
        // Intercept at the vehicle centre: avoids height drift when the
        // front/rear axle offsets are not symmetric.
        originHeight=(front+rear)*.5-
                     longitudinalGrade*(frontAxle+rearAxle)*.5;
        roadPitch=float(std::atan(longitudinalGrade)*RAD2DEG);
        roadRoll=float(-std::atan(transverseGrade)*RAD2DEG);
    }
    const Vector3 p{float(car.position.x*kWorldScale),
                    float(originHeight*kWorldScale),
                    float(car.position.y*kWorldScale)};
    return {p,roadPitch,roadRoll};
}
Vector3 orient(Vector3 local,const Car& car,const RoadPose& pose) {
    // Match the actual model's matrix order: chassis pitch, chassis roll,
    // road pitch, road roll and finally yaw.
    local=Vector3RotateByAxisAngle(local,{0,0,1},float(car.pitch));
    local=Vector3RotateByAxisAngle(local,{1,0,0},float(car.roll));
    local=Vector3RotateByAxisAngle(local,{0,0,1},pose.pitch*DEG2RAD);
    local=Vector3RotateByAxisAngle(local,{1,0,0},pose.roll*DEG2RAD);
    return Vector3RotateByAxisAngle(local,{0,1,0},float(-car.heading));
}

} // namespace

Headlights VehicleRenderer::headlights(const Car& car,const VehicleProfile& profile,int index,
                                       const Track& track,HeadlightMode mode,float night,float rain) const {
    Headlights lights;
    if(mode==HeadlightMode::Off) return lights;
    lights.high=mode==HeadlightMode::High;
    lights.intensity=mode==HeadlightMode::Auto?
        std::max(std::clamp((night-.23f)/.32f,0.f,1.f),rain*.85f):1.f;
    if(lights.intensity<=.001f) return lights;
    const auto pose=roadPose(car,profile,index,true,&track);
    // Source positions match the visible models, including their body scale.
    const std::array<Vector3,4> origins{{
        {.84f,.19f,.25f},{1.25f,.69f,.36f},
        {gt3model::lampAnchor.x+.02f,gt3model::lampAnchor.y,gt3model::lampAnchor.z},
        {1.43f*.96f,.035f+.16f*.96f,.205f}
    }};
    const auto local=origins[std::clamp(index,0,3)];
    lights.up=Vector3Normalize(orient({0,1,0},car,pose));
    for(int i=0;i<2;++i) {
        const float side=i==0?-1.f:1.f;
        const Vector3 p=Vector3Add(pose.origin,orient({local.x,local.y,local.z*side},car,pose));
        const Vector3 direction=Vector3Normalize(orient({1.f,lights.high?-.014f:-.065f,side*.025f},car,pose));
        lights.positions[i]={p.x,p.y,p.z,lights.intensity};
        lights.directions[i]={direction.x,direction.y,direction.z,0.f};
    }
    return lights;
}

void VehicleRenderer::draw(const Car& car,const VehicleProfile& profile,
                           int index,int colorIndex,bool night,
                           bool followRoad,const Track* track,bool headlights) const {
    const Color paint=followRoad?surface(palette(colorIndex),Surface::Paint):palette(colorIndex);
    const Color dark{31,34,42,255};
    const Color glassBase=night?Color{29,48,70,255}:Color{58,104,139,255};
    const Color glass=followRoad?surface(glassBase,Surface::Glass):glassBase;
    const auto pose=roadPose(car,profile,index,followRoad,track);
    const Vector3 p=pose.origin;
    const float roadPitch=pose.pitch,roadRoll=pose.roll;
    rlPushMatrix();
    rlTranslatef(p.x,p.y,p.z);
    rlRotatef(float(-car.heading*RAD2DEG),0,1,0);
    rlRotatef(roadRoll,1,0,0);
    rlRotatef(roadPitch,0,0,1);
    // Suspension motion applies to the chassis, not the wheel contacts.
    rlPushMatrix();
    rlRotatef(float(car.roll*RAD2DEG),1,0,0);
    rlRotatef(float(car.pitch*RAD2DEG),0,0,1);
    switch(index) {
    case 0: kart(paint,headlights);break;
    case 1: offRoad(paint,glass,dark,car.brake>.1,headlights);break;
    case 2: gt3(paint,glass,dark,car.brake>.1,headlights);break;
    case 3:
        // Nathan's adjusted hypercar body: wheels and driving physics
        // retain their original reference positions.
        rlPushMatrix();
        rlTranslatef(0,.035f,0);
        rlScalef(.96f,.96f,1.0f);
        wec(paint,glass,dark,car.brake>.1,headlights);
        rlPopMatrix();
        break;
    default: gt3(paint,glass,dark,car.brake>.1,headlights);break;
    }
    rlPopMatrix();
    wheels(car,profile,index);
    rlPopMatrix();
}

} // namespace karting
