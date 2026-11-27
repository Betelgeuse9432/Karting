#include "karting/post_process.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace karting {
namespace {
constexpr const char* skySource=R"GLSL(#version 330
uniform vec2 screenSize;
uniform vec3 cameraForward;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform vec2 cameraSlope;
uniform vec3 zenithColor;
uniform vec3 horizonColor;
uniform vec3 sunDirection;
uniform vec2 weather;
out vec4 finalColor;
void main() {
    vec2 ndc=gl_FragCoord.xy/screenSize*2.0-1.0;
    vec3 ray=normalize(cameraForward+cameraRight*ndc.x*cameraSlope.x+
                       cameraUp*ndc.y*cameraSlope.y);
    float elevation=max(ray.y,0.0);
    vec3 sky=mix(horizonColor,zenithColor,pow(elevation,0.48));
    float sunAngle=acos(clamp(dot(ray,sunDirection),-1.0,1.0));
    float night=weather.x,rain=weather.y;
    float sunlight=(1.0-smoothstep(0.55,0.78,night))*(1.0-0.95*rain);
    float dusk=smoothstep(0.10,0.55,night);
    vec3 sunColor=mix(vec3(1.0,0.86,0.57),vec3(1.0,0.30,0.065),dusk);
    // Stylized angular radius, independent of screen resolution or FOV.
    float radius=0.019+0.006*dusk;
    float aa=max(fwidth(sunAngle),0.0002);
    float disk=1.0-smoothstep(radius-aa,radius+aa,sunAngle);
    float radial=clamp(sunAngle/radius,0.0,1.0);
    float limb=0.48+0.52*sqrt(max(0.0,1.0-radial*radial));
    float aureole=exp(-sunAngle/0.085)*0.65+exp(-sunAngle/0.34)*0.12;
    sky+=sunColor*sunlight*(disk*limb*mix(12.0,3.3,dusk)+aureole*mix(1.0,0.54,dusk));
    // A cooler moon with a small soft halo, fixed in world direction.
    vec3 moon=normalize(vec3(0.65,0.58,0.48));
    float ma=acos(clamp(dot(ray,moon),-1.0,1.0));
    float moonDisk=1.0-smoothstep(0.012,0.014,ma);
    float moonOn=smoothstep(0.57,0.94,night)*(1.0-rain);
    sky+=vec3(0.57,0.72,1.0)*moonOn*(moonDisk*1.8+exp(-ma/0.055)*0.10);
    finalColor=vec4(sky,1.0);
}
)GLSL";
constexpr const char* linearSource=R"GLSL(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main() {
    vec4 c=texture(texture0,fragTexCoord)*fragColor*colDiffuse;
    finalColor=vec4(pow(max(c.rgb,vec3(0.0)),vec3(2.2)),c.a);
}
)GLSL";
constexpr const char* extractSource=R"GLSL(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 sourceTexel;
out vec4 finalColor;
vec3 bright(vec2 uv) {
    vec3 c=texture(texture0,uv).rgb;
    float l=max(c.r,max(c.g,c.b));
    float soft=clamp(l-0.65,0.0,0.70);
    soft=soft*soft/1.40;
    return c*max(l-1.0,soft)/max(l,0.0001);
}
void main() {
    vec2 d=sourceTexel*0.75;
    vec3 c=(bright(fragTexCoord+vec2(d.x,d.y))+
            bright(fragTexCoord+vec2(-d.x,d.y))+
            bright(fragTexCoord+vec2(d.x,-d.y))+
            bright(fragTexCoord-vec2(d.x,d.y)))*0.25;
    finalColor=vec4(c,1.0);
}
)GLSL";
constexpr const char* blurSource=R"GLSL(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 direction;
out vec4 finalColor;
void main() {
    vec3 c=texture(texture0,fragTexCoord).rgb*0.227027;
    c+=(texture(texture0,fragTexCoord+direction*1.384615).rgb+
        texture(texture0,fragTexCoord-direction*1.384615).rgb)*0.316216;
    c+=(texture(texture0,fragTexCoord+direction*3.230769).rgb+
        texture(texture0,fragTexCoord-direction*3.230769).rgb)*0.070270;
    finalColor=vec4(c,1.0);
}
)GLSL";
constexpr const char* compositeSource=R"GLSL(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform sampler2D bloomTexture;
uniform vec2 grade;
out vec4 finalColor;
void main() {
    float night=grade.x,rain=grade.y;
    // Peak at the actual dusk preset (0.55), not midway to it.
    float dusk=smoothstep(0.0,0.55,night)*(1.0-smoothstep(0.55,1.0,night));
    float dark=smoothstep(0.55,1.0,night);
    float exposure=1.0+0.07*dusk+0.38*dark;
    float bloomStrength=mix(0.24,0.13,max(dusk,dark));
    vec3 c=texture(texture0,fragTexCoord).rgb*exposure+
           texture(bloomTexture,fragTexCoord).rgb*bloomStrength;
    c*=mix(vec3(1.05,1.02,0.99),vec3(1.14,1.00,0.88),dusk);
    c*=mix(vec3(1.0),vec3(0.90,0.98,1.12),dark);
    float lum=dot(c,vec3(0.2126,0.7152,0.0722));
    c=mix(c,vec3(lum),0.13*rain);
    // Luminance-based filmic shoulder preserves the original midtones
    // and hue while compressing high-energy highlights.
    float peak=max(c.r,max(c.g,c.b));
    float mapped=peak<=0.55?peak:0.55+0.45*(1.0-exp(-(peak-0.55)/0.45));
    c*=mapped/max(peak,0.00001);
    finalColor=vec4(pow(clamp(c,0.0,1.0),vec3(1.0/2.2)),1.0);
}
)GLSL";

