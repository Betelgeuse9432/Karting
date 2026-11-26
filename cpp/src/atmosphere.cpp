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
uniform int hdrMode;
uniform vec4 lunarLight;
uniform int lampCount;
uniform vec4 lampPositions[32];
uniform vec4 lampDirections[32];
uniform vec4 headPositions[2];
uniform vec4 headDirections[2];
uniform vec3 headUp;
uniform vec2 headSettings; // intensity, high beam
uniform vec3 shadowParams; // inverse size, strength, valid
out vec4 finalColor;

float kartingHash(vec3 p) {
    p=fract(p*0.1031);
    p+=dot(p,p.yzx+33.33);
    return fract((p.x+p.y)*p.z);
}
float kartingValueNoise(vec3 p) {
    vec3 i=floor(p),f=fract(p);
    f=f*f*(3.0-2.0*f);
    return mix(mix(mix(kartingHash(i),kartingHash(i+vec3(1,0,0)),f.x),
                   mix(kartingHash(i+vec3(0,1,0)),kartingHash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(kartingHash(i+vec3(0,0,1)),kartingHash(i+vec3(1,0,1)),f.x),
                   mix(kartingHash(i+vec3(0,1,1)),kartingHash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
float kartingDetailNoise(vec3 p,float frequency) {
    // Suppress subpixel detail instead of producing shimmer while driving.
    float footprint=max(length(dFdx(p)),length(dFdy(p)))*frequency;
    return mix(kartingValueNoise(p*frequency),0.5,smoothstep(0.35,1.4,footprint));
}
float sunVisibility(vec3 position,float facing) {
    if(shadowParams.z<0.5) return 1.0;
    vec4 clip=lightVP*vec4(position,1.0);
    vec3 uvz=clip.xyz/clip.w*0.5+0.5;
    if(any(lessThanEqual(uvz,vec3(0.0)))||any(greaterThanEqual(uvz,vec3(1.0)))) return 1.0;
    float bias=0.00010+0.00030*(1.0-facing);
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
    float visibility=sunVisibility(worldPosition+normal*0.018,facing);
    int material=int(floor(fragColor.a*255.0+0.5));
    bool tagged=material>=247 && material<=254;
    if(tagged) base.a=1.0;
    vec3 p=worldPosition;
    float broad=kartingValueNoise(p*0.55);
    float fine=kartingDetailNoise(p,24.0);
    float roughness=0.85;
    float reflectance=0.0;
    if(material==254) { // asphalt: aggregate, dark patches and rubber
        float grit=kartingDetailNoise(p,65.0);
        base.rgb*=0.91+0.20*broad+0.20*(fine-0.5)+0.16*(grit-0.5);
        roughness=mix(0.82,0.25,lighting.z);
        reflectance=mix(0.025,0.32,lighting.z);
    } else if(material==253) {
        float patches=kartingValueNoise(p*0.18);
        base.rgb=mix(base.rgb*vec3(0.78,0.90,0.70),
                     base.rgb*vec3(1.12,1.04,0.76),patches);
        base.rgb*=0.87+0.22*broad+0.30*(fine-0.5);
    } else if(material==252) {
        base.rgb*=0.87+0.22*broad+0.17*(fine-0.5);
        roughness=0.73; reflectance=0.025;
    } else if(material==251) {
        base.rgb*=0.78+0.36*broad+0.32*(fine-0.5);
        roughness=0.95;
    } else if(material==250) {
        roughness=0.19; reflectance=0.32;
    } else if(material==249) {
        roughness=0.10; reflectance=0.58;
    }
    float shade=mix(1.0,0.60+0.54*facing*visibility,strength);
    shade*=mix(0.76,1.0,visibility);
    vec3 warm=mix(vec3(1.0),vec3(1.13,0.97,0.79),lighting.y*facing*visibility);
    vec3 cool=mix(vec3(1.0),vec3(0.84,0.91,1.08),lighting.y*(1.0-facing)*0.5);
    vec3 lit=base.rgb*shade*warm*cool;
    float moonFacing=max(dot(normal,lunarLight.xyz),0.0);
    lit*=mix(vec3(1.0),vec3(0.76,0.88,1.13)*(0.78+0.42*moonFacing),lunarLight.w);
    vec3 highEnergy=vec3(0.0);
    if(reflectance>0.0) {
        vec3 view=normalize(eyePosition-p);
        vec3 halfVector=normalize(view+sunDirection);
        float ndv=max(dot(normal,view),0.0);
        float fresnel=0.04+0.96*pow(1.0-ndv,5.0);
        float specular=pow(max(dot(normal,halfVector),0.0),
                           mix(100.0,5.0,roughness));
        vec3 reflected=reflect(-view,normal);
        // Analytic sky reflection, not a scene reflection or mirror.
        vec3 sky=mix(horizonColor,vec3(0.30,0.53,0.80)*(0.18+0.82*lighting.x),
                     clamp(reflected.y,0.0,1.0));
        lit=mix(lit,sky,reflectance*(0.20+0.80*fresnel));
        lit+=vec3(1.0,0.92,0.79)*specular*reflectance*strength*visibility;
        highEnergy=vec3(1.0,0.82,0.59)*specular*reflectance*strength*visibility*2.2;
    }
    // Local directional lamps: real world-space lighting on every surface.
    // Cone + range + Lambert response, not screen-space painted circles.
    vec3 lampEnergy=vec3(0.0);
    for(int i=0;i<lampCount;++i) {
        vec3 delta=lampPositions[i].xyz-p;
        float distanceToLamp=length(delta);
        vec3 L=delta/max(distanceToLamp,0.001);
        float cone=smoothstep(0.52,0.85,dot(-L,lampDirections[i].xyz));
        float falloff=pow(max(0.0,1.0-distanceToLamp/20.0),2.0);
        float diffuse=max(dot(normal,L),0.0);
        float energy=lampPositions[i].w*cone*falloff*diffuse*8.0/(1.0+0.05*distanceToLamp*distanceToLamp);
        lampEnergy+=vec3(1.0,0.76,0.44)*energy*pow(max(base.rgb,vec3(0.0)),vec3(1.4));
    }
    // Two independently positioned, flattened automotive spotlights.
    // The cutoff is evaluated in the car's own up/forward frame so it
    // follows banking, chassis pitch, and road grade.
    if(headSettings.x>0.001) {
        float high=headSettings.y;
        float beamRange=mix(42.0,75.0,high);
        for(int i=0;i<2;++i) {
            vec3 delta=p-headPositions[i].xyz;
            float dist=length(delta);
            vec3 ray=delta/max(dist,0.001);
            vec3 forward=normalize(headDirections[i].xyz);
            vec3 side=normalize(cross(forward,headUp));
            vec3 up=normalize(cross(side,forward));
            float along=dot(ray,forward);
            float horizontal=dot(ray,side)/max(along,0.01);
            float vertical=dot(ray,up)/max(along,0.01);
            float width=mix(0.48,0.31,high);
            float spread=1.0-smoothstep(width*0.42,width,abs(horizontal));
            float ceiling=mix(0.035,0.14,high);
            float cutoff=1.0-smoothstep(ceiling-0.026,ceiling+0.021,vertical);
            float floorFade=smoothstep(-0.46,-0.11,vertical);
            float beam=spread*cutoff*floorFade*smoothstep(0.08,0.60,along);
            float fade=1.0-smoothstep(beamRange*0.66,beamRange,dist);
            float incidence=max(dot(normal,-ray),0.0);
            float power=mix(2.7,5.2,high)*beam*fade/(1.0+0.006*dist*dist);
            // A mild grazing term makes the road readable on long, flat
            // straights while vertical surfaces still receive true diffuse.
            float irradiance=power*(incidence+0.13*sqrt(incidence));
            vec3 tint=vec3(1.0,0.94,0.80);
            lampEnergy+=tint*irradiance*headSettings.x*pow(max(base.rgb,vec3(0.0)),vec3(1.5));
            if(reflectance>0.0) {
                vec3 halfDir=normalize(normalize(eyePosition-p)-ray);
                float glint=pow(max(dot(normal,halfDir),0.0),mix(95.0,8.0,roughness));
                lampEnergy+=tint*power*glint*reflectance*headSettings.x*0.36;
            }
        }
    }
    if(material==247) {
        lit=base.rgb;
        highEnergy+=vec3(2.2,2.0,1.5)*headSettings.x*(1.0+headSettings.y*0.35);
    }
    if(material==248) {
        lit=base.rgb;
        highEnergy+=vec3(4.6,2.6,0.85)*smoothstep(0.0,0.7,lunarLight.w+0.32);
    }
    float distanceToEye=length(worldPosition-eyePosition);
    float haze=smoothstep(fogRange.x,fogRange.y,distanceToEye);
    vec3 color=mix(lit,horizonColor,haze);
    if(hdrMode!=0) color=pow(max(color,vec3(0.0)),vec3(2.2))+(highEnergy+lampEnergy)*(1.0-haze);
    else color+=lampEnergy*(1.0-haze);
    finalColor=vec4(color,base.a);
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
    hdr_=GetShaderLocation(shader_,"hdrMode");
    lunar_=GetShaderLocation(shader_,"lunarLight");
    lampCount_=GetShaderLocation(shader_,"lampCount");
    lampPositions_=GetShaderLocation(shader_,"lampPositions[0]");
    lampDirections_=GetShaderLocation(shader_,"lampDirections[0]");
    headPositions_=GetShaderLocation(shader_,"headPositions[0]");
    headDirections_=GetShaderLocation(shader_,"headDirections[0]");
    headUp_=GetShaderLocation(shader_,"headUp");
    headSettings_=GetShaderLocation(shader_,"headSettings");
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

Vector3 Atmosphere::moonDirection() {
    return Vector3Normalize({.65f,.58f,.48f});
}

bool Atmosphere::beginShadow(const Camera3D& camera,float night,float rain) {
    shadowValid_=false;
    if(!ready_||!shadowReady_||night>.78f||rain>.95f) return false;
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

void Atmosphere::begin(const Camera3D& camera,Color horizon,float night,float rain,int quality,bool hdr,const StreetLights& lamps,const Headlights& headlights) const {
    if(!ready_) return;
    BeginShaderMode(shader_);
    const int hdrMode=hdr?1:0;
    SetShaderValue(shader_,hdr_,&hdrMode,SHADER_UNIFORM_INT);
    const Vector3 eye=camera.position;
    const Vector3 sun=sunDirection(night);
    const Vector3 fogColor{horizon.r/255.f,horizon.g/255.f,horizon.b/255.f};
    const float warm=std::max(0.f,1.f-std::abs(night-.48f)/.48f);
    const float lunarAmount=std::clamp((night-.55f)/.45f,0.f,1.f);
    const Vector3 light{1.f-.99f*lunarAmount,warm*(1.f-rain),rain};
    const Vector3 moon=moonDirection();
    const Vector4 lunar{moon.x,moon.y,moon.z,lunarAmount*(1.f-.6f*rain)};
    SetShaderValue(shader_,lunar_,&lunar,SHADER_UNIFORM_VEC4);
    SetShaderValue(shader_,lampCount_,&lamps.count,SHADER_UNIFORM_INT);
    if(lamps.count>0) {
        SetShaderValueV(shader_,lampPositions_,lamps.positions.data(),SHADER_UNIFORM_VEC4,lamps.count);
        SetShaderValueV(shader_,lampDirections_,lamps.directions.data(),SHADER_UNIFORM_VEC4,lamps.count);
    }
    const Vector2 headSettings{headlights.intensity,headlights.high?1.f:0.f};
    SetShaderValue(shader_,headSettings_,&headSettings,SHADER_UNIFORM_VEC2);
    if(headlights.intensity>0.001f) {
        SetShaderValueV(shader_,headPositions_,headlights.positions.data(),SHADER_UNIFORM_VEC4,2);
        SetShaderValueV(shader_,headDirections_,headlights.directions.data(),SHADER_UNIFORM_VEC4,2);
        SetShaderValue(shader_,headUp_,&headlights.up,SHADER_UNIFORM_VEC3);
    }
    const float end=(quality==0?63.f:quality==1?82.f:102.f)*(1.f-.36f*rain);
    const Vector2 range{end*(.34f-.15f*rain),end};
    SetShaderValue(shader_,eye_,&eye,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,sun_,&sun,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,horizon_,&fogColor,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,lighting_,&light,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,fog_,&range,SHADER_UNIFORM_VEC2);
    const Vector3 shadow{1.f/shadowSize,
                         (1.f-std::clamp((night-.55f)/.23f,0.f,1.f))*(1.f-rain),
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
