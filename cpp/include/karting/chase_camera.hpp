#pragma once
#include "karting/core.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

// Visual-only chase camera dynamics. No raylib dependency: the core tests
// can check the response without a window or GPU.
struct ChaseCamera {
    double angle=0, height=0, distance=0, lookAhead=0, lookAngle=0, fov=68;

    static double ease(double from,double to,double rate,double dt) {
        return from+(to-from)*(-std::expm1(-rate*dt));
    }

    void reset(const Car& car,const VehicleProfile& spec) {
        angle=car.heading;
        height=car.height+spec.cameraHeight;
        distance=spec.cameraDistance;
        lookAhead=0;
        lookAngle=0;
        fov=68;
    }

    void update(const Car& car,const VehicleProfile& spec,double dt,bool dynamic=true) {
        dt=std::clamp(dt,0.0,.15);
        if(dt<=0) return;
        const double speed=std::clamp(std::abs(car.speed)/spec.topSpeed,0.0,1.0);
        const double forward=std::clamp(car.speed/spec.topSpeed,0.0,1.0);
        const double response=std::sqrt(forward);
        const double braking=std::clamp(car.brake,0.0,1.0)*
                             std::min(1.0,forward*2.0);
        const double thrust=std::clamp(car.throttle,0.0,1.0)*(1.0-braking);
        const double limit=spec.maxSteer/(1.0+std::abs(car.speed)/spec.steerSensitivity);
        const double steer=std::clamp(car.steering/limit,-1.0,1.0);
        const double turnWeight=std::min(1.0,forward*2.0);

        // Keep the horizon and car framing stable. No pitch/height changes
        // from pedals, no braking-induced FOV jump, minimal lateral orbit.
        // V toggles a comfort mode with fixed distance and field of view.
        const double desiredAngle=car.heading+(dynamic?.035*steer*turnWeight:0.0);
        angle=wrapAngle(angle+ease(0.0,wrapAngle(desiredAngle-angle),9.0,dt));
        const double distanceFactor=dynamic?
            1.0+.10*thrust*response-.10*braking:1.0;
        distance=ease(distance,spec.cameraDistance*distanceFactor,3.5,dt);
        const double desiredHeight=car.height+spec.cameraHeight;
        height=ease(height,desiredHeight,8.0,dt);
        height=std::clamp(height,desiredHeight-16.0,desiredHeight+16.0);

        lookAhead=ease(lookAhead,dynamic?
                       22.0*response+8.0*std::abs(steer)*turnWeight:0.0,3.5,dt);
        lookAngle=ease(lookAngle,dynamic?.045*steer*turnWeight:0.0,3.5,dt);
        const double speedEase=speed*speed*(3.0-2.0*speed);
        fov=ease(fov,dynamic?68.0+8.0*speedEase:70.0,2.3,dt);
    }
};

} // namespace karting