bool usable(Shader shader) {
    return shader.id!=0 && shader.id!=rlGetShaderIdDefault();
}
void uniform(Shader shader,const char* name,const void* value,int type) {
    const int location=GetShaderLocation(shader,name);
    if(location>=0) SetShaderValue(shader,location,value,type);
}
Vector3 linearColor(Color c) {
    return {std::pow(c.r/255.f,2.2f),std::pow(c.g/255.f,2.2f),
            std::pow(c.b/255.f,2.2f)};
}
RenderTexture2D floatTarget(int width,int height) {
    RenderTexture2D target{};
    target.id=rlLoadFramebuffer();
    if(!target.id) return {};
    target.texture={rlLoadTexture(nullptr,width,height,PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,1),
                    width,height,1,PIXELFORMAT_UNCOMPRESSED_R16G16B16A16};
    if(target.texture.id)
        rlFramebufferAttach(target.id,target.texture.id,RL_ATTACHMENT_COLOR_CHANNEL0,
                            RL_ATTACHMENT_TEXTURE2D,0);
    target.depth={rlLoadTextureDepth(width,height,true),width,height,1,19};
    if(target.depth.id)
        rlFramebufferAttach(target.id,target.depth.id,RL_ATTACHMENT_DEPTH,
                            RL_ATTACHMENT_RENDERBUFFER,0);
    if(!target.texture.id||!target.depth.id||!rlFramebufferComplete(target.id)) {
        UnloadRenderTexture(target);
        return {};
    }
    SetTextureFilter(target.texture,TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(target.texture,TEXTURE_WRAP_CLAMP);
    return target;
}
void screenQuad(Texture2D texture,int width,int height) {
    DrawTexturePro(texture,{0,0,float(texture.width),-float(texture.height)},
                   {0,0,float(width),float(height)},{0,0},0,WHITE);
}
void pass(RenderTexture2D target,Texture2D source,Shader shader) {
    BeginTextureMode(target);
    ClearBackground(BLACK);
    BeginShaderMode(shader);
    screenQuad(source,target.texture.width,target.texture.height);
    EndShaderMode();
    EndTextureMode();
}
} // namespace

