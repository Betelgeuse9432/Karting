#include "karting/atmosphere.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace karting {
namespace {
// The raylib immediate-mode batch transforms submitted vertices to world
// coordinates on the CPU. Derivative normals therefore work even with
// procedural primitives which do not supply useful vertex normals.
constexpr const char* vertexSource=R"GLSL(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec3 worldPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
void main() {
    worldPosition=vertexPosition;
    fragTexCoord=vertexTexCoord;
    fragColor=vertexColor;
    gl_Position=mvp*vec4(vertexPosition,1.0);
}
)GLSL";

constexpr const char* fragmentSource=R"GLSL(#version 330
in vec3 worldPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 eyePosition;
uniform vec3 sunDirection;
uniform vec3 horizonColor;
uniform vec3 lighting; // daylight, dusk warmth, rain
uniform vec2 fogRange;
out vec4 finalColor;
void main() {
    vec4 base=texture(texture0,fragTexCoord)*colDiffuse*fragColor;
    vec3 crossNormal=cross(dFdx(worldPosition),dFdy(worldPosition));
    float normalLength=length(crossNormal);
    vec3 normal=normalLength>0.000001?crossNormal/normalLength:vec3(0.0,1.0,0.0);
    float facing=max(dot(normal,sunDirection),0.0);
    // Existing vertex colours already incorporate night and rain. Add
    // directional contrast, not a second full day/night multiplier.
    float strength=lighting.x*(1.0-0.75*lighting.z);
    float shade=mix(1.0,0.74+0.40*facing,strength);
    vec3 warm=mix(vec3(1.0),vec3(1.13,0.97,0.79),lighting.y*facing);
    vec3 cool=mix(vec3(1.0),vec3(0.84,0.91,1.08),lighting.y*(1.0-facing)*0.5);
    vec3 lit=base.rgb*shade*warm*cool;
    float distanceToEye=length(worldPosition-eyePosition);
    float haze=smoothstep(fogRange.x,fogRange.y,distanceToEye);
    finalColor=vec4(mix(lit,horizonColor,haze),base.a);
}
)GLSL";
} // namespace

void Atmosphere::initialize() {
    shader_=LoadShaderFromMemory(vertexSource,fragmentSource);
    // raylib returns the default shader if compilation or linking fails.
    ready_=shader_.id!=0 && shader_.id!=rlGetShaderIdDefault();
    if(!ready_) return;
    eye_=GetShaderLocation(shader_,"eyePosition");
    sun_=GetShaderLocation(shader_,"sunDirection");
    horizon_=GetShaderLocation(shader_,"horizonColor");
    lighting_=GetShaderLocation(shader_,"lighting");
    fog_=GetShaderLocation(shader_,"fogRange");
    if(eye_<0||sun_<0||horizon_<0||lighting_<0||fog_<0) shutdown();
}

void Atmosphere::shutdown() {
    if(ready_) UnloadShader(shader_);
    ready_=false;
    shader_={};
}

Vector3 Atmosphere::sunDirection(float night) {
    const float dusk=std::clamp(night/.55f,0.f,1.f);
    return Vector3Normalize({-.68f,.78f-.65f*dusk,-.48f});
}

void Atmosphere::begin(const Camera3D& camera,Color horizon,float night,float rain,int quality) const {
    if(!ready_) return;
    BeginShaderMode(shader_);
    const Vector3 eye=camera.position;
    const Vector3 sun=sunDirection(night);
    const Vector3 fogColor{horizon.r/255.f,horizon.g/255.f,horizon.b/255.f};
    const float warm=std::max(0.f,1.f-std::abs(night-.48f)/.48f);
    const Vector3 light{1.f-.82f*night,warm*(1.f-rain),rain};
    const float end=(quality==0?63.f:quality==1?82.f:102.f)*(1.f-.36f*rain);
    const Vector2 range{end*(.34f-.15f*rain),end};
    SetShaderValue(shader_,eye_,&eye,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,sun_,&sun,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,horizon_,&fogColor,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,lighting_,&light,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,fog_,&range,SHADER_UNIFORM_VEC2);
}
void Atmosphere::end() const {
    if(ready_) EndShaderMode();
}

void drawContactShadow(const Track& track,const Car& car,const VehicleProfile& spec,
                       float night,float rain) {
    const Vec2 forward{std::cos(car.heading),std::sin(car.heading)};
    const Vec2 side{-forward.y,forward.x};
    // Soft layered contact shadow on the current deck, never the road
    // underneath the bridge. Each vertex follows its local road elevation.
    const double length=std::max(38.0,(spec.frontAxle-spec.rearAxle)*.82);
    const double width=std::max(27.0,spec.halfWheelTrack*1.65);
    const auto point=[&](double x,double y) {
        const Vec2 p=car.position+forward*x+side*y;
        return Vector3{float(p.x*kWorldScale),
                       float((track.roadHeightAt(p,car.roadIndex)+2.6)*kWorldScale),
                       float(p.y*kWorldScale)};
    };
    const Vector3 center=point(0,0);
    for(int ring=0;ring<3;++ring) {
        const double factor=1.0-.18*ring;
        const float opacity=(.075f+.035f*ring)*(1.f-.48f*night)*(1.f-.30f*rain);
        const Color color=Fade({17,22,29,255},opacity);
        constexpr int segments=20;
        for(int i=0;i<segments;++i) {
            const double a=2*kPi*i/segments,b=2*kPi*(i+1)/segments;
            const Vector3 p=point(length*factor*std::cos(a),width*factor*std::sin(a));
            const Vector3 q=point(length*factor*std::cos(b),width*factor*std::sin(b));
            // Positive X/Z winding faces down in Y; draw the upper face.
            DrawTriangle3D(center,q,p,color);
        }
    }
}
} // namespace karting
