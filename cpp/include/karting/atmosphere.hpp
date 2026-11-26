#pragma once
#include "karting/core.hpp"
#include "raylib.h"

namespace karting {

// Directional forward lighting with an independent sun-space shadow pass.
// The original forward shader remains usable if the shadow target fails.
class Atmosphere {
public:
    void initialize();
    void shutdown();
    void begin(const Camera3D& camera,Color horizon,float night,float rain,int quality) const;
    void end() const;
    bool beginShadow(const Camera3D& camera,float night,float rain);
    void endShadow();
    bool ready() const { return ready_; }
    static Vector3 sunDirection(float night);
private:
    Shader shader_{};
    Shader depthShader_{};
    RenderTexture2D shadowMap_{};
    Matrix lightVP_{};
    bool shadowReady_=false, shadowValid_=false;
    bool ready_=false;
    int eye_=-1,sun_=-1,horizon_=-1,lighting_=-1,fog_=-1;
    int shadowSampler_=-1,lightMatrix_=-1,shadowParams_=-1;
};

void drawContactShadow(const Track& track,const Car& car,const VehicleProfile& spec,
                       float night,float rain);

} // namespace karting
