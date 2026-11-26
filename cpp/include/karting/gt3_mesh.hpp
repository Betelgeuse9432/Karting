#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace karting::gt3model {

// Original fictional GT racer. Front is +X, right is +Z, in metres.
// Visual dimensions are independent of the arcade collision geometry.
struct Point { float x,y,z; };
enum class Finish { Paint, Accent, Glass, Carbon, Grille, Headlamp, Tail, Metal };
struct Face { std::array<std::uint16_t,3> index; Finish finish; };
struct Geometry {
    std::vector<Point> vertices;
    std::vector<Face> faces;
    std::uint16_t add(Point p) {
        vertices.push_back(p);
        return static_cast<std::uint16_t>(vertices.size()-1);
    }
    void tri(std::uint16_t a,std::uint16_t b,std::uint16_t c,Finish f,Point outside) {
        const auto p=vertices[a],q=vertices[b],r=vertices[c];
        const Point u{q.x-p.x,q.y-p.y,q.z-p.z},v{r.x-p.x,r.y-p.y,r.z-p.z};
        const Point n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
        if(n.x*outside.x+n.y*outside.y+n.z*outside.z<0) std::swap(b,c);
        faces.push_back({{a,b,c},f});
    }
    void quad(std::uint16_t a,std::uint16_t b,std::uint16_t c,std::uint16_t d,
              Finish f,Point outside) {
        tri(a,b,c,f,outside);tri(a,c,d,f,outside);
    }
    void panel(Point a,Point b,Point c,Point d,Finish f,Point outside) {
        quad(add(a),add(b),add(c),add(d),f,outside);
    }
};
inline constexpr float axle=0.76f;
inline constexpr float wheelCenterZ=0.60f;
inline constexpr float wheelCenterY=0.22f;
inline constexpr float wheelRadius=0.205f;
inline constexpr float archRadius=0.265f;
inline constexpr Point lampAnchor{1.153f,.355f,.48f};
inline float archFloor(float x) {
    const float offset=std::min(std::abs(x-axle),std::abs(x+axle));
    if(offset>=archRadius) return .115f;
    // Smooth at the lower ends and form a genuine semicircular cutout.
    const float arc=std::sqrt(std::max(0.f,archRadius*archRadius-offset*offset));
    return .115f+arc+(.105f)*std::clamp(arc/.09f,0.f,1.f);
}
struct Station { float x,center,shoulder,width; };

