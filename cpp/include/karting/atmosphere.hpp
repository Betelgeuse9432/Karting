#pragma once
#include "karting/core.hpp"
#include "raylib.h"

namespace karting {

// All scene primitives share one forward-pass shader. No render targets or
// shadow maps are required. The default raylib shader remains a fallback.
class Atmosphere {
public:
    void initialize();
    void shutdown();
    void begin(const Camera3D& camera,Color horizon,float night,float rain,int quality) const;
    void end() const;
    bool ready() const { return ready_; }
    static Vector3 sunDirection(float night);
private:
    Shader shader_{};
    bool ready_=false;
    int eye_=-1,sun_=-1,horizon_=-1,lighting_=-1,fog_=-1;
};

void drawContactShadow(const Track& track,const Car& car,const VehicleProfile& spec,
                       float night,float rain);

} // namespace karting
