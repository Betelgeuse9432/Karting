#pragma once
#include "raylib.h"

namespace karting {

// Linear floating-point scene, soft bloom and final display transform.
// A failed shader or framebuffer leaves the old direct renderer usable.
class PostProcess {
public:
    void initialize();
    void shutdown();
    bool begin(int width,int height);
    void sky(const Camera3D& camera,Color zenith,Color horizon,Vector3 sun,
             float night,float rain) const;
    void beginLinear() const;
    void endLinear() const;
    void finish(float night,float rain);
    bool active() const { return active_; }
private:
    void releaseTargets();
    bool resize(int width,int height);
    Shader sky_{},linear_{},extract_{},blur_{},composite_{};
    RenderTexture2D scene_{},bloomA_{},bloomB_{};
    int width_=0,height_=0;
    bool ready_=false,active_=false;
};
} // namespace karting