inline Geometry build() {
    Geometry m;
    // Stations at wheel-arch endpoints and subdivisions make the arch an
    // actual boundary of the body, not a black decal over a solid panel.
    constexpr std::array<Station,23> stations{{
        {-1.25f,.35f,.36f,.58f},{-1.16f,.44f,.46f,.67f},
        {-1.025f,.47f,.51f,.705f},{-.99f,.48f,.55f,.725f},
        {-.94f,.49f,.57f,.742f},{-.89f,.50f,.59f,.75f},
        {-.76f,.51f,.60f,.76f},{-.63f,.51f,.585f,.75f},
        {-.58f,.51f,.56f,.73f},{-.53f,.51f,.53f,.70f},
        {-.495f,.51f,.51f,.675f},{-.30f,.50f,.49f,.64f},
        { .02f,.48f,.47f,.625f},{ .30f,.46f,.48f,.65f},
        { .495f,.44f,.51f,.685f},{ .53f,.44f,.54f,.705f},
        { .58f,.43f,.56f,.73f},{ .63f,.43f,.58f,.75f},
        { .76f,.415f,.59f,.76f},{ .89f,.40f,.56f,.75f},
        { .99f,.385f,.49f,.72f},{1.025f,.38f,.46f,.69f},
        {1.16f,.33f,.39f,.62f}
    }};
    // Each cross-section shares a central ridge; shoulder and beltline
    // give the fenders their volume. Lower side edge follows the arches.
    using Ring=std::array<std::uint16_t,9>;
    std::vector<Ring> rings;
    for(const auto& s:stations) {
        const float lower=archFloor(s.x);
        const float belt=std::max(lower+.036f,s.shoulder-.09f);
        const float top=s.shoulder;
        rings.push_back({
            m.add({s.x,lower,-s.width*.97f}),
            m.add({s.x,belt,-s.width}),
            m.add({s.x,top,-s.width*.78f}),
            m.add({s.x,s.center+.015f,-s.width*.43f}),
            m.add({s.x,s.center,0}),
            m.add({s.x,s.center+.015f,s.width*.43f}),
            m.add({s.x,top,s.width*.78f}),
            m.add({s.x,belt,s.width}),
            m.add({s.x,lower,s.width*.97f})
        });
    }
    for(std::size_t i=1;i<rings.size();++i) {
        const auto& a=rings[i-1];const auto& b=rings[i];
        for(int j=0;j<8;++j) {
            const Point normal{0,j==0||j==7?.12f:1.f,j<4?-1.f:1.f};
            m.quad(a[j],b[j],b[j+1],a[j+1],Finish::Paint,normal);
        }
        // Narrow dark inner lip, open underneath. No solid face spans
        // the wheel opening. The tyres remain visible and steer freely.
        for(int j:{0,8}) {
            const auto p=m.vertices[a[j]],q=m.vertices[b[j]];
            Point pp=p,qq=q;pp.z*=.88f;qq.z*=.88f;
            m.quad(a[j],b[j],m.add(qq),m.add(pp),Finish::Carbon,{0,-1,0});
        }
    }
    // Front and rear fascia close each end using a triangulated fan.
    for(int end:{0,1}) {
        const auto& r=end?rings.back():rings.front();
        const auto s=end?stations.back():stations.front();
        const auto center=m.add({s.x,.19f,0});
        for(int j=0;j<8;++j)
            m.tri(center,r[j],r[j+1],Finish::Paint,{end?1.f:-1.f,0,0});
        m.tri(center,r[8],r[0],Finish::Carbon,{end?1.f:-1.f,0,0});
    }

    // Cabin: multi-section roof, laid-back windshield, rear fastback.
    struct Cabin { float x,baseY,baseZ,roofY,roofZ; };
    constexpr std::array<Cabin,6> cabin{{
        {-.91f,.485f,.38f,.53f,.31f},
        {-.63f,.50f,.42f,.775f,.30f},
        {-.37f,.49f,.44f,.875f,.315f},
        {-.08f,.475f,.43f,.87f,.31f},
        { .13f,.46f,.40f,.79f,.29f},
        { .48f,.435f,.35f,.47f,.30f}
    }};
    std::vector<std::array<std::uint16_t,5>> cab;
    for(auto s:cabin) cab.push_back({
        m.add({s.x,s.baseY,-s.baseZ}),m.add({s.x,s.roofY,-s.roofZ}),
        m.add({s.x,s.roofY+.026f,0}),m.add({s.x,s.roofY,s.roofZ}),
        m.add({s.x,s.baseY,s.baseZ})});
    for(std::size_t i=1;i<cab.size();++i) {
        const auto a=cab[i-1],b=cab[i];
        for(int j=0;j<4;++j) {
            const bool roof=j==1||j==2;
            const bool windshield=i==1||i==5;
            m.quad(a[j],b[j],b[j+1],a[j+1],
                   roof&&!windshield?Finish::Paint:Finish::Glass,
                   {0,1,j<2?-1.f:1.f});
        }
    }
    for(int end:{0,1}) {
        const auto r=end?cab.back():cab.front();
        for(int j=1;j<4;++j)
            m.tri(r[0],r[j],r[j+1],Finish::Glass,{end?1.f:-1.f,0,0});
    }
    // A-pillar/B-pillar graphics hug the glass rather than sitting as rods.
    for(float side:{-1.f,1.f}) {
        m.panel({.48f,.438f,side*.352f},{.45f,.438f,side*.363f},
                {.106f,.79f,side*.303f},{.13f,.792f,side*.293f},
                Finish::Paint,{0,0,side});
        m.panel({-.34f,.49f,side*.442f},{-.385f,.49f,side*.442f},
                {-.39f,.876f,side*.318f},{-.345f,.876f,side*.318f},
                Finish::Carbon,{0,0,side});
        // Angular front lamps sit on the nose, with a dark surround.
        m.panel({1.162f,.303f,side*.34f},{1.162f,.303f,side*.59f},
                {1.162f,.375f,side*.54f},{1.162f,.355f,side*.34f},
                Finish::Grille,{1,0,0});
        m.panel({1.164f,.33f,side*.36f},{1.164f,.328f,side*.565f},
                {1.164f,.358f,side*.53f},{1.164f,.35f,side*.36f},
                Finish::Headlamp,{1,0,0});
        m.panel({-1.252f,.282f,side*.28f},{-1.252f,.282f,side*.54f},
                {-1.252f,.322f,side*.54f},{-1.252f,.322f,side*.28f},
                Finish::Tail,{-1,0,0});
        // Side air intakes, inset into the rocker contour.
        m.panel({-.43f,.17f,side*.671f},{-.18f,.17f,side*.644f},
                {-.21f,.31f,side*.649f},{-.43f,.37f,side*.671f},
                Finish::Grille,{0,0,side});
        // Sculpted splitter and rocker plates.
        m.panel({-.49f,.105f,side*.68f},{.49f,.105f,side*.69f},
                {.47f,.105f,side*.75f},{-.47f,.105f,side*.75f},
                Finish::Carbon,{0,1,0});
        // Twin wing supports, swept and tapered, not cylindrical rods.
        const float z=side*.36f;
        m.panel({-1.01f,.46f,z},{-.93f,.49f,z},
                {-1.10f,.78f,z},{-1.20f,.78f,z},Finish::Carbon,{0,0,side});
        m.panel({-1.01f,.46f,z-.012f},{-.93f,.49f,z-.012f},
                {-1.10f,.78f,z-.012f},{-1.20f,.78f,z-.012f},
                Finish::Carbon,{0,0,-side});
        // Large sculpted endplates, deliberately below the roofline.
        m.panel({-1.32f,.75f,side*.73f},{-.98f,.77f,side*.73f},
                {-1.01f,.85f,side*.73f},{-1.28f,.87f,side*.73f},
                Finish::Carbon,{0,0,side});
        m.panel({-1.32f,.75f,side*.725f},{-.98f,.77f,side*.725f},
                {-1.01f,.85f,side*.725f},{-1.28f,.87f,side*.725f},
                Finish::Carbon,{0,0,-side});
    }
    m.panel({1.163f,.15f,-.36f},{1.163f,.15f,.36f},
            {1.163f,.285f,.29f},{1.163f,.285f,-.29f},Finish::Grille,{1,0,0});
    m.panel({-1.253f,.15f,-.48f},{-1.253f,.15f,.48f},
            {-1.253f,.25f,.38f},{-1.253f,.25f,-.38f},Finish::Grille,{-1,0,0});
    m.panel({1.04f,.105f,-.69f},{1.25f,.105f,-.62f},
            {1.25f,.105f,.62f},{1.04f,.105f,.69f},Finish::Carbon,{0,1,0});
    m.panel({-1.36f,.11f,-.58f},{-1.04f,.11f,-.64f},
            {-1.04f,.11f,.64f},{-1.36f,.11f,.58f},Finish::Carbon,{0,1,0});
    // Wing airfoil with actual upper and lower surfaces.
    m.panel({-1.30f,.825f,-.73f},{-1.00f,.785f,-.73f},
            {-1.00f,.785f,.73f},{-1.30f,.825f,.73f},Finish::Carbon,{0,1,0});
    m.panel({-1.30f,.808f,-.73f},{-1.00f,.77f,-.73f},
            {-1.00f,.77f,.73f},{-1.30f,.808f,.73f},Finish::Carbon,{0,-1,0});
    m.panel({-1.30f,.808f,-.73f},{-1.30f,.825f,-.73f},
            {-1.30f,.825f,.73f},{-1.30f,.808f,.73f},Finish::Carbon,{-1,0,0});
    m.panel({-1.00f,.77f,-.73f},{-1.00f,.785f,-.73f},
            {-1.00f,.785f,.73f},{-1.00f,.77f,.73f},Finish::Carbon,{1,0,0});
    // Two faceted hood vents follow the bonnet instead of floating above it.
    for(float side:{-1.f,1.f})
        m.panel({.55f,.45f,side*.15f},{.88f,.418f,side*.15f},
                {.91f,.453f,side*.31f},{.56f,.49f,side*.31f},
                Finish::Grille,{0,1,0});
    return m;
}
} // namespace karting::gt3model