void PostProcess::initialize() {
    sky_=LoadShaderFromMemory(nullptr,skySource);
    linear_=LoadShaderFromMemory(nullptr,linearSource);
    extract_=LoadShaderFromMemory(nullptr,extractSource);
    blur_=LoadShaderFromMemory(nullptr,blurSource);
    composite_=LoadShaderFromMemory(nullptr,compositeSource);
    ready_=usable(sky_)&&usable(linear_)&&usable(extract_)&&usable(blur_)&&usable(composite_);
    if(!ready_) TraceLog(LOG_WARNING,"KARTING: HDR shaders unavailable; direct rendering retained");
}
void PostProcess::releaseTargets() {
    for(RenderTexture2D* target:{&scene_,&bloomA_,&bloomB_}) {
        if(target->id) UnloadRenderTexture(*target);
        *target={};
    }
}
void PostProcess::shutdown() {
    releaseTargets();
    for(Shader* shader:{&sky_,&linear_,&extract_,&blur_,&composite_}) {
        if(usable(*shader)) UnloadShader(*shader);
        *shader={};
    }
    ready_=active_=false;
}
bool PostProcess::resize(int width,int height) {
    if(width==width_&&height==height_) return scene_.id!=0;
    releaseTargets();
    width_=width;height_=height;
    scene_=floatTarget(width,height);
    bloomA_=floatTarget(std::max(1,width/4),std::max(1,height/4));
    bloomB_=floatTarget(std::max(1,width/4),std::max(1,height/4));
    if(!scene_.id||!bloomA_.id||!bloomB_.id) {
        releaseTargets();
        TraceLog(LOG_WARNING,"KARTING: floating point targets unavailable; direct rendering retained");
        return false;
    }
    TraceLog(LOG_INFO,"KARTING: HDR RGBA16F scene %ix%i, bloom %ix%i",width,height,
             bloomA_.texture.width,bloomA_.texture.height);
    return true;
}
bool PostProcess::begin(int width,int height) {
    active_=ready_&&width>0&&height>0&&resize(width,height);
    if(active_) {
        BeginTextureMode(scene_);
        ClearBackground(BLACK);
    }
    return active_;
}
void PostProcess::sky(const Camera3D& camera,Color zenith,Color horizon,Vector3 sun,
                      float night,float rain) const {
    if(!active_) return;
    const Vector3 forward=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
    const Vector3 right=Vector3Normalize(Vector3CrossProduct(forward,camera.up));
    const Vector3 up=Vector3CrossProduct(right,forward);
    const float slope=std::tan(camera.fovy*DEG2RAD*.5f);
    const Vector2 slopes{slope*float(width_)/height_,slope};
    const Vector2 size{float(width_),float(height_)},weather{night,rain};
    const Vector3 top=linearColor(zenith),bottom=linearColor(horizon);
    BeginShaderMode(sky_);
    uniform(sky_,"screenSize",&size,SHADER_UNIFORM_VEC2);
    uniform(sky_,"cameraForward",&forward,SHADER_UNIFORM_VEC3);
    uniform(sky_,"cameraRight",&right,SHADER_UNIFORM_VEC3);
    uniform(sky_,"cameraUp",&up,SHADER_UNIFORM_VEC3);
    uniform(sky_,"cameraSlope",&slopes,SHADER_UNIFORM_VEC2);
    uniform(sky_,"zenithColor",&top,SHADER_UNIFORM_VEC3);
    uniform(sky_,"horizonColor",&bottom,SHADER_UNIFORM_VEC3);
    uniform(sky_,"sunDirection",&sun,SHADER_UNIFORM_VEC3);
    uniform(sky_,"weather",&weather,SHADER_UNIFORM_VEC2);
    DrawRectangle(0,0,width_,height_,WHITE);
    EndShaderMode();
}
void PostProcess::beginLinear() const { if(active_) BeginShaderMode(linear_); }
void PostProcess::endLinear() const { if(active_) EndShaderMode(); }
void PostProcess::finish(float night,float rain) {
    if(!active_) return;
    EndTextureMode();
    const Vector2 texel{1.f/width_,1.f/height_};
    uniform(extract_,"sourceTexel",&texel,SHADER_UNIFORM_VEC2);
    pass(bloomA_,scene_.texture,extract_);
    // Two separable Gaussian pairs give a broad, soft halo.
    for(float radius:{1.5f,3.5f}) {
        const Vector2 x{radius/bloomA_.texture.width,0},y{0,radius/bloomA_.texture.height};
        uniform(blur_,"direction",&x,SHADER_UNIFORM_VEC2);
        pass(bloomB_,bloomA_.texture,blur_);
        uniform(blur_,"direction",&y,SHADER_UNIFORM_VEC2);
        pass(bloomA_,bloomB_.texture,blur_);
    }
    BeginShaderMode(composite_);
    const Vector2 grade{night,rain};
    uniform(composite_,"grade",&grade,SHADER_UNIFORM_VEC2);
    SetShaderValueTexture(composite_,GetShaderLocation(composite_,"bloomTexture"),bloomA_.texture);
    screenQuad(scene_.texture,width_,height_);
    EndShaderMode();
    active_=false;
}
} // namespace karting
