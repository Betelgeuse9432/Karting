#pragma once
#include "karting/core.hpp"
#include "raylib.h"
#include <array>

namespace karting {

// Opaque surface tags travel in vertex alpha (restored to 1 by the shader).
// Reserve only 247..254; ordinary opaque and translucent draws are unchanged.
enum class Surface : unsigned char { Asphalt=254, Grass=253, Concrete=252,
                                     Soil=251, Paint=250, Glass=249, Lamp=248, Headlamp=247 };
inline Color surface(Color c,Surface kind) { c.a=static_cast<unsigned char>(kind); return c; }

constexpr int kMaxStreetLights=32;
struct StreetLights {
    std::array<Vector4,kMaxStreetLights> positions{};
    std::array<Vector4,kMaxStreetLights> directions{};
    int count=0;
};

enum class HeadlightMode { Auto, Low, High, Off };
struct Headlights {
    std::array<Vector4,2> positions{};
    std::array<Vector4,2> directions{};
    Vector3 up{0,1,0};
    float intensity=0.f;
    bool high=false;
};

// Directional forward lighting with an independent sun-space shadow pass.
// The original forward shader remains usable if the shadow target fails.
class Atmosphere {
public:
    void initialize();
    void shutdown();
    void begin(const Camera3D& camera,Color horizon,float night,float rain,int quality,bool hdr=false,const StreetLights& lamps={},const Headlights& headlights={}) const;
    void end() const;
    bool beginShadow(const Camera3D& camera,float night,float rain);
    void endShadow();
    void clearShadow() { shadowValid_=false; }
    bool ready() const { return ready_; }
    static Vector3 sunDirection(float night);
    static Vector3 moonDirection();
private:
    Shader shader_{};
    Shader depthShader_{};
    RenderTexture2D shadowMap_{};
    Matrix lightVP_{};
    bool shadowReady_=false, shadowValid_=false;
    bool ready_=false;
    int eye_=-1,sun_=-1,horizon_=-1,lighting_=-1,fog_=-1;
    int shadowSampler_=-1,lightMatrix_=-1,shadowParams_=-1,hdr_=-1;
    int lunar_=-1,lampCount_=-1,lampPositions_=-1,lampDirections_=-1;
    int headPositions_=-1,headDirections_=-1,headUp_=-1,headSettings_=-1;
};

void drawContactShadow(const Track& track,const Car& car,const VehicleProfile& spec,
                       float night,float rain);

} // namespace karting
