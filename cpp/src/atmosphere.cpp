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

// Render linear light-space depth into an RGB8 color attachment.
constexpr const char* depthFragment=R"GLSL(#version 330
out vec4 finalColor;
void main() {
    float depth=min(gl_FragCoord.z,0.999999);
    vec3 packedDepth=fract(depth*vec3(1.0,255.0,65025.0));
    packedDepth-=packedDepth.yzz*vec3(1.0/255.0,1.0/255.0,0.0);
    finalColor=vec4(packedDepth,1.0);
}
)GLSL";

constexpr int shadowSize=4096;
constexpr int shadowSlot=10;
constexpr float shadowWidth=100.0f;

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
uniform sampler2D sunShadowMap;
uniform mat4 lightVP;
uniform vec3 shadowParams; // inverse size, strength, valid
out vec4 finalColor;

float sunVisibility(vec3 position,float facing) {
    if(shadowParams.z<0.5) return 1.0;
    vec4 clip=lightVP*vec4(position,1.0);
    vec3 uvz=clip.xyz/clip.w*0.5+0.5;
    if(any(lessThanEqual(uvz,vec3(0.0)))||any(greaterThanEqual(uvz,vec3(1.0)))) return 1.0;
    float bias=0.000045+0.00013*(1.0-facing);
    float sum=0.0;
    for(int y=-2;y<=2;++y) for(int x=-2;x<=2;++x) {
        vec2 uv=uvz.xy+vec2(x,y)*shadowParams.x*1.5;
        vec3 encoded=texture(sunShadowMap,uv).rgb;
        float depth=dot(encoded,vec3(1.0,1.0/255.0,1.0/65025.0));
        sum+=uvz.z-bias<=depth?1.0:0.0;
    }
    float edge=min(min(uvz.x,1.0-uvz.x),min(uvz.y,1.0-uvz.y));
    return mix(1.0,sum/25.0,shadowParams.y*smoothstep(0.01,0.07,edge));
}
void main() {
    vec4 base=texture(texture0,fragTexCoord)*colDiffuse*fragColor;
    vec3 crossNormal=cross(dFdx(worldPosition),dFdy(worldPosition));
    float normalLength=length(crossNormal);
    vec3 normal=normalLength>0.000001?crossNormal/normalLength:vec3(0.0,1.0,0.0);
    float facing=max(dot(normal,sunDirection),0.0);
    // Existing vertex colours already incorporate night and rain. Add
    // directional contrast, not a second full day/night multiplier.
    float strength=lighting.x*(1.0-0.75*lighting.z);
    float visibility=sunVisibility(worldPosition,facing);
    float shade=mix(1.0,0.60+0.54*facing*visibility,strength);
    shade*=mix(0.76,1.0,visibility);
    vec3 warm=mix(vec3(1.0),vec3(1.13,0.97,0.79),lighting.y*facing*visibility);
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
    shadowSampler_=GetShaderLocation(shader_,"sunShadowMap");
    lightMatrix_=GetShaderLocation(shader_,"lightVP");
    shadowParams_=GetShaderLocation(shader_,"shadowParams");
    if(eye_<0||sun_<0||horizon_<0||lighting_<0||fog_<0 ||
       shadowSampler_<0||lightMatrix_<0||shadowParams_<0) {
        shutdown();
        return;
    }
    depthShader_=LoadShaderFromMemory(vertexSource,depthFragment);
    if(depthShader_.id!=0 && depthShader_.id!=rlGetShaderIdDefault()) {
        shadowMap_=LoadRenderTexture(shadowSize,shadowSize);
        shadowReady_=shadowMap_.id!=0 && shadowMap_.texture.id!=0 && shadowMap_.depth.id!=0;
        if(shadowReady_) {
            SetTextureFilter(shadowMap_.texture,TEXTURE_FILTER_POINT);
            SetTextureWrap(shadowMap_.texture,TEXTURE_WRAP_CLAMP);
        }
    }
}

void Atmosphere::shutdown() {
    if(shadowMap_.id) UnloadRenderTexture(shadowMap_);
    if(depthShader_.id && depthShader_.id!=rlGetShaderIdDefault()) UnloadShader(depthShader_);
    shadowMap_={};
    depthShader_={};
    shadowReady_=shadowValid_=false;
    if(ready_) UnloadShader(shader_);
    ready_=false;
    shader_={};
}

Vector3 Atmosphere::sunDirection(float night) {
    const float dusk=std::clamp(night/.55f,0.f,1.f);
    return Vector3Normalize({-.68f,.78f-.65f*dusk,-.48f});
}

bool Atmosphere::beginShadow(const Camera3D& camera,float night,float rain) {
    shadowValid_=false;
    if(!ready_||!shadowReady_||night>.88f||rain>.95f) return false;
    const Vector3 sun=sunDirection(night);
    const Vector3 right=Vector3Normalize(Vector3CrossProduct(Vector3Scale(sun,-1),{0,1,0}));
    const Vector3 up=Vector3CrossProduct(right,Vector3Scale(sun,-1));
    Vector3 focus=camera.target;
    // Snap the light camera to shadow texels so static shadows do not
    // crawl across the screen as the player drives.
    const float texel=shadowWidth/shadowSize;
    const auto snap=[texel](float v){ return std::round(v/texel)*texel; };
    const float x=Vector3DotProduct(focus,right),y=Vector3DotProduct(focus,up);
    focus=Vector3Add(focus,Vector3Add(Vector3Scale(right,snap(x)-x),
                                    Vector3Scale(up,snap(y)-y)));
    Camera3D light{};
    light.target=focus;
    light.position=Vector3Add(focus,Vector3Scale(sun,150.f));
    light.up=up;
    light.fovy=shadowWidth;
    light.projection=CAMERA_ORTHOGRAPHIC;
    BeginTextureMode(shadowMap_);
    ClearBackground(WHITE);
    BeginMode3D(light);
    lightVP_=MatrixMultiply(rlGetMatrixModelview(),rlGetMatrixProjection());
    BeginShaderMode(depthShader_);
    return true;
}

void Atmosphere::endShadow() {
    EndShaderMode();
    EndMode3D();
    EndTextureMode();
    shadowValid_=true;
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
    const Vector3 shadow{1.f/shadowSize,
                         (1.f-std::clamp((night-.55f)/.33f,0.f,1.f))*(1.f-rain),
                         shadowValid_?1.f:0.f};
    SetShaderValue(shader_,shadowParams_,&shadow,SHADER_UNIFORM_VEC3);
    if(shadowValid_) SetShaderValueMatrix(shader_,lightMatrix_,lightVP_);
    const int slot=shadowSlot;
    SetShaderValue(shader_,shadowSampler_,&slot,SHADER_UNIFORM_INT);
    // Bind outside raylib's transient batch texture slots so every
    // internal batch flush samples the same completed depth map.
    rlActiveTextureSlot(shadowSlot);
    rlEnableTexture(shadowValid_?shadowMap_.texture.id:rlGetTextureIdDefault());
    rlActiveTextureSlot(0);
}
void Atmosphere::end() const {
    if(ready_) {
        EndShaderMode();
        rlActiveTextureSlot(shadowSlot);
        rlDisableTexture();
        rlActiveTextureSlot(0);
    }
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
